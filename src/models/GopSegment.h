#ifndef GOPSEGMENT_H
#define GOPSEGMENT_H

struct GopSegment
{
    int gopIndex = -1;
    int startFrame = -1;
    int endFrame = -1;
    int size = 0;
    double startTime = 0.0;
    double endTime = 0.0;
};

#endif // GOPSEGMENT_H
