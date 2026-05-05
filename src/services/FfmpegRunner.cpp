#include "FfmpegRunner.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTextStream>

namespace {
double parseTimeToSeconds(const QString &s)
{
    // Accepts HH:MM:SS.xx
    const QStringList parts = s.split(':');
    if (parts.size() != 3) {
        return 0.0;
    }
    return parts[0].toDouble() * 3600.0 +
           parts[1].toDouble() * 60.0 +
           parts[2].toDouble();
}

QString resolveBundledToolPath(const QString &toolName)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString currentDir = QDir::currentPath();
    const int maxLevels = 8;

    QSet<QString> visited;

    auto searchUpwards = [&](const QString &startPath) -> QString {
        QDir dir(startPath);
        for (int i = 0; i < maxLevels; ++i) {
            const QString candidate = QFileInfo(dir.filePath(QString("ffmpeg/%1").arg(toolName))).absoluteFilePath();
            if (!visited.contains(candidate) && QFileInfo::exists(candidate)) {
                return candidate;
            }
            visited.insert(candidate);

            if (!dir.cdUp()) {
                break;
            }
        }

        return QString();
    };

    const QString fromAppDir = searchUpwards(appDir);
    if (!fromAppDir.isEmpty()) {
        return fromAppDir;
    }

    return searchUpwards(currentDir);
}

QString resolveFfprobePath()
{
    return resolveBundledToolPath("ffprobe.exe");
}

QString benchmarkMetricsPath()
{
    const QString logDir = QDir(QCoreApplication::applicationDirPath()).filePath("logs");
    QDir().mkpath(logDir);
    return QDir(logDir).filePath("benchmark_metrics.tsv");
}

QString benchmarkDirPath()
{
    const QString path = QDir(QCoreApplication::applicationDirPath()).filePath("logs/benchmark");
    QDir().mkpath(path);
    return path;
}

QString benchmarkComparisonPath()
{
    return QDir(benchmarkDirPath()).filePath("benchmark_comparison.tsv");
}

QString sanitizeMetricField(const QString &text)
{
    QString sanitized = text;
    sanitized.replace('\t', ' ');
    sanitized.replace('\r', ' ');
    sanitized.replace('\n', ' ');
    return sanitized;
}

QString ffmpegThreadOptionValue()
{
    // Let ffmpeg/ffprobe auto-select a suitable thread count per codec/backend.
    return "0";
}

void appendBenchmarkMetric(const QString &backend,
                           const QString &videoPath,
                           const QString &analysisLogPath,
                           qint64 elapsedMs,
                           qint64 logBytes,
                           bool success,
                           const QString &errorMessage)
{
    const QString metricsPath = benchmarkMetricsPath();
    const bool needsHeader = !QFileInfo::exists(metricsPath);

    QFile file(metricsPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    if (needsHeader) {
        out << "timestamp\tbackend\tsuccess\telapsed_ms\tlog_bytes\tvideo_path\tanalysis_log\terror\n";
    }

    out << QDateTime::currentDateTime().toString(Qt::ISODate) << '\t'
        << sanitizeMetricField(backend) << '\t'
        << (success ? "1" : "0") << '\t'
        << elapsedMs << '\t'
        << logBytes << '\t'
        << sanitizeMetricField(videoPath) << '\t'
        << sanitizeMetricField(analysisLogPath) << '\t'
        << sanitizeMetricField(errorMessage) << '\n';
}

void appendBenchmarkComparison(const QString &videoPath,
                               const QString &ffprobeLogPath,
                               bool ffprobeOk,
                               qint64 ffprobeMs,
                               qint64 ffprobeBytes,
                               const QString &ffprobeError,
                               const QString &ffmpegLogPath,
                               bool ffmpegOk,
                               qint64 ffmpegMs,
                               qint64 ffmpegBytes,
                               const QString &ffmpegError)
{
    const QString path = benchmarkComparisonPath();
    const bool needsHeader = !QFileInfo::exists(path);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    if (needsHeader) {
        out << "timestamp\tvideo_path\tffprobe_ok\tffprobe_ms\tffprobe_log_bytes\tffprobe_log\tffprobe_error"
               "\tffmpeg_showinfo_ok\tffmpeg_showinfo_ms\tffmpeg_showinfo_log_bytes\tffmpeg_showinfo_log\tffmpeg_showinfo_error"
               "\tspeedup_showinfo_over_ffprobe\tlog_ratio_showinfo_over_ffprobe\n";
    }

    QString speedup = "";
    if (ffprobeOk && ffmpegOk && ffprobeMs > 0) {
        speedup = QString::number(static_cast<double>(ffmpegMs) / static_cast<double>(ffprobeMs), 'f', 3);
    }

    QString logRatio = "";
    if (ffprobeOk && ffmpegOk && ffprobeBytes > 0) {
        logRatio = QString::number(static_cast<double>(ffmpegBytes) / static_cast<double>(ffprobeBytes), 'f', 3);
    }

    out << QDateTime::currentDateTime().toString(Qt::ISODate) << '\t'
        << sanitizeMetricField(videoPath) << '\t'
        << (ffprobeOk ? "1" : "0") << '\t'
        << ffprobeMs << '\t'
        << ffprobeBytes << '\t'
        << sanitizeMetricField(ffprobeLogPath) << '\t'
        << sanitizeMetricField(ffprobeError) << '\t'
        << (ffmpegOk ? "1" : "0") << '\t'
        << ffmpegMs << '\t'
        << ffmpegBytes << '\t'
        << sanitizeMetricField(ffmpegLogPath) << '\t'
        << sanitizeMetricField(ffmpegError) << '\t'
        << speedup << '\t'
        << logRatio << '\n';
}
}

QString FfmpegRunner::resolveFfmpegPath()
{
    return resolveBundledToolPath("ffmpeg.exe");
}

QString FfmpegRunner::buildLogPath(const QString &videoPath)
{
    const QString logDir = QDir(QCoreApplication::applicationDirPath()).filePath("logs");
    QDir().mkpath(logDir);

    QString videoPrefix = QFileInfo(videoPath).completeBaseName();
    if (videoPrefix.isEmpty()) {
        videoPrefix = "video";
    }

    const QString name = QString("%1_frameinfo_%2.log")
                             .arg(videoPrefix)
                             .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    return QDir(logDir).filePath(name);
}

bool FfmpegRunner::runShowInfo(const QString &videoPath, QString &logPath, QString &errorMessage,
                               std::function<void(int)> progressCallback)
{
    QElapsedTimer totalTimer;
    totalTimer.start();

    if (!QFileInfo::exists(videoPath)) {
        errorMessage = QString("Video file does not exist: %1").arg(videoPath);
        appendBenchmarkMetric("precheck", videoPath, QString(), totalTimer.elapsed(), 0, false, errorMessage);
        return false;
    }

    logPath = buildLogPath(videoPath);

    auto runAndCapture = [&](const QString &program,
                             const QStringList &arguments,
                             const bool parseFfmpegProgress,
                             QString &runError) -> bool {
        QFile logFile(logPath);
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            runError = QString("Failed to write log file: %1").arg(logPath);
            return false;
        }

        QProcess process;
        process.setProgram(program);
        process.setArguments(arguments);
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start();

        if (!process.waitForStarted()) {
            logFile.close();
            runError = QString("Failed to start process: %1").arg(program);
            return false;
        }

        const QRegularExpression durationRe("Duration:\\s*(\\d+:\\d+:[\\d.]+)");
        const QRegularExpression timeRe("time=(\\d+:\\d+:[\\d.]+)");
        double totalDuration = 0.0;

        auto handleChunk = [&](const QByteArray &chunk) -> bool {
            if (chunk.isEmpty()) {
                return true;
            }

            if (logFile.write(chunk) != chunk.size()) {
                runError = QString("Failed to write log file: %1").arg(logPath);
                return false;
            }

            if (!parseFfmpegProgress || !progressCallback) {
                return true;
            }

            const QString text = QString::fromUtf8(chunk);
            if (totalDuration <= 0.0) {
                const QRegularExpressionMatch dm = durationRe.match(text);
                if (dm.hasMatch()) {
                    totalDuration = parseTimeToSeconds(dm.captured(1));
                }
            }

            if (totalDuration > 0.0) {
                QRegularExpressionMatchIterator it = timeRe.globalMatch(text);
                double lastTime = -1.0;
                while (it.hasNext()) {
                    lastTime = parseTimeToSeconds(it.next().captured(1));
                }
                if (lastTime >= 0.0) {
                    const int pct = qBound(1, static_cast<int>(lastTime / totalDuration * 95.0), 95);
                    progressCallback(pct);
                }
            }

            return true;
        };

        while (!process.waitForFinished(150)) {
            if (!handleChunk(process.readAll())) {
                process.kill();
                process.waitForFinished();
                logFile.close();
                return false;
            }
        }

        if (!handleChunk(process.readAll())) {
            process.kill();
            process.waitForFinished();
            logFile.close();
            return false;
        }

        logFile.close();

        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
            runError = QString("Process exited with code %1. See log: %2")
                           .arg(process.exitCode())
                           .arg(logPath);
            return false;
        }

        return true;
    };

    const QString ffprobePath = resolveFfprobePath();
    QString ffprobeError;
    if (!ffprobePath.isEmpty()) {
        if (progressCallback) {
            progressCallback(5);
        }

        const QStringList ffprobeArgs = QStringList()
                                        << "-v" << "error"
                                        << "-threads" << ffmpegThreadOptionValue()
                                        << "-select_streams" << "v:0"
                                        << "-show_streams"
                                        << "-show_frames"
                                        << "-show_entries"
                                        << "stream=codec_name,width,height,avg_frame_rate,r_frame_rate,duration,color_space,bits_per_raw_sample,pix_fmt:frame=key_frame,pict_type,best_effort_timestamp,best_effort_timestamp_time,pkt_duration,pkt_duration_time,pkt_size"
                                        << "-of" << "csv"
                                        << videoPath;

        if (runAndCapture(ffprobePath, ffprobeArgs, false, ffprobeError)) {
            if (progressCallback) {
                progressCallback(100);
            }
            appendBenchmarkMetric("ffprobe", videoPath, logPath, totalTimer.elapsed(), QFileInfo(logPath).size(), true, QString());
            return true;
        }
    }

    const QString ffmpegPath = resolveFfmpegPath();
    if (ffmpegPath.isEmpty()) {
        errorMessage = "ffmpeg.exe not found under ./ffmpeg.";
        appendBenchmarkMetric("ffmpeg", videoPath, logPath, totalTimer.elapsed(), 0, false, errorMessage);
        return false;
    }

    QString ffmpegError;
    const QStringList ffmpegArgs = QStringList()
                                   << "-hide_banner"
                                   << "-nostats"
                                   << "-threads" << ffmpegThreadOptionValue()
                                   << "-i" << videoPath
                                   << "-map" << "0:v:0"
                                   << "-an"
                                   << "-sn"
                                   << "-dn"
                                   << "-vf" << "showinfo=checksum=0"
                                   << "-f" << "null" << "-";

    if (runAndCapture(ffmpegPath, ffmpegArgs, true, ffmpegError)) {
        if (progressCallback) {
            progressCallback(100);
        }
        appendBenchmarkMetric("ffmpeg", videoPath, logPath, totalTimer.elapsed(), QFileInfo(logPath).size(), true, QString());
        return true;
    }

    if (!ffprobeError.isEmpty()) {
        errorMessage = QString("ffprobe failed: %1\nffmpeg fallback failed: %2")
                           .arg(ffprobeError)
                           .arg(ffmpegError);
    } else {
        errorMessage = ffmpegError;
    }
    appendBenchmarkMetric("ffprobe+ffmpeg", videoPath, logPath, totalTimer.elapsed(), QFileInfo(logPath).size(), false, errorMessage);
    return false;
}

bool FfmpegRunner::runBenchmarkComparison(const QString &videoPath,
                                          QString &reportPath,
                                          QString &summaryLine,
                                          QString &errorMessage)
{
    reportPath = benchmarkComparisonPath();
    summaryLine.clear();

    if (!QFileInfo::exists(videoPath)) {
        errorMessage = QString("Video file does not exist: %1").arg(videoPath);
        return false;
    }

    struct RunResult {
        bool ok = false;
        qint64 elapsedMs = 0;
        qint64 logBytes = 0;
        QString logPath;
        QString error;
    };

    const QString benchmarkDir = benchmarkDirPath();
    QString baseName = QFileInfo(videoPath).completeBaseName();
    if (baseName.isEmpty()) {
        baseName = "video";
    }
    const QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

    auto runCommand = [&](const QString &program,
                          const QStringList &args,
                          const QString &logPath) -> RunResult {
        RunResult result;
        result.logPath = logPath;

        QElapsedTimer timer;
        timer.start();

        QFile logFile(logPath);
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            result.error = QString("Failed to write log file: %1").arg(logPath);
            return result;
        }

        QProcess process;
        process.setProgram(program);
        process.setArguments(args);
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start();

        if (!process.waitForStarted()) {
            logFile.close();
            result.error = QString("Failed to start process: %1").arg(program);
            return result;
        }

        while (!process.waitForFinished(150)) {
            const QByteArray chunk = process.readAll();
            if (!chunk.isEmpty() && logFile.write(chunk) != chunk.size()) {
                process.kill();
                process.waitForFinished();
                logFile.close();
                result.error = QString("Failed to write log file: %1").arg(logPath);
                return result;
            }
        }

        const QByteArray tail = process.readAll();
        if (!tail.isEmpty() && logFile.write(tail) != tail.size()) {
            logFile.close();
            result.error = QString("Failed to write log file: %1").arg(logPath);
            return result;
        }

        logFile.close();

        result.elapsedMs = timer.elapsed();
        result.logBytes = QFileInfo(logPath).size();

        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
            result.error = QString("Process exited with code %1. See log: %2")
                               .arg(process.exitCode())
                               .arg(logPath);
            return result;
        }

        result.ok = true;
        return result;
    };

    RunResult ffprobeResult;
    RunResult ffmpegResult;

    const QString ffprobePath = resolveFfprobePath();
    ffprobeResult.logPath = QDir(benchmarkDir).filePath(QString("%1_%2_ffprobe.log").arg(baseName, ts));
    if (ffprobePath.isEmpty()) {
        ffprobeResult.error = "ffprobe.exe not found under ./ffmpeg.";
    } else {
        const QStringList ffprobeArgs = QStringList()
                                        << "-v" << "error"
                                        << "-threads" << ffmpegThreadOptionValue()
                                        << "-select_streams" << "v:0"
                                        << "-show_streams"
                                        << "-show_frames"
                                        << "-show_entries"
                                        << "stream=codec_name,width,height,avg_frame_rate,r_frame_rate:frame=key_frame,pict_type,best_effort_timestamp,best_effort_timestamp_time,pkt_duration,pkt_duration_time"
                                        << "-of" << "default=noprint_wrappers=0"
                                        << videoPath;
        ffprobeResult = runCommand(ffprobePath, ffprobeArgs, ffprobeResult.logPath);
    }

    const QString ffmpegPath = resolveFfmpegPath();
    ffmpegResult.logPath = QDir(benchmarkDir).filePath(QString("%1_%2_showinfo.log").arg(baseName, ts));
    if (ffmpegPath.isEmpty()) {
        ffmpegResult.error = "ffmpeg.exe not found under ./ffmpeg.";
    } else {
        const QStringList ffmpegArgs = QStringList()
                                       << "-hide_banner"
                                       << "-nostats"
                                       << "-threads" << ffmpegThreadOptionValue()
                                       << "-i" << videoPath
                                       << "-map" << "0:v:0"
                                       << "-an"
                                       << "-sn"
                                       << "-dn"
                                       << "-vf" << "showinfo=checksum=0"
                                       << "-f" << "null" << "-";
        ffmpegResult = runCommand(ffmpegPath, ffmpegArgs, ffmpegResult.logPath);
    }

    appendBenchmarkComparison(videoPath,
                              ffprobeResult.logPath,
                              ffprobeResult.ok,
                              ffprobeResult.elapsedMs,
                              ffprobeResult.logBytes,
                              ffprobeResult.error,
                              ffmpegResult.logPath,
                              ffmpegResult.ok,
                              ffmpegResult.elapsedMs,
                              ffmpegResult.logBytes,
                              ffmpegResult.error);

    if (ffprobeResult.ok && ffmpegResult.ok) {
        const double ffprobeMs = static_cast<double>(ffprobeResult.elapsedMs);
        const double ffmpegMs = static_cast<double>(ffmpegResult.elapsedMs);
        const double ffprobeBytes = static_cast<double>(ffprobeResult.logBytes);
        const double ffmpegBytes = static_cast<double>(ffmpegResult.logBytes);

        if (ffprobeMs > 0.0 && ffprobeBytes > 0.0) {
            const double speedRatio = ffmpegMs / ffprobeMs;
            const double logRatio = ffmpegBytes / ffprobeBytes;

            if (speedRatio >= 1.0) {
                summaryLine = QString("Summary: ffprobe is %1x faster than ffmpeg showinfo; showinfo log is %2x larger.")
                                  .arg(QString::number(speedRatio, 'f', 3))
                                  .arg(QString::number(logRatio, 'f', 3));
            } else {
                const double reverseSpeed = ffprobeMs / qMax(ffmpegMs, 1.0);
                summaryLine = QString("Summary: ffmpeg showinfo is %1x faster than ffprobe; showinfo log is %2x larger.")
                                  .arg(QString::number(reverseSpeed, 'f', 3))
                                  .arg(QString::number(logRatio, 'f', 3));
            }
        }
    }

    if (ffprobeResult.ok && ffmpegResult.ok) {
        return true;
    }

    errorMessage = QString("Benchmark failed. ffprobe: %1; ffmpeg showinfo: %2")
                       .arg(ffprobeResult.ok ? QString("ok") : ffprobeResult.error)
                       .arg(ffmpegResult.ok ? QString("ok") : ffmpegResult.error);
    return false;
}

bool FfmpegRunner::captureFrame(const QString &videoPath,
                                double ptsTime,
                                int frameIndex,
                                QString &outputPath,
                                QString &errorMessage)
{
    const QString ffmpegPath = resolveFfmpegPath();
    if (ffmpegPath.isEmpty()) {
        errorMessage = "ffmpeg.exe not found under ./ffmpeg.";
        return false;
    }

    // Cache directory: <appDir>/logs/preview_cache/
    const QString cacheDir =
        QDir(QCoreApplication::applicationDirPath()).filePath("logs/preview_cache");
    QDir().mkpath(cacheDir);

    // Cache key: <videoBaseName>_f<frameIndex>.jpg
    const QString baseName = QFileInfo(videoPath).completeBaseName();
    outputPath = QDir(cacheDir).filePath(
        QString("%1_f%2.jpg").arg(baseName.isEmpty() ? "video" : baseName).arg(frameIndex));

    // Return cached result without re-running ffmpeg
    if (QFileInfo::exists(outputPath)) {
        return true;
    }

    // ffmpeg -ss <pts> -i <video> -frames:v 1 -q:v 2 -y <output>
    QProcess process;
    process.setProgram(ffmpegPath);
    process.setArguments(QStringList()
                         << "-ss" << QString::number(ptsTime, 'f', 6)
                         << "-i"  << videoPath
                         << "-frames:v" << "1"
                         << "-q:v"      << "2"
                         << "-y"
                         << outputPath);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start();

    if (!process.waitForStarted(5000)) {
        errorMessage = "Failed to start ffmpeg for frame capture.";
        return false;
    }

    if (!process.waitForFinished(15000)) {
        process.kill();
        process.waitForFinished();
        errorMessage = "Frame capture timed out.";
        return false;
    }

    if (!QFileInfo::exists(outputPath)) {
        errorMessage = QString("Frame capture produced no output (exit code %1).")
                           .arg(process.exitCode());
        return false;
    }

    return true;
}
