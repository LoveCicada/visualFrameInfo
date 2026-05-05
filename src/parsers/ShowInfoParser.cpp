#include "ShowInfoParser.h"

#include <QHash>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

namespace
{
QRegularExpression frameLineRegex(
    "n:\\s*(\\d+)\\s+pts:\\s*(-?\\d+)\\s+pts_time:([0-9\\.-]+)\\s+duration:\\s*(-?\\d+)\\s+duration_time:([0-9\\.-]+).*iskey:(\\d).*type:([IPB])");
QRegularExpression resolutionRegex("s:(\\d+x\\d+)");
QRegularExpression fpsConfigRegex("frame_rate:\\s*(\\d+)\\/(\\d+)");
QRegularExpression fpsSimpleRegex("([0-9]+(?:\\.[0-9]+)?)\\s*fps");
QRegularExpression codecRegex("Video:\\s*([a-zA-Z0-9_]+)");

double parseFps(const QString &numeratorText, const QString &denominatorText)
{
    const double numerator = numeratorText.toDouble();
    const double denominator = denominatorText.toDouble();
    if (denominator == 0.0) {
        return 0.0;
    }
    return numerator / denominator;
}

bool tryBuildFrameFromFfprobe(const QHash<QString, QString> &fields, int frameIndex, FrameInfo &frame)
{
    const QString typeText = fields.value("pict_type");
    if (typeText.isEmpty()) {
        return false;
    }

    const QChar type = typeText.at(0);
    if (type != QChar('I') && type != QChar('P') && type != QChar('B')) {
        return false;
    }

    frame.index = frameIndex;
    frame.type = type;
    frame.isKey = fields.value("key_frame") == "1";
    frame.pts = fields.value("best_effort_timestamp").toLongLong();
    frame.ptsTime = fields.value("best_effort_timestamp_time").toDouble();
    frame.duration = fields.value("pkt_duration").toLongLong();
    frame.durationTime = fields.value("pkt_duration_time").toDouble();
    frame.rawLine = QString("ffprobe frame=%1 type=%2 key=%3")
                        .arg(frame.index)
                        .arg(frame.type)
                        .arg(frame.isKey ? "1" : "0");
    return true;
}

struct FfprobeFrameFields
{
    QString pictType;
    bool keyFrame = false;
    qint64 pts = 0;
    double ptsTime = 0.0;
    qint64 duration = 0;
    double durationTime = 0.0;
    int pktSize = 0;

    void clear()
    {
        pictType.clear();
        keyFrame = false;
        pts = 0;
        ptsTime = 0.0;
        duration = 0;
        durationTime = 0.0;
        pktSize = 0;
    }
};

void parseFfprobeFrameField(const QString &line, FfprobeFrameFields &fields)
{
    if (line.startsWith("pict_type=")) {
        fields.pictType = line.mid(10).trimmed();
    } else if (line.startsWith("key_frame=")) {
        fields.keyFrame = line.mid(10).trimmed() == "1";
    } else if (line.startsWith("best_effort_timestamp=")) {
        fields.pts = line.mid(22).trimmed().toLongLong();
    } else if (line.startsWith("best_effort_timestamp_time=")) {
        fields.ptsTime = line.mid(27).trimmed().toDouble();
    } else if (line.startsWith("pkt_duration=")) {
        fields.duration = line.mid(13).trimmed().toLongLong();
    } else if (line.startsWith("pkt_duration_time=")) {
        fields.durationTime = line.mid(18).trimmed().toDouble();
    } else if (line.startsWith("pkt_size=")) {
        fields.pktSize = line.mid(9).trimmed().toInt();
    }
}

bool tryBuildFrameFromFfprobeFast(const FfprobeFrameFields &fields, int frameIndex, FrameInfo &frame)
{
    if (fields.pictType.isEmpty()) {
        return false;
    }

    const QChar type = fields.pictType.at(0);
    if (type != QChar('I') && type != QChar('P') && type != QChar('B')) {
        return false;
    }

    frame.index = frameIndex;
    frame.type = type;
    frame.isKey = fields.keyFrame;
    frame.pts = fields.pts;
    frame.ptsTime = fields.ptsTime;
    frame.duration = fields.duration;
    frame.durationTime = fields.durationTime;
    frame.sizeInBytes = fields.pktSize;
    if (frame.durationTime > 0.0 && frame.sizeInBytes > 0) {
        frame.bitrate = (static_cast<double>(frame.sizeInBytes) * 8.0) / frame.durationTime / 1000.0;
    }
    frame.rawLine = QString("ffprobe frame=%1 type=%2 key=%3")
                        .arg(frame.index)
                        .arg(frame.type)
                        .arg(frame.isKey ? "1" : "0");
    return true;
}

bool parseFpsFromFraction(const QString &fractionText, double &fpsValue, QString &fpsText)
{
    const QStringList parts = fractionText.split('/');
    if (parts.size() != 2) {
        return false;
    }

    const double fps = parseFps(parts[0], parts[1]);
    if (fps <= 0.0) {
        return false;
    }

    fpsValue = fps;
    fpsText = QString::number(fpsValue, 'f', 3) + " fps";
    return true;
}

bool isMeaningfulMetadataValue(const QString &value)
{
    const QString trimmed = value.trimmed();
    return !trimmed.isEmpty() && trimmed != "unknown" && trimmed != "N/A";
}

int inferBitDepthFromPixFmt(const QString &pixFmt)
{
    if (pixFmt.isEmpty()) {
        return 0;
    }

    // Common ffmpeg formats: yuv420p, yuv420p10le, p010le, gbrp12le
    const QRegularExpression depthInPixFmtRe("p(\\d+)(?:le|be)?$");
    const QRegularExpressionMatch depthMatch = depthInPixFmtRe.match(pixFmt);
    if (depthMatch.hasMatch()) {
        const int depth = depthMatch.captured(1).toInt();
        if (depth > 0) {
            return depth;
        }
    }

    // No explicit depth in pix_fmt usually means 8-bit.
    return 8;
}
}

bool ShowInfoParser::parseFile(const QString &logPath, QVector<FrameInfo> &frames, QString &errorMessage)
{
    frames.clear();

    QFile file(logPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorMessage = QString("Failed to open log file: %1").arg(logPath);
        return false;
    }

    QTextStream stream(&file);
    bool inFrameSection = false;
    FfprobeFrameFields ffprobeFrameFields;

    while (!stream.atEnd()) {
        const QString line = stream.readLine();

        // CSV format: ffprobe -of csv outputs one line per frame:
        // frame,{key_frame},{best_effort_timestamp},{best_effort_timestamp_time},{pkt_duration},{pkt_duration_time},{pkt_size},{pict_type}
        if (line.startsWith("frame,")) {
            const QStringList parts = line.split(',');
            if (parts.size() >= 8) {
                const QString typeStr = parts.at(7);
                if (!typeStr.isEmpty()) {
                    const QChar type = typeStr.at(0);
                    if (type == QChar('I') || type == QChar('P') || type == QChar('B')) {
                        FrameInfo frame;
                        frame.index = frames.size();
                        frame.isKey = (parts.at(1) == "1");
                        frame.type = type;
                        frame.pts = parts.at(2).toLongLong();
                        frame.ptsTime = parts.at(3).toDouble();
                        frame.duration = parts.at(4).toLongLong();
                        frame.durationTime = parts.at(5).toDouble();
                        frame.sizeInBytes = parts.at(6).toInt();
                        if (frame.durationTime > 0.0 && frame.sizeInBytes > 0) {
                            frame.bitrate = (static_cast<double>(frame.sizeInBytes) * 8.0) / frame.durationTime / 1000.0;
                        }
                        frame.rawLine = QString("ffprobe frame=%1 type=%2 key=%3")
                                            .arg(frame.index)
                                            .arg(frame.type)
                                            .arg(frame.isKey ? "1" : "0");
                        frames.push_back(frame);
                    }
                }
            }
            continue;
        }

        // Default format: ffprobe -of default=noprint_wrappers=0
        if (line == "[FRAME]") {
            inFrameSection = true;
            ffprobeFrameFields.clear();
            continue;
        }

        if (line == "[/FRAME]") {
            if (inFrameSection) {
                FrameInfo frame;
                if (tryBuildFrameFromFfprobeFast(ffprobeFrameFields, frames.size(), frame)) {
                    frames.push_back(frame);
                }
            }
            inFrameSection = false;
            ffprobeFrameFields.clear();
            continue;
        }

        if (inFrameSection) {
            parseFfprobeFrameField(line, ffprobeFrameFields);
            continue;
        }

        if (!line.contains("Parsed_showinfo") || !line.contains(" type:")) {
            continue;
        }

        const QRegularExpressionMatch match = frameLineRegex.match(line);
        if (!match.hasMatch()) {
            continue;
        }

        FrameInfo frame;
        frame.index = match.captured(1).toInt();
        frame.pts = match.captured(2).toLongLong();
        frame.ptsTime = match.captured(3).toDouble();
        frame.duration = match.captured(4).toLongLong();
        frame.durationTime = match.captured(5).toDouble();
        frame.isKey = match.captured(6) == "1";
        frame.type = match.captured(7).at(0);
        frame.rawLine = line;
        frames.push_back(frame);
    }

    if (frames.isEmpty()) {
        errorMessage = "No frame rows found in analysis log.";
        return false;
    }

    return true;
}

QVector<GopSegment> ShowInfoParser::buildGops(QVector<FrameInfo> &frames)
{
    QVector<int> starts;
    starts.reserve(frames.size());

    for (int i = 0; i < frames.size(); ++i) {
        if (frames[i].type == QChar('I')) {
            starts.push_back(i);
        }
    }

    if (starts.isEmpty()) {
        starts.push_back(0);
    } else if (starts.first() != 0) {
        starts.prepend(0);
    }

    QVector<GopSegment> gops;
    gops.reserve(starts.size());

    for (int g = 0; g < starts.size(); ++g) {
        const int startPos = starts[g];
        const int endPos = (g + 1 < starts.size()) ? (starts[g + 1] - 1) : (frames.size() - 1);
        if (startPos < 0 || endPos < startPos || endPos >= frames.size()) {
            continue;
        }

        GopSegment seg;
        seg.gopIndex = g;
        seg.startFrame = frames[startPos].index;
        seg.endFrame = frames[endPos].index;
        seg.size = endPos - startPos + 1;
        seg.startTime = frames[startPos].ptsTime;
        seg.endTime = frames[endPos].ptsTime;
        gops.push_back(seg);

        for (int i = startPos; i <= endPos; ++i) {
            frames[i].gopIndex = g;
            frames[i].indexInGop = i - startPos;
        }
    }

    return gops;
}

AnalysisSummary ShowInfoParser::buildSummary(const QString &videoPath,
                                             const QString &logPath,
                                             const QVector<FrameInfo> &frames,
                                             const QVector<GopSegment> &gops)
{
    AnalysisSummary summary;
    summary.filePath = videoPath;
    summary.logPath = logPath;
    summary.totalFrames = frames.size();
    summary.gopCount = gops.size();

    QFile file(logPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        bool inStreamSection = false;
        QHash<QString, QString> ffprobeStreamFields;

        while (!stream.atEnd()) {
            const QString line = stream.readLine();

            // CSV format: ffprobe -of csv outputs stream info as one line:
            // stream,{codec_name},{width},{height},{avg_frame_rate},{r_frame_rate},...
            if (line.startsWith("stream,")) {
                const QStringList parts = line.split(',');
                // Some ffprobe versions prepend stream index/type in CSV output.
                // Parse the selected stream fields from the tail to avoid offset mismatch.
                if (parts.size() >= 10) {
                    const int n = parts.size();
                    const QString codecName = parts.at(n - 9).trimmed();
                    const QString width = parts.at(n - 8).trimmed();
                    const QString height = parts.at(n - 7).trimmed();
                    const QString avgFrameRate = parts.at(n - 6).trimmed();
                    const QString rFrameRate = parts.at(n - 5).trimmed();
                    const QString duration = parts.at(n - 4).trimmed();
                    const QString colorSpace = parts.at(n - 3).trimmed();
                    const QString bitsPerRawSample = parts.at(n - 2).trimmed();
                    const QString pixFmt = parts.at(n - 1).trimmed();

                    if (summary.codec.isEmpty() && !codecName.isEmpty()) {
                        summary.codec = codecName.toUpper();
                    }
                    if (summary.resolution.isEmpty() && !width.isEmpty() && !height.isEmpty()) {
                        summary.resolution = width + "x" + height;
                    }
                    if (summary.fpsText.isEmpty() && !avgFrameRate.isEmpty()) {
                        parseFpsFromFraction(avgFrameRate, summary.fpsValue, summary.fpsText);
                    }
                    if (summary.fpsText.isEmpty() && !rFrameRate.isEmpty()) {
                        parseFpsFromFraction(rFrameRate, summary.fpsValue, summary.fpsText);
                    }

                    if (summary.durationSeconds <= 0.0 && !duration.isEmpty()) {
                        const double d = duration.toDouble();
                        if (d > 0.0) {
                            summary.durationSeconds = d;
                        }
                    }
                    if (summary.colorSpace.isEmpty() && isMeaningfulMetadataValue(colorSpace)) {
                        summary.colorSpace = colorSpace;
                    }
                    if (summary.pixFmt.isEmpty() && isMeaningfulMetadataValue(pixFmt)) {
                        summary.pixFmt = pixFmt;
                    }

                    if (summary.bitDepth == 0) {
                        const int bd = bitsPerRawSample.toInt();
                        if (bd > 0) {
                            summary.bitDepth = bd;
                        }
                    }
                    if (summary.bitDepth == 0 && !summary.pixFmt.isEmpty()) {
                        summary.bitDepth = inferBitDepthFromPixFmt(summary.pixFmt);
                    }
                }
                continue;
            }

            // Default format: ffprobe -of default=noprint_wrappers=0
            if (line == "[STREAM]") {
                inStreamSection = true;
                ffprobeStreamFields.clear();
                continue;
            }

            if (line == "[/STREAM]") {
                if (inStreamSection) {
                    if (summary.codec.isEmpty()) {
                        const QString codecName = ffprobeStreamFields.value("codec_name");
                        if (!codecName.isEmpty()) {
                            summary.codec = codecName.toUpper();
                        }
                    }

                    if (summary.resolution.isEmpty()) {
                        const QString width = ffprobeStreamFields.value("width");
                        const QString height = ffprobeStreamFields.value("height");
                        if (!width.isEmpty() && !height.isEmpty()) {
                            summary.resolution = width + "x" + height;
                        }
                    }

                    // Enhanced metadata (F8)
                    if (summary.durationSeconds <= 0.0) {
                        const double d = ffprobeStreamFields.value("duration").toDouble();
                        if (d > 0.0) { summary.durationSeconds = d; }
                    }
                    if (summary.colorSpace.isEmpty()) {
                        const QString cs = ffprobeStreamFields.value("color_space");
                        if (isMeaningfulMetadataValue(cs)) {
                            summary.colorSpace = cs;
                        }
                    }
                    if (summary.bitDepth == 0) {
                        const int bd = ffprobeStreamFields.value("bits_per_raw_sample").toInt();
                        if (bd > 0) { summary.bitDepth = bd; }
                    }
                    if (summary.pixFmt.isEmpty()) {
                        const QString pf = ffprobeStreamFields.value("pix_fmt");
                        if (isMeaningfulMetadataValue(pf)) {
                            summary.pixFmt = pf;
                        }
                    }
                    if (summary.bitDepth == 0 && !summary.pixFmt.isEmpty()) {
                        summary.bitDepth = inferBitDepthFromPixFmt(summary.pixFmt);
                    }

                    if (summary.fpsText.isEmpty()) {
                        const QString avgFrameRate = ffprobeStreamFields.value("avg_frame_rate");
                        if (!avgFrameRate.isEmpty()) {
                            parseFpsFromFraction(avgFrameRate, summary.fpsValue, summary.fpsText);
                        }

                        if (summary.fpsText.isEmpty()) {
                            const QString rFrameRate = ffprobeStreamFields.value("r_frame_rate");
                            if (!rFrameRate.isEmpty()) {
                                parseFpsFromFraction(rFrameRate, summary.fpsValue, summary.fpsText);
                            }
                        }
                    }
                }

                inStreamSection = false;
                ffprobeStreamFields.clear();
                continue;
            }

            // Extract duration from showinfo stderr log line as fallback
            if (summary.durationSeconds <= 0.0 && line.contains("Duration:")) {
                const QRegularExpression durationLineRe("Duration:\\s*(\\d+):(\\d+):([0-9.]+)");
                const QRegularExpressionMatch dm = durationLineRe.match(line);
                if (dm.hasMatch()) {
                    summary.durationSeconds = dm.captured(1).toDouble() * 3600.0
                                             + dm.captured(2).toDouble() * 60.0
                                             + dm.captured(3).toDouble();
                }
            }

            if (inStreamSection) {
                const int splitPos = line.indexOf('=');
                if (splitPos > 0) {
                    const QString key = line.left(splitPos).trimmed();
                    const QString value = line.mid(splitPos + 1).trimmed();
                    ffprobeStreamFields.insert(key, value);
                }
                continue;
            }

            if (summary.codec.isEmpty()) {
                const QRegularExpressionMatch codecMatch = codecRegex.match(line);
                if (codecMatch.hasMatch()) {
                    summary.codec = codecMatch.captured(1).toUpper();
                }
            }

            if (summary.fpsText.isEmpty() && line.contains("frame_rate:")) {
                const QRegularExpressionMatch fpsMatch = fpsConfigRegex.match(line);
                if (fpsMatch.hasMatch()) {
                    summary.fpsValue = parseFps(fpsMatch.captured(1), fpsMatch.captured(2));
                    if (summary.fpsValue > 0.0) {
                        summary.fpsText = QString::number(summary.fpsValue, 'f', 3);
                        summary.fpsText = summary.fpsText + " fps";
                    }
                }
            }

            if (summary.fpsText.isEmpty() && line.contains(" fps")) {
                const QRegularExpressionMatch fpsSimpleMatch = fpsSimpleRegex.match(line);
                if (fpsSimpleMatch.hasMatch()) {
                    summary.fpsValue = fpsSimpleMatch.captured(1).toDouble();
                    if (summary.fpsValue > 0.0) {
                        summary.fpsText = QString::number(summary.fpsValue, 'f', 3);
                        summary.fpsText = summary.fpsText + " fps";
                    }
                }
            }

            if (summary.resolution.isEmpty() && line.contains(" s:")) {
                const QRegularExpressionMatch resolutionMatch = resolutionRegex.match(line);
                if (resolutionMatch.hasMatch()) {
                    summary.resolution = resolutionMatch.captured(1);
                }
            }

            if (!summary.codec.isEmpty() && !summary.resolution.isEmpty() && !summary.fpsText.isEmpty()) {
                break;
            }
        }
    }

    if (summary.codec.isEmpty()) {
        summary.codec = "Unknown";
    }
    if (summary.resolution.isEmpty()) {
        summary.resolution = "Unknown";
    }
    if (summary.fpsText.isEmpty()) {
        summary.fpsText = "Unknown";
    }

    for (const FrameInfo &frame : frames) {
        if (frame.type == QChar('I')) {
            ++summary.iCount;
        } else if (frame.type == QChar('P')) {
            ++summary.pCount;
        } else if (frame.type == QChar('B')) {
            ++summary.bCount;
        }
    }

    qint64 totalBytes = 0;
    double totalDurationSeconds = 0.0;
    for (const FrameInfo &frame : frames) {
        if (frame.sizeInBytes > 0) {
            totalBytes += frame.sizeInBytes;
        }
        if (frame.durationTime > 0.0) {
            totalDurationSeconds += frame.durationTime;
        }
    }

    if (totalDurationSeconds <= 0.0 && summary.fpsValue > 0.0 && !frames.isEmpty()) {
        totalDurationSeconds = static_cast<double>(frames.size()) / summary.fpsValue;
    }

    if (totalBytes > 0 && totalDurationSeconds > 0.0) {
        summary.averageBitrate = (static_cast<double>(totalBytes) * 8.0) / totalDurationSeconds / 1000.0;
    }

    // Use frame-derived duration as fallback for durationSeconds
    if (summary.durationSeconds <= 0.0 && totalDurationSeconds > 0.0) {
        summary.durationSeconds = totalDurationSeconds;
    }

    if (!gops.isEmpty()) {
        summary.minGopSize = gops.first().size;
        summary.maxGopSize = gops.first().size;
        int totalGopSize = 0;

        for (const GopSegment &gop : gops) {
            totalGopSize += gop.size;
            if (gop.size < summary.minGopSize) {
                summary.minGopSize = gop.size;
            }
            if (gop.size > summary.maxGopSize) {
                summary.maxGopSize = gop.size;
            }
        }

        summary.avgGopSize = static_cast<double>(totalGopSize) / static_cast<double>(gops.size());

        if (gops.size() > 1) {
            double totalFrameInterval = 0.0;
            double totalTimeInterval = 0.0;
            for (int i = 1; i < gops.size(); ++i) {
                totalFrameInterval += static_cast<double>(gops[i].startFrame - gops[i - 1].startFrame);
                totalTimeInterval += (gops[i].startTime - gops[i - 1].startTime);
            }
            const double divisor = static_cast<double>(gops.size() - 1);
            summary.avgGopIntervalFrames = totalFrameInterval / divisor;
            summary.avgGopIntervalSeconds = totalTimeInterval / divisor;
        }
    }

    return summary;
}
