#ifndef ANALYSISSUMMARY_H
#define ANALYSISSUMMARY_H

#include <QString>

struct AnalysisSummary
{
    QString filePath;
    QString logPath;
    QString codec;
    QString resolution;
    QString fpsText;
    double fpsValue = 0.0;
    int totalFrames = 0;
    int iCount = 0;
    int pCount = 0;
    int bCount = 0;
    int gopCount = 0;
    int minGopSize = 0;
    int maxGopSize = 0;
    double avgGopSize = 0.0;
    double avgGopIntervalFrames = 0.0;
    double avgGopIntervalSeconds = 0.0;
    double averageBitrate = 0.0; // Average bitrate of the video
};

#endif // ANALYSISSUMMARY_H
