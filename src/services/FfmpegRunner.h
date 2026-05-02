#ifndef FFMPEGRUNNER_H
#define FFMPEGRUNNER_H

#include <functional>
#include <QString>

class FfmpegRunner
{
public:
    static QString resolveFfmpegPath();
    static QString buildLogPath(const QString &videoPath);
    static bool runShowInfo(const QString &videoPath, QString &logPath, QString &errorMessage,
                            std::function<void(int)> progressCallback = nullptr);
    static bool runBenchmarkComparison(const QString &videoPath,
                                       QString &reportPath,
                                       QString &summaryLine,
                                       QString &errorMessage);

    // Extracts a single frame at the given PTS time from videoPath.
    // Returns true and sets outputPath on success. Uses a file cache keyed by
    // videoPath + frameIndex to avoid repeated ffmpeg invocations.
    static bool captureFrame(const QString &videoPath,
                             double ptsTime,
                             int frameIndex,
                             QString &outputPath,
                             QString &errorMessage);
};

#endif // FFMPEGRUNNER_H
