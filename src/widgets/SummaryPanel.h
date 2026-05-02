#ifndef SUMMARYPANEL_H
#define SUMMARYPANEL_H

#include <QLabel>
#include <QWidget>

#include "../models/AnalysisSummary.h"
#include "../models/FrameInfo.h"
#include "../models/GopSegment.h"

class SummaryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SummaryPanel(QWidget *parent = nullptr);

    void setSummary(const AnalysisSummary &summary);
    void clearSummary();
    void setSelectedFrame(const FrameInfo *frame, const GopSegment *gop);

private:
    QLabel *m_totalFramesValue = nullptr;
    QLabel *m_ipbValue = nullptr;
    QLabel *m_gopCountValue = nullptr;
    QLabel *m_avgGopValue = nullptr;
    QLabel *m_minMaxGopValue = nullptr;
    QLabel *m_intervalValue = nullptr;
    QLabel *m_selectedFrameValue = nullptr;
    QLabel *m_selectedGopValue = nullptr;
};

#endif // SUMMARYPANEL_H
