#include "SummaryPanel.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QToolButton>
#include <QVBoxLayout>

SummaryPanel::SummaryPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *summaryBox = new QGroupBox("Summary", this);
    auto *summaryForm = new QFormLayout(summaryBox);

    m_totalFramesValue = new QLabel("-");
    m_ipbValue = new QLabel("-");
    m_gopCountValue = new QLabel("-");
    m_avgGopValue = new QLabel("-");
    m_minMaxGopValue = new QLabel("-");
    m_intervalValue = new QLabel("-");

    m_totalFramesValue->setStyleSheet("font-weight: 600;");
    m_ipbValue->setStyleSheet("font-weight: 600;");
    m_gopCountValue->setStyleSheet("font-weight: 600;");

    summaryForm->addRow("Total frames:", m_totalFramesValue);
    summaryForm->addRow("I / P / B:", m_ipbValue);
    summaryForm->addRow("GOP count:", m_gopCountValue);
    summaryForm->addRow("Avg GOP size:", m_avgGopValue);
    summaryForm->addRow("Min / Max GOP:", m_minMaxGopValue);
    summaryForm->addRow("GOP interval:", m_intervalValue);

    auto *selectedBox = new QGroupBox("Selection", this);
    auto *selectedForm = new QFormLayout(selectedBox);

    m_selectedFrameValue = new QLabel("-");
    m_selectedGopValue = new QLabel("-");
    m_selectedFrameValue->setWordWrap(true);
    m_selectedGopValue->setWordWrap(true);

    selectedForm->addRow("Frame:", m_selectedFrameValue);
    selectedForm->addRow("GOP:", m_selectedGopValue);

    layout->addWidget(summaryBox);
    layout->addWidget(selectedBox);

    // More Info (F8) - collapsible
    m_moreInfoToggle = new QToolButton(this);
    m_moreInfoToggle->setText("More Info v");
    m_moreInfoToggle->setCheckable(true);
    m_moreInfoToggle->setChecked(false);
    m_moreInfoToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_moreInfoToggle->setToolButtonStyle(Qt::ToolButtonTextOnly);
    layout->addWidget(m_moreInfoToggle);

    m_moreInfoBox = new QGroupBox(this);
    auto *moreForm = new QFormLayout(m_moreInfoBox);
    m_durationValue   = new QLabel("-");
    m_bitrateValue    = new QLabel("-");
    m_colorSpaceValue = new QLabel("-");
    m_bitDepthValue   = new QLabel("-");
    m_pixFmtValue     = new QLabel("-");
    moreForm->addRow("Duration:",     m_durationValue);
    moreForm->addRow("Avg bitrate:",  m_bitrateValue);
    moreForm->addRow("Color space:",  m_colorSpaceValue);
    moreForm->addRow("Bit depth:",    m_bitDepthValue);
    moreForm->addRow("Pixel format:", m_pixFmtValue);
    m_moreInfoBox->setVisible(false);
    layout->addWidget(m_moreInfoBox);

    connect(m_moreInfoToggle, &QToolButton::toggled, this, [this](bool checked) {
        m_moreInfoBox->setVisible(checked);
        m_moreInfoToggle->setText(checked ? "More Info ^" : "More Info v");
    });

    layout->addStretch();
}

void SummaryPanel::setSummary(const AnalysisSummary &summary)
{
    m_totalFramesValue->setText(QString::number(summary.totalFrames));
    m_ipbValue->setText(QString("%1 / %2 / %3").arg(summary.iCount).arg(summary.pCount).arg(summary.bCount));
    m_gopCountValue->setText(QString::number(summary.gopCount));
    m_avgGopValue->setText(QString::number(summary.avgGopSize, 'f', 2));
    m_minMaxGopValue->setText(QString("%1 / %2").arg(summary.minGopSize).arg(summary.maxGopSize));
    m_intervalValue->setText(QString("%1 frames, %2 s")
                                 .arg(summary.avgGopIntervalFrames, 0, 'f', 2)
                                 .arg(summary.avgGopIntervalSeconds, 0, 'f', 3));

    // More Info (F8)
    if (summary.durationSeconds > 0.0) {
        const int h = static_cast<int>(summary.durationSeconds) / 3600;
        const int m = (static_cast<int>(summary.durationSeconds) % 3600) / 60;
        const double s = summary.durationSeconds - h * 3600 - m * 60;
        m_durationValue->setText(QString("%1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 6, 'f', 3, QChar('0')));
    } else {
        m_durationValue->setText("-");
    }
    m_bitrateValue->setText(summary.averageBitrate > 0.0
        ? QString("%1 kbps").arg(summary.averageBitrate, 0, 'f', 1) : "-");
    m_colorSpaceValue->setText(summary.colorSpace.isEmpty() ? "-" : summary.colorSpace);
    m_bitDepthValue->setText(summary.bitDepth > 0 ? QString::number(summary.bitDepth) + " bit" : "-");
    m_pixFmtValue->setText(summary.pixFmt.isEmpty() ? "-" : summary.pixFmt.toUpper());
}

void SummaryPanel::clearSummary()
{
    m_totalFramesValue->setText("-");
    m_ipbValue->setText("-");
    m_gopCountValue->setText("-");
    m_avgGopValue->setText("-");
    m_minMaxGopValue->setText("-");
    m_intervalValue->setText("-");
    m_selectedFrameValue->setText("-");
    m_selectedGopValue->setText("-");
    m_durationValue->setText("-");
    m_bitrateValue->setText("-");
    m_colorSpaceValue->setText("-");
    m_bitDepthValue->setText("-");
    m_pixFmtValue->setText("-");
}

void SummaryPanel::setSelectedFrame(const FrameInfo *frame, const GopSegment *gop)
{
    if (!frame) {
        m_selectedFrameValue->setText("-");
        m_selectedGopValue->setText("-");
        return;
    }

    m_selectedFrameValue->setText(QString("n=%1, t=%2, type=%3, key=%4")
                                      .arg(frame->index)
                                      .arg(frame->ptsTime, 0, 'f', 4)
                                      .arg(frame->type)
                                      .arg(frame->isKey ? "Y" : "N"));

    if (!gop) {
        m_selectedGopValue->setText("-");
        return;
    }

    m_selectedGopValue->setText(QString("#%1, [%2 - %3], size=%4")
                                    .arg(gop->gopIndex + 1)
                                    .arg(gop->startFrame)
                                    .arg(gop->endFrame)
                                    .arg(gop->size));
}
