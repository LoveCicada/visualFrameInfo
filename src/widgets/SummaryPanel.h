#ifndef SUMMARYPANEL_H
#define SUMMARYPANEL_H

#include <QGroupBox>
#include <QLabel>
#include <QToolButton>
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

    // More Info (F8)
    QToolButton *m_moreInfoToggle = nullptr;
    QGroupBox   *m_moreInfoBox = nullptr;
    QLabel *m_durationValue = nullptr;
    QLabel *m_bitrateValue = nullptr;
    QLabel *m_colorSpaceValue = nullptr;
    QLabel *m_bitDepthValue = nullptr;
    QLabel *m_pixFmtValue = nullptr;
};

#endif // SUMMARYPANEL_H
