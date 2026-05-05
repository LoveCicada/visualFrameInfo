#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QLabel>
#include <QKeyEvent>
#include <QMainWindow>
#include <QPushButton>
#include <QVector>

#include "models/AnalysisSummary.h"
#include "models/FrameInfo.h"
#include "models/GopSegment.h"

class FrameTableView;
class PreviewFrameWidget;
class QComboBox;
class QLineEdit;
class QProgressBar;
class QSlider;
class QScrollArea;
class QSpinBox;
class SummaryPanel;
class TimelineView;
class TimelineOverviewBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void chooseVideoFile();
    void chooseLogFile();
    void startAnalysis();
    void runBenchmark();
    void exportFrameCsv();
    void exportAnalysisReport();
    void openLogFile();
    void onFrameSelected(int frameIndex);
    void jumpToFrame();
    void jumpToGop();
    void selectPreviousFrame();
    void selectNextFrame();
    void selectPreviousIFrame();
    void selectNextIFrame();
    void applyZoomFactor(double newZoom, int anchorFrameIndex = -1);
    void syncOverviewRange();
    void onPreviewToggled(bool enabled);

private:
    void setupUi();
    void updateSourceInfo();
    void clearResultViews();
    bool loadAnalysisLog(const QString &logPath, const QString &sourceVideoPath = QString());
    int currentSelectedFrameIndex() const;
    void selectFrameByOffset(int offset);
    void selectAdjacentIFrame(int direction);
    const FrameInfo *findFrame(int frameIndex) const;
    const GopSegment *findGop(int gopIndex) const;
    void updatePreviewToggleState();
    void updateFramePreview(const FrameInfo *frame);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QLineEdit *m_videoPathEdit = nullptr;
    QPushButton *m_analyzeButton = nullptr;
    QPushButton *m_benchmarkButton = nullptr;
    QPushButton *m_openAnalysisLogButton = nullptr;
    QPushButton *m_openLogButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_zoomPercentLabel = nullptr;
    QComboBox *m_densityModeCombo = nullptr;
    QSpinBox *m_jumpFrameSpin = nullptr;
    QSpinBox *m_jumpGopSpin = nullptr;
    QSlider *m_zoomSlider = nullptr;
    QProgressBar *m_progressBar = nullptr;

    QLabel *m_fileNameValue = nullptr;
    QLabel *m_codecValue = nullptr;
    QLabel *m_resolutionValue = nullptr;
    QLabel *m_fpsValue = nullptr;
    QLabel *m_logPathValue = nullptr;
    QLabel *m_bitrateValue = nullptr;

    QScrollArea *m_timelineScrollArea = nullptr;
    TimelineView *m_timelineView = nullptr;
    TimelineOverviewBar *m_overviewBar = nullptr;
    SummaryPanel *m_summaryPanel = nullptr;
    FrameTableView *m_frameTableView = nullptr;
    QCheckBox *m_previewToggle = nullptr;
    PreviewFrameWidget *m_previewWidget = nullptr;
    int m_previewRequestSeq = 0;

    QString m_videoPath;
    QString m_logPath;
    QVector<FrameInfo> m_frames;
    QVector<GopSegment> m_gops;
    AnalysisSummary m_summary;
    double m_timelineZoom = 1.0;
    bool m_analysisInProgress = false;
};

#endif // MAINWINDOW_H
