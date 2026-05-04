#ifndef FRAMEINFO_H
#define FRAMEINFO_H

#include <QString>

struct FrameInfo
{
    int index = -1;
    qint64 pts = 0;
    double ptsTime = 0.0;
    qint64 duration = 0;
    double durationTime = 0.0;
    bool isKey = false;
    QChar type = QChar('?');
    int gopIndex = -1;
    int indexInGop = -1;
    QString rawLine;
    int sizeInBytes = 0; // Size of the frame in bytes
    double bitrate = 0.0; // Bitrate in kbps
};

#endif // FRAMEINFO_H
