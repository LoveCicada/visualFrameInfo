#include "SummaryPanel.h"

#include <QFormLayout>
#include <QGroupBox>
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
