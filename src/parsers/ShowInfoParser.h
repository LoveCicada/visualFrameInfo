#ifndef SHOWINFOPARSER_H
#define SHOWINFOPARSER_H

#include <QString>
#include <QVector>

#include "../models/AnalysisSummary.h"
#include "../models/FrameInfo.h"
#include "../models/GopSegment.h"

class ShowInfoParser
{
public:
    static bool parseFile(const QString &logPath, QVector<FrameInfo> &frames, QString &errorMessage);
    static QVector<GopSegment> buildGops(QVector<FrameInfo> &frames);
    static AnalysisSummary buildSummary(const QString &videoPath,
                                        const QString &logPath,
                                        const QVector<FrameInfo> &frames,
                                        const QVector<GopSegment> &gops);
};

#endif // SHOWINFOPARSER_H
