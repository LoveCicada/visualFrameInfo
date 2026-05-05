#include "MainWindow.h"

#include <QDesktopServices>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QGroupBox>
#include <QHeaderView>
#include <QPointer>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenuBar>
#include <QProgressBar>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTextStream>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMenu>
#include <QMimeData>
#include <QSettings>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include "parsers/ShowInfoParser.h"
#include "services/FfmpegRunner.h"
#include "widgets/FrameTableView.h"
#include "widgets/PreviewFrameWidget.h"
#include "widgets/GopHistogramWidget.h"
#include "widgets/SummaryPanel.h"
#include "widgets/TimelineOverviewBar.h"
#include "widgets/TimelineView.h"

namespace {
QString wrapLongText(const QString &text, int maxCharsPerLine)
{
    if (text.isEmpty() || maxCharsPerLine <= 0 || text.size() <= maxCharsPerLine) {
        return text;
    }

    QString wrapped;
    wrapped.reserve(text.size() + (text.size() / maxCharsPerLine) + 1);
    for (int i = 0; i < text.size(); ++i) {
        if (i > 0 && (i % maxCharsPerLine) == 0) {
            wrapped.append('\n');
        }
        wrapped.append(text.at(i));
    }
    return wrapped;
}

struct AnalysisTaskResult {
    bool ok = false;
    QString errorMessage;
    QString videoPath;
    QString logPath;
    QVector<FrameInfo> frames;
    QVector<GopSegment> gops;
    AnalysisSummary summary;
};

struct BenchmarkTaskResult {
    bool ok = false;
    QString reportPath;
    QString summaryLine;
    QString errorMessage;
};

struct BatchTaskResult {
    QVector<AnalysisSummary> summaries;
    QVector<QString> errors;
};

struct FrameCaptureResult {
    bool ok = false;
    QString outputPath;
    QString errorMessage;
    int requestSeq = 0;
};

QString csvEscape(const QString &value)
{
    QString escaped = value;
    escaped.replace('"', "\"\"");
    if (escaped.contains(',') || escaped.contains('"') || escaped.contains('\n') || escaped.contains('\r')) {
        escaped = '"' + escaped + '"';
    }
    return escaped;
}

QString buildRoundedUiStyleSheet()
{
    // ── Theme tokens ──────────────────────────────────────────────────────────
    // Adjust these to retheme the entire application in one place.
    const QString bg         = QStringLiteral("#edf1f7");  // window / panel bg
    const QString card       = QStringLiteral("#f7faff");  // elevated card surface
    const QString border     = QStringLiteral("#ccd6e6");  // default border
    const QString accent     = QStringLiteral("#5a8ddf");  // primary action colour
    const QString accentDk   = QStringLiteral("#4678c8");  // pressed / darker accent
    const QString text       = QStringLiteral("#1f2937");  // primary text

    // ── Derived tokens (computed from above; rarely need changing) ────────────
    const QString btnNorm    = QStringLiteral("#e8edf6");
    const QString btnHov     = QStringLiteral("#dbe5f6");
    const QString btnPrs     = QStringLiteral("#d1ddef");
    const QString borderInput= QStringLiteral("#c7d1e2");
    const QString borderFoc  = QStringLiteral("#6e99de");
    const QString textMuted  = QStringLiteral("#98a3b6");
    const QString textTitle  = QStringLiteral("#3f4d66");
    const QString textHead   = QStringLiteral("#334155");
    const QString accentLight= QStringLiteral("#9fb9e6");
    const QString scrollBg   = QStringLiteral("#edf2fa");
    const QString scrollHnd  = QStringLiteral("#b9c6db");
    const QString scrollHov  = QStringLiteral("#a9b9d2");

    return QString(
        // ── Base ──────────────────────────────────────────────────────────────
        "QMainWindow, QWidget {"
        "  background-color: %1; color: %2;"
        "  selection-background-color: #cfe0fb; selection-color: %2; }"

        // ── Buttons ───────────────────────────────────────────────────────────
        "QPushButton, QToolButton {"
        "  background-color: %3; border: 1px solid %4; border-radius: 8px;"
        "  padding: 4px 11px; min-height: 20px; }"
        "QPushButton:hover, QToolButton:hover { background-color: %5; }"
        "QPushButton:pressed, QToolButton:pressed { background-color: %6; }"
        "QPushButton:disabled, QToolButton:disabled {"
        "  color: %7; background-color: #f1f4fa; border-color: #d8dfea; }"

        // Capsule button group variants (assign via setProperty("capsule", "first/mid/last"))
        "QPushButton[capsule=\"first\"] {"
        "  border-top-right-radius: 0; border-bottom-right-radius: 0; border-right: none; }"
        "QPushButton[capsule=\"mid\"] {"
        "  border-radius: 0; border-right: none; }"
        "QPushButton[capsule=\"last\"] {"
        "  border-top-left-radius: 0; border-bottom-left-radius: 0; }"

        // Accent / primary button
        "QPushButton[accent=\"true\"] {"
        "  background-color: %8; color: #ffffff; border: 1px solid %9; font-weight: 600; }"
        "QPushButton[accent=\"true\"]:hover  { background-color: #4f84d9; }"
        "QPushButton[accent=\"true\"]:pressed { background-color: %9; }"
        "QPushButton[accent=\"true\"]:disabled {"
        "  background-color: %10; border-color: %10; color: #f3f7ff; }"

        // ── Inputs ────────────────────────────────────────────────────────────
        "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {"
        "  background-color: #ffffff; border: 1px solid %11; border-radius: 8px; padding: 3px 8px; }"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {"
        "  border: 1px solid %12; }"
        "QComboBox::drop-down { border: none; width: 18px; }"
        "QComboBox QAbstractItemView {"
        "  background-color: #ffffff; border: 1px solid %11; border-radius: 8px; outline: 0; padding: 4px; }"

        // ── GroupBox cards ────────────────────────────────────────────────────
        "QGroupBox {"
        "  border: 1px solid %4; border-radius: 12px; margin-top: 10px;"
        "  padding: 9px 7px 7px 7px; background-color: %13; }"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 10px; padding: 0 6px; color: %14; }"

        // Summary card – mirrors GroupBox visually (set via setObjectName("summaryCard"))
        "QWidget#summaryCard {"
        "  border: 1px solid %4; border-radius: 12px; background-color: %13; }"

        // ── Labels ────────────────────────────────────────────────────────────
        "QLabel { background: transparent; }"

        // ── Table ─────────────────────────────────────────────────────────────
        "QTableView, QTableWidget {"
        "  background-color: #ffffff; border: 1px solid %4; border-radius: 10px;"
        "  gridline-color: #e5ebf5; alternate-background-color: #f8fbff; }"
        "QHeaderView::section {"
        "  background-color: #eaf0fa; border: none; border-bottom: 1px solid #d4deee;"
        "  padding: 5px 6px; color: %15; }"

        // ── ProgressBar ───────────────────────────────────────────────────────
        "QProgressBar {"
        "  background-color: #e6ecf8; border: 1px solid #c6d1e3; border-radius: 8px;"
        "  text-align: center; color: #2f425f; }"
        "QProgressBar::chunk { background-color: %8; border-radius: 7px; }"

        // ── ScrollBar ─────────────────────────────────────────────────────────
        "QScrollBar:vertical, QScrollBar:horizontal {"
        "  background: %16; border: none; border-radius: 7px; margin: 2px; }"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal {"
        "  background: %17; border-radius: 6px; min-height: 24px; min-width: 24px; }"
        "QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {"
        "  background: %18; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0px; height: 0px; }"

        // ── CheckBox ──────────────────────────────────────────────────────────
        "QCheckBox::indicator {"
        "  width: 14px; height: 14px; border: 1px solid #aeb9cf;"
        "  border-radius: 5px; background: #ffffff; }"
        "QCheckBox::indicator:checked { background: %8; border-color: %8; }"

        // ── Menus ─────────────────────────────────────────────────────────────
        "QMenuBar, QMenu { background-color: #edf2fb; border: 1px solid #d4ddef; border-radius: 8px; }"
        "QMenuBar::item { padding: 4px 8px; border-radius: 6px; }"
        "QMenuBar::item:selected { background-color: #dfe8f8; }"
        "QMenu::item:selected { background-color: #dfe8f8; border-radius: 6px; }"

        // ── StatusBar / Splitter ──────────────────────────────────────────────
        "QStatusBar { background-color: #edf2fb; border-top: 1px solid #d4deee; }"
        "QSplitter::handle { background-color: #d8e0ee; border-radius: 3px; }"
        "QSplitter::handle:horizontal { width: 6px; }"
        "QSplitter::handle:vertical   { height: 6px; }"
    )
    .arg(bg, text, btnNorm, border, btnHov, btnPrs, textMuted)   // %1 – %7
    .arg(accent, accentDk, accentLight)                           // %8 – %10
    .arg(borderInput, borderFoc, card, textTitle, textHead)       // %11 – %15
    .arg(scrollBg, scrollHnd, scrollHov);                         // %16 – %18
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);
    setupUi();
    clearResultViews();
}

void MainWindow::chooseVideoFile()
{
    const QString path = QFileDialog::getOpenFileName(this,
                                                      "Select video file",
                                                      QString(),
                                                      "Videos (*.mp4 *.mov *.mkv *.avi *.mxf);;All files (*.*)");
    if (path.isEmpty()) {
        return;
    }

    m_videoPath = path;
    m_videoPathEdit->setText(path);
    m_videoPathEdit->setToolTip(path);
    updateSourceInfo();
}

void MainWindow::chooseLogFile()
{
    const QString path = QFileDialog::getOpenFileName(this,
                                                      "Select analysis log file",
                                                      QString(),
                                                      "Log Files (*.log *.txt);;All files (*.*)");
    if (path.isEmpty()) {
        return;
    }

    loadAnalysisLog(path);
}

void MainWindow::startAnalysis()
{
    const QString path = m_videoPathEdit->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Missing input", "Please select a video file first.");
        return;
    }

    if (m_analysisInProgress) {
        return;
    }

    m_videoPath = path;
    m_videoPathEdit->setToolTip(path);
    m_analysisInProgress = true;
    m_analyzeButton->setEnabled(false);
    m_benchmarkButton->setEnabled(false);
    m_openAnalysisLogButton->setEnabled(false);
    m_openLogButton->setEnabled(false);
    m_statusLabel->setText("Analyzing with ffmpeg...");
    statusBar()->showMessage("Running ffmpeg...");
    if (m_progressBar) {
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
    }

    auto *watcher = new QFutureWatcher<AnalysisTaskResult>(this);
    connect(watcher, &QFutureWatcher<AnalysisTaskResult>::finished, this, [this, watcher]() {
        const AnalysisTaskResult result = watcher->result();
        watcher->deleteLater();

        m_analysisInProgress = false;
        m_analyzeButton->setEnabled(true);
        m_benchmarkButton->setEnabled(true);
        m_openAnalysisLogButton->setEnabled(true);
        if (m_progressBar) {
            m_progressBar->setValue(100);
            m_progressBar->setVisible(false);
        }

        if (!result.ok) {
            if (!result.logPath.isEmpty()) {
                m_logPath = result.logPath;
                m_openLogButton->setEnabled(true);
                m_logPathValue->setText(m_logPath);
            }
            m_statusLabel->setText("Analyze failed");
            statusBar()->showMessage("Analyze failed", 5000);
            QMessageBox::critical(this, "Analyze failed", result.errorMessage);
            return;
        }

        m_videoPath = result.videoPath;
        m_logPath = result.logPath;
        m_frames = result.frames;
        m_gops = result.gops;
        m_summary = result.summary;

        m_timelineView->setData(m_frames, m_gops);
        m_overviewBar->setData(m_frames, m_gops);
        m_frameTableView->setFrames(m_frames);
        m_summaryPanel->setSummary(m_summary);
        m_gopHistogram->setGops(m_gops);

        if (!m_frames.isEmpty()) {
            m_jumpFrameSpin->setRange(m_frames.first().index, m_frames.last().index);
            m_jumpFrameSpin->setValue(m_frames.first().index);
            onFrameSelected(m_frames.first().index);
        }
        if (!m_gops.isEmpty()) {
            m_jumpGopSpin->setRange(1, m_gops.size());
            m_jumpGopSpin->setValue(1);
        }

        m_openLogButton->setEnabled(true);
        m_logPathValue->setText(m_logPath);
        addRecentFile(m_videoPath);
        updateSourceInfo();
        syncOverviewRange();
        updatePreviewToggleState();
        m_statusLabel->setText("Analyze complete");
        statusBar()->showMessage("Analyze complete", 4000);
    });

    QPointer<MainWindow> weakSelf = this;
    auto progressCallback = [weakSelf](int pct) {
        QMetaObject::invokeMethod(weakSelf, [weakSelf, pct]() {
            if (weakSelf && weakSelf->m_progressBar) {
                weakSelf->m_progressBar->setValue(pct);
            }
        });
    };

    watcher->setFuture(QtConcurrent::run([path, progressCallback]() {
        AnalysisTaskResult result;
        result.videoPath = path;

        QString runError;
        if (!FfmpegRunner::runShowInfo(path, result.logPath, runError, progressCallback)) {
            result.errorMessage = runError;
            return result;
        }

        QString parseError;
        if (!ShowInfoParser::parseFile(result.logPath, result.frames, parseError)) {
            result.errorMessage = parseError;
            return result;
        }

        result.gops = ShowInfoParser::buildGops(result.frames);
        result.summary = ShowInfoParser::buildSummary(result.videoPath, result.logPath, result.frames, result.gops);

        result.ok = true;
        return result;
    }));
}

void MainWindow::startBatchAnalysis()
{
    if (m_analysisInProgress) {
        return;
    }

    const QStringList paths = QFileDialog::getOpenFileNames(
        this,
        "Select video files for batch analysis",
        QString(),
        "Videos (*.mp4 *.mov *.mkv *.avi *.mxf);;All files (*.*)");
    if (paths.isEmpty()) {
        return;
    }

    m_analysisInProgress = true;
    m_batchSummaries.clear();
    m_batchErrors.clear();

    m_analyzeButton->setEnabled(false);
    m_benchmarkButton->setEnabled(false);
    m_openAnalysisLogButton->setEnabled(false);
    m_openLogButton->setEnabled(false);

    m_statusLabel->setText(QString("Batch analyzing %1 file(s)...").arg(paths.size()));
    statusBar()->showMessage(m_statusLabel->text());
    if (m_progressBar) {
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
    }

    auto *watcher = new QFutureWatcher<BatchTaskResult>(this);
    connect(watcher, &QFutureWatcher<BatchTaskResult>::finished, this, [this, watcher]() {
        const BatchTaskResult result = watcher->result();
        watcher->deleteLater();

        m_analysisInProgress = false;
        m_analyzeButton->setEnabled(true);
        m_benchmarkButton->setEnabled(true);
        m_openAnalysisLogButton->setEnabled(true);
        m_openLogButton->setEnabled(!m_logPath.isEmpty());

        if (m_progressBar) {
            m_progressBar->setValue(100);
            m_progressBar->setVisible(false);
        }

        m_batchSummaries = result.summaries;
        m_batchErrors = result.errors;

        int successCount = 0;
        for (const QString &err : m_batchErrors) {
            if (err.isEmpty()) {
                ++successCount;
            }
        }

        m_statusLabel->setText(QString("Batch done: %1/%2 success")
                                   .arg(successCount)
                                   .arg(m_batchSummaries.size()));
        statusBar()->showMessage(m_statusLabel->text(), 5000);
        showBatchSummaryDialog();
    });

    QPointer<MainWindow> weakSelf = this;
    watcher->setFuture(QtConcurrent::run([paths, weakSelf]() {
        BatchTaskResult result;
        result.summaries.reserve(paths.size());
        result.errors.reserve(paths.size());

        const int total = qMax(1, paths.size());
        for (int i = 0; i < paths.size(); ++i) {
            const QString path = paths.at(i);
            AnalysisSummary summary;
            summary.filePath = path;

            auto progressCallback = [weakSelf, i, total](int pct) {
                const int boundedPct = qBound(0, pct, 100);
                const int overall = qBound(0, (i * 100 + boundedPct) / total, 100);
                QMetaObject::invokeMethod(weakSelf, [weakSelf, overall]() {
                    if (weakSelf && weakSelf->m_progressBar) {
                        weakSelf->m_progressBar->setValue(overall);
                    }
                });
            };

            QString runError;
            QString logPath;
            if (!FfmpegRunner::runShowInfo(path, logPath, runError, progressCallback)) {
                summary.logPath = logPath;
                result.summaries.push_back(summary);
                result.errors.push_back(runError);
                continue;
            }

            QVector<FrameInfo> frames;
            QString parseError;
            if (!ShowInfoParser::parseFile(logPath, frames, parseError)) {
                summary.logPath = logPath;
                result.summaries.push_back(summary);
                result.errors.push_back(parseError);
                continue;
            }

            QVector<GopSegment> gops = ShowInfoParser::buildGops(frames);
            summary = ShowInfoParser::buildSummary(path, logPath, frames, gops);

            result.summaries.push_back(summary);
            result.errors.push_back(QString());

            QMetaObject::invokeMethod(weakSelf, [weakSelf, i, total]() {
                if (weakSelf && weakSelf->m_progressBar) {
                    weakSelf->m_progressBar->setValue(qBound(0, ((i + 1) * 100) / total, 100));
                }
            });
        }

        return result;
    }));
}

void MainWindow::runBenchmark()
{
    const QString path = m_videoPathEdit->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Missing input", "Please select a video file first.");
        return;
    }

    if (m_analysisInProgress) {
        return;
    }

    m_analysisInProgress = true;
    m_analyzeButton->setEnabled(false);
    m_benchmarkButton->setEnabled(false);
    m_openAnalysisLogButton->setEnabled(false);
    m_openLogButton->setEnabled(false);
    m_statusLabel->setText("Running benchmark...");
    statusBar()->showMessage("Benchmarking ffprobe vs ffmpeg showinfo...");
    if (m_progressBar) {
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 0);
    }

    auto *watcher = new QFutureWatcher<BenchmarkTaskResult>(this);
    connect(watcher, &QFutureWatcher<BenchmarkTaskResult>::finished, this, [this, watcher]() {
        const BenchmarkTaskResult result = watcher->result();
        watcher->deleteLater();

        m_analysisInProgress = false;
        m_analyzeButton->setEnabled(true);
        m_benchmarkButton->setEnabled(true);
        m_openAnalysisLogButton->setEnabled(true);
        m_openLogButton->setEnabled(!m_logPath.isEmpty());

        if (m_progressBar) {
            m_progressBar->setVisible(false);
            m_progressBar->setRange(0, 100);
            m_progressBar->setValue(0);
        }

        if (!result.ok) {
            m_statusLabel->setText("Benchmark failed");
            statusBar()->showMessage("Benchmark failed", 5000);
            QMessageBox::critical(this,
                                  "Benchmark failed",
                                  result.errorMessage + "\n\nReport: " + result.reportPath);
            return;
        }

        m_statusLabel->setText("Benchmark complete");
        statusBar()->showMessage("Benchmark complete", 4000);
        QMessageBox::information(this,
                                 "Benchmark complete",
                                 result.summaryLine + "\n\nComparison report updated:\n" + result.reportPath);
    });

    watcher->setFuture(QtConcurrent::run([path]() {
        BenchmarkTaskResult result;
        if (FfmpegRunner::runBenchmarkComparison(path,
                                                 result.reportPath,
                                                 result.summaryLine,
                                                 result.errorMessage)) {
            result.ok = true;
        }
        return result;
    }));
}

void MainWindow::showBatchSummaryDialog()
{
    if (m_batchSummaries.isEmpty()) {
        QMessageBox::information(this, "Batch Analysis", "No batch analysis result available.");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Batch Analysis Summary");
    dialog.resize(1100, 520);

    auto *layout = new QVBoxLayout(&dialog);
    auto *table = new QTableWidget(m_batchSummaries.size(), 12, &dialog);
    table->setHorizontalHeaderLabels({
        "Status", "File", "Codec", "Resolution", "FPS", "Duration(s)",
        "Bitrate(kbps)", "Frames", "GOPs", "Color Space", "Bit Depth", "Pixel Format"
    });
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);

    for (int i = 0; i < m_batchSummaries.size(); ++i) {
        const AnalysisSummary &s = m_batchSummaries.at(i);
        const QString err = (i < m_batchErrors.size()) ? m_batchErrors.at(i) : QString();
        const bool ok = err.isEmpty();

        auto setCell = [table, i](int col, const QString &text) {
            table->setItem(i, col, new QTableWidgetItem(text));
        };

        setCell(0, ok ? "OK" : "Failed");
        setCell(1, QFileInfo(s.filePath).fileName().isEmpty() ? s.filePath : QFileInfo(s.filePath).fileName());
        setCell(2, s.codec);
        setCell(3, s.resolution);
        setCell(4, s.fpsText);
        setCell(5, s.durationSeconds > 0.0 ? QString::number(s.durationSeconds, 'f', 3) : "-");
        setCell(6, s.averageBitrate > 0.0 ? QString::number(s.averageBitrate, 'f', 2) : "-");
        setCell(7, s.totalFrames > 0 ? QString::number(s.totalFrames) : "-");
        setCell(8, s.gopCount > 0 ? QString::number(s.gopCount) : "-");
        setCell(9, s.colorSpace.isEmpty() ? "-" : s.colorSpace);
        setCell(10, s.bitDepth > 0 ? QString::number(s.bitDepth) : "-");
        setCell(11, s.pixFmt.isEmpty() ? "-" : s.pixFmt.toUpper());

        if (!ok) {
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem *item = table->item(i, col);
                if (item) {
                    item->setToolTip(err);
                }
            }
        }
    }

    layout->addWidget(table);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto *exportButton = buttonBox->addButton("Export CSV...", QDialogButtonBox::ActionRole);
    connect(exportButton, &QPushButton::clicked, &dialog, [this, &dialog]() {
        const QString outputPath = QFileDialog::getSaveFileName(
            &dialog,
            "Export Batch Summary CSV",
            QDir::current().filePath("batch_analysis_summary.csv"),
            "CSV Files (*.csv)");
        if (outputPath.isEmpty()) {
            return;
        }
        if (exportBatchSummaryCsv(outputPath)) {
            QMessageBox::information(&dialog, "Export complete", "Batch CSV exported:\n" + outputPath);
        } else {
            QMessageBox::critical(&dialog, "Export failed", "Cannot write file:\n" + outputPath);
        }
    });
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

bool MainWindow::exportBatchSummaryCsv(const QString &outputPath) const
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    QTextStream out(&file);
    out << "status,file_path,log_path,codec,resolution,fps,duration_seconds,average_bitrate_kbps,total_frames,i_count,p_count,b_count,gop_count,min_gop,max_gop,avg_gop,color_space,bit_depth,pixel_format,error\n";

    for (int i = 0; i < m_batchSummaries.size(); ++i) {
        const AnalysisSummary &s = m_batchSummaries.at(i);
        const QString err = (i < m_batchErrors.size()) ? m_batchErrors.at(i) : QString();
        out << csvEscape(err.isEmpty() ? "OK" : "Failed") << ','
            << csvEscape(s.filePath) << ','
            << csvEscape(s.logPath) << ','
            << csvEscape(s.codec) << ','
            << csvEscape(s.resolution) << ','
            << csvEscape(s.fpsText) << ','
            << QString::number(s.durationSeconds, 'f', 3) << ','
            << QString::number(s.averageBitrate, 'f', 2) << ','
            << s.totalFrames << ','
            << s.iCount << ','
            << s.pCount << ','
            << s.bCount << ','
            << s.gopCount << ','
            << s.minGopSize << ','
            << s.maxGopSize << ','
            << QString::number(s.avgGopSize, 'f', 3) << ','
            << csvEscape(s.colorSpace) << ','
            << (s.bitDepth > 0 ? QString::number(s.bitDepth) : QString()) << ','
            << csvEscape(s.pixFmt.toUpper()) << ','
            << csvEscape(err)
            << '\n';
    }

    return true;
}

void MainWindow::exportFrameCsv()
{
    if (m_frames.isEmpty()) {
        QMessageBox::information(this, "No data", "No frame data is available. Please run analysis first.");
        return;
    }

    const QString basePath = !m_videoPath.isEmpty() ? m_videoPath : m_logPath;
    const QFileInfo baseInfo(basePath);
    const QString stem = baseInfo.completeBaseName().isEmpty() ? QString("analysis") : baseInfo.completeBaseName();
    const QString dirPath = baseInfo.absolutePath().isEmpty() ? QString() : baseInfo.absolutePath();
    const QString defaultPath = dirPath.isEmpty()
                                    ? QString("%1_frames.csv").arg(stem)
                                    : QDir(dirPath).filePath(QString("%1_frames.csv").arg(stem));

    const QString frameCsvPath = QFileDialog::getSaveFileName(this,
                                                              "Export Frame CSV",
                                                              defaultPath,
                                                              "CSV Files (*.csv)");
    if (frameCsvPath.isEmpty()) {
        return;
    }

    QFile frameFile(frameCsvPath);
    if (!frameFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Export failed", QString("Cannot write file:\n%1").arg(frameCsvPath));
        return;
    }

    QTextStream frameOut(&frameFile);
    frameOut << "index,pts_time,type,isKey,gopIndex,indexInGop,durationTime,pktSize\n";
    for (const FrameInfo &frame : m_frames) {
        frameOut << frame.index << ','
                 << QString::number(frame.ptsTime, 'f', 6) << ','
                 << frame.type << ','
                 << (frame.isKey ? 1 : 0) << ','
                 << frame.gopIndex << ','
                 << frame.indexInGop << ','
                 << QString::number(frame.durationTime, 'f', 6) << ','
                 << frame.sizeInBytes << '\n';
    }
    frameFile.close();

    QString exportedMessage = QString("Frame CSV exported:\n%1").arg(frameCsvPath);

    const QMessageBox::StandardButton exportGop = QMessageBox::question(
        this,
        "Export GOP Summary",
        "Do you also want to export GOP summary CSV?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (exportGop == QMessageBox::Yes && !m_gops.isEmpty()) {
        const QFileInfo frameCsvInfo(frameCsvPath);
        const QString gopPath = QDir(frameCsvInfo.absolutePath())
                                    .filePath(frameCsvInfo.completeBaseName() + "_gops.csv");

        QFile gopFile(gopPath);
        if (!gopFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QMessageBox::critical(this, "Export failed", QString("Cannot write file:\n%1").arg(gopPath));
            return;
        }

        QTextStream gopOut(&gopFile);
        gopOut << "gopIndex,startFrame,endFrame,size,startTime,endTime\n";
        for (const GopSegment &gop : m_gops) {
            gopOut << gop.gopIndex << ','
                   << gop.startFrame << ','
                   << gop.endFrame << ','
                   << gop.size << ','
                   << QString::number(gop.startTime, 'f', 6) << ','
                   << QString::number(gop.endTime, 'f', 6) << '\n';
        }
        gopFile.close();
        exportedMessage += QString("\nGOP CSV exported:\n%1").arg(gopPath);
    }

    QMessageBox::information(this, "Export complete", exportedMessage);
}

void MainWindow::exportAnalysisReport()
{
    if (m_frames.isEmpty()) {
        QMessageBox::information(this, "No data", "No analysis result is available. Please run analysis first.");
        return;
    }

    const QString basePath = !m_videoPath.isEmpty() ? m_videoPath : m_logPath;
    const QFileInfo baseInfo(basePath);
    const QString stem = baseInfo.completeBaseName().isEmpty() ? QString("analysis") : baseInfo.completeBaseName();
    const QString dirPath = baseInfo.absolutePath().isEmpty() ? QString() : baseInfo.absolutePath();
    const QString defaultPath = dirPath.isEmpty()
                                    ? QString("%1_report.txt").arg(stem)
                                    : QDir(dirPath).filePath(QString("%1_report.txt").arg(stem));

    const QString reportPath = QFileDialog::getSaveFileName(this,
                                                            "Export Analysis Report",
                                                            defaultPath,
                                                            "Text Report (*.txt);;HTML Report (*.html *.htm)");
    if (reportPath.isEmpty()) {
        return;
    }

    QString finalPath = reportPath;
    const QFileInfo selectedInfo(reportPath);
    QString suffix = selectedInfo.suffix().toLower();
    bool asHtml = (suffix == "html" || suffix == "htm");
    if (suffix.isEmpty()) {
        asHtml = false;
        finalPath += ".txt";
    }

    const int total = m_summary.totalFrames;
    const auto ratioOf = [total](int count) {
        if (total <= 0) {
            return 0.0;
        }
        return static_cast<double>(count) * 100.0 / static_cast<double>(total);
    };

    const QString generatedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    QString report;

    if (asHtml) {
        report += "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Analysis Report</title>";
        report += "<style>body{font-family:Segoe UI,Arial,sans-serif;line-height:1.5;padding:20px;}";
        report += "h1,h2{margin:0 0 10px 0;}table{border-collapse:collapse;margin:10px 0 20px 0;}";
        report += "th,td{border:1px solid #cccccc;padding:6px 10px;text-align:left;}";
        report += "th{background:#f5f5f5;}</style></head><body>";
        report += "<h1>visualFrameInfo Analysis Report</h1>";
        report += QString("<p><b>Generated:</b> %1</p>").arg(generatedAt.toHtmlEscaped());

        report += "<h2>Video Metadata</h2><table>";
        report += QString("<tr><th>Video Path</th><td>%1</td></tr>").arg(m_summary.filePath.toHtmlEscaped());
        report += QString("<tr><th>Log Path</th><td>%1</td></tr>").arg(m_summary.logPath.toHtmlEscaped());
        report += QString("<tr><th>Codec</th><td>%1</td></tr>").arg(m_summary.codec.toHtmlEscaped());
        report += QString("<tr><th>Resolution</th><td>%1</td></tr>").arg(m_summary.resolution.toHtmlEscaped());
        report += QString("<tr><th>FPS</th><td>%1</td></tr>").arg(m_summary.fpsText.toHtmlEscaped());
        report += QString("<tr><th>Average Bitrate (kbps)</th><td>%1</td></tr>")
                      .arg(QString::number(m_summary.averageBitrate, 'f', 2));
        report += "</table>";

        report += "<h2>Frame Type Statistics</h2><table>";
        report += QString("<tr><th>Total Frames</th><td>%1</td></tr>").arg(m_summary.totalFrames);
        report += QString("<tr><th>I Frames</th><td>%1 (%2%)</td></tr>")
                      .arg(m_summary.iCount)
                      .arg(QString::number(ratioOf(m_summary.iCount), 'f', 2));
        report += QString("<tr><th>P Frames</th><td>%1 (%2%)</td></tr>")
                      .arg(m_summary.pCount)
                      .arg(QString::number(ratioOf(m_summary.pCount), 'f', 2));
        report += QString("<tr><th>B Frames</th><td>%1 (%2%)</td></tr>")
                      .arg(m_summary.bCount)
                      .arg(QString::number(ratioOf(m_summary.bCount), 'f', 2));
        report += "</table>";

        report += "<h2>GOP Statistics</h2><table>";
        report += QString("<tr><th>GOP Count</th><td>%1</td></tr>").arg(m_summary.gopCount);
        report += QString("<tr><th>Min GOP Size</th><td>%1</td></tr>").arg(m_summary.minGopSize);
        report += QString("<tr><th>Max GOP Size</th><td>%1</td></tr>").arg(m_summary.maxGopSize);
        report += QString("<tr><th>Avg GOP Size</th><td>%1</td></tr>")
                      .arg(QString::number(m_summary.avgGopSize, 'f', 2));
        report += QString("<tr><th>Avg GOP Interval (frames)</th><td>%1</td></tr>")
                      .arg(QString::number(m_summary.avgGopIntervalFrames, 'f', 2));
        report += QString("<tr><th>Avg GOP Interval (seconds)</th><td>%1</td></tr>")
                      .arg(QString::number(m_summary.avgGopIntervalSeconds, 'f', 3));
        report += "</table></body></html>";
    } else {
        QTextStream out(&report);
        out << "visualFrameInfo Analysis Report\n";
        out << "Generated: " << generatedAt << "\n\n";

        out << "[Video Metadata]\n";
        out << "Video Path: " << m_summary.filePath << "\n";
        out << "Log Path: " << m_summary.logPath << "\n";
        out << "Codec: " << m_summary.codec << "\n";
        out << "Resolution: " << m_summary.resolution << "\n";
        out << "FPS: " << m_summary.fpsText << "\n";
        out << "Average Bitrate (kbps): " << QString::number(m_summary.averageBitrate, 'f', 2) << "\n\n";

        out << "[Frame Type Statistics]\n";
        out << "Total Frames: " << m_summary.totalFrames << "\n";
        out << "I Frames: " << m_summary.iCount << " (" << QString::number(ratioOf(m_summary.iCount), 'f', 2) << "%)\n";
        out << "P Frames: " << m_summary.pCount << " (" << QString::number(ratioOf(m_summary.pCount), 'f', 2) << "%)\n";
        out << "B Frames: " << m_summary.bCount << " (" << QString::number(ratioOf(m_summary.bCount), 'f', 2) << "%)\n\n";

        out << "[GOP Statistics]\n";
        out << "GOP Count: " << m_summary.gopCount << "\n";
        out << "Min GOP Size: " << m_summary.minGopSize << "\n";
        out << "Max GOP Size: " << m_summary.maxGopSize << "\n";
        out << "Avg GOP Size: " << QString::number(m_summary.avgGopSize, 'f', 2) << "\n";
        out << "Avg GOP Interval (frames): " << QString::number(m_summary.avgGopIntervalFrames, 'f', 2) << "\n";
        out << "Avg GOP Interval (seconds): " << QString::number(m_summary.avgGopIntervalSeconds, 'f', 3) << "\n";
    }

    QFile reportFile(finalPath);
    if (!reportFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Export failed", QString("Cannot write file:\n%1").arg(finalPath));
        return;
    }

    QTextStream reportOut(&reportFile);
    reportOut << report;
    reportFile.close();

    QMessageBox::information(this, "Export complete", QString("Report exported:\n%1").arg(finalPath));
}

void MainWindow::openLogFile()
{
    if (m_logPath.isEmpty()) {
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(m_logPath));
}

void MainWindow::onFrameSelected(int frameIndex)
{
    const FrameInfo *frame = findFrame(frameIndex);
    if (!frame) {
        return;
    }

    const GopSegment *gop = findGop(frame->gopIndex);
    if (m_jumpFrameSpin) {
        m_jumpFrameSpin->blockSignals(true);
        m_jumpFrameSpin->setValue(frameIndex);
        m_jumpFrameSpin->blockSignals(false);
    }
    if (m_jumpGopSpin && gop) {
        m_jumpGopSpin->blockSignals(true);
        m_jumpGopSpin->setValue(gop->gopIndex + 1);
        m_jumpGopSpin->blockSignals(false);
    }
    m_timelineView->setSelectedFrame(frameIndex);
    m_overviewBar->setSelectedFrame(frameIndex);
    m_frameTableView->setSelectedFrame(frameIndex);
    m_summaryPanel->setSelectedFrame(frame, gop);
    updateFramePreview(frame);

    if (m_timelineScrollArea && !m_frames.isEmpty()) {
        const int targetX = qMax(0, m_timelineView->canvasPositionForFrame(frame->index) -
                                    m_timelineScrollArea->viewport()->width() / 2);
        m_timelineScrollArea->horizontalScrollBar()->setValue(targetX);
    }

    syncOverviewRange();
}

void MainWindow::setupUi()
{
    setWindowTitle("Visual Frame GOP Analyzer");
    resize(1560, 920);
    setStyleSheet(buildRoundedUiStyleSheet());

    QMenu *toolsMenu = menuBar()->addMenu("Tools");
    QAction *batchAnalyzeAction = toolsMenu->addAction("Batch Analyze...");

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);

    auto *topControlsLayout = new QVBoxLayout();
    topControlsLayout->setContentsMargins(0, 0, 0, 0);
    topControlsLayout->setSpacing(6);
    auto *topRowPrimary = new QHBoxLayout();
    auto *topRowSecondary = new QHBoxLayout();
    auto *chooseButton = new QPushButton("Select Video", this);
    m_recentFilesCombo = new QComboBox(this);
    m_recentFilesCombo->setMinimumWidth(340);
    m_recentFilesCombo->setMaximumWidth(520);
    m_recentFilesCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    m_recentFilesCombo->setToolTip("Recent files");
    rebuildRecentMenu();
    m_videoPathEdit = new QLineEdit(this);
    m_analyzeButton = new QPushButton("Start Analyze", this);
    m_benchmarkButton = new QPushButton("Run Benchmark", this);
    m_openAnalysisLogButton = new QPushButton("Open Analysis Log", this);
    auto *exportCsvButton = new QPushButton("Export CSV...", this);
    auto *exportReportButton = new QPushButton("Export Report...", this);
    auto *reanalyzeButton = new QPushButton("Reanalyze", this);
    m_openLogButton = new QPushButton("Open Log", this);
    m_statusLabel = new QLabel("Ready", this);

    m_analyzeButton->setProperty("accent", true);

    auto configureActionButton = [](QPushButton *button) {
        button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        button->setMinimumWidth(110);
    };
    configureActionButton(chooseButton);
    configureActionButton(m_analyzeButton);
    configureActionButton(m_benchmarkButton);
    configureActionButton(m_openAnalysisLogButton);
    configureActionButton(exportCsvButton);
    configureActionButton(exportReportButton);
    configureActionButton(reanalyzeButton);
    configureActionButton(m_openLogButton);
    m_recentFilesCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_videoPathEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_videoPathEdit->setClearButtonEnabled(true);
    m_videoPathEdit->setToolTip("Input video path. Full path is shown on hover.");
    m_statusLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    m_openLogButton->setEnabled(false);

    connect(m_recentFilesCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this,
            [this](int index) {
        if (index < 0) {
            return;
        }
        const QString path = m_recentFilesCombo->itemData(index).toString();
        if (path.isEmpty()) {
            return;
        }
        if (path == QStringLiteral("__clear__")) {
            QSettings().remove(QStringLiteral("recentFiles"));
            rebuildRecentMenu();
            return;
        }
        m_videoPath = path;
        m_videoPathEdit->setText(path);
        m_videoPathEdit->setToolTip(path);
        updateSourceInfo();
    });

    // Keep file selection on the first row, and group analysis actions on the second row.
    topRowPrimary->addWidget(chooseButton);
    topRowPrimary->addWidget(m_recentFilesCombo);
    topRowPrimary->addWidget(m_videoPathEdit, 1);

    topRowSecondary->addWidget(m_analyzeButton);
    topRowSecondary->addWidget(reanalyzeButton);
    topRowSecondary->addWidget(m_benchmarkButton);
    topRowSecondary->addWidget(m_openAnalysisLogButton);
    topRowSecondary->addWidget(exportCsvButton);
    topRowSecondary->addWidget(exportReportButton);
    topRowSecondary->addWidget(m_openLogButton);
    topRowSecondary->addStretch(1);

    // Status details are shown in statusBar() to avoid duplicated horizontal pressure.
    m_statusLabel->setVisible(false);

    topControlsLayout->addLayout(topRowPrimary);
    topControlsLayout->addLayout(topRowSecondary);

    auto *mainVerticalSplitter = new QSplitter(Qt::Vertical, this);
    auto *topPanel = new QWidget(this);
    auto *topPanelLayout = new QHBoxLayout(topPanel);

    auto *leftBox = new QGroupBox("Source", this);
    leftBox->setMinimumWidth(220);
    leftBox->setMaximumWidth(340);
    auto *leftForm = new QFormLayout(leftBox);
    m_fileNameValue = new QLabel("-");
    m_fileNameValue->setWordWrap(true);
    m_codecValue = new QLabel("HEVC/H264(auto)");
    m_resolutionValue = new QLabel("-");
    m_fpsValue = new QLabel("-");
    m_logPathValue = new QLabel("-");
    m_logPathValue->setWordWrap(true);
    m_bitrateValue = new QLabel("-");

    leftForm->addRow("File:", m_fileNameValue);
    leftForm->addRow("Codec:", m_codecValue);
    leftForm->addRow("Resolution:", m_resolutionValue);
    leftForm->addRow("FPS:", m_fpsValue);
    leftForm->addRow("Log:", m_logPathValue);
    leftForm->addRow("Average Bitrate:", m_bitrateValue);

    auto *centerContainer = new QWidget(this);
    auto *centerLayout = new QVBoxLayout(centerContainer);

    auto *timelineToolbar = new QVBoxLayout();
    timelineToolbar->setContentsMargins(0, 0, 0, 0);
    timelineToolbar->setSpacing(4);
    auto *timelineToolbarRowPrimary = new QHBoxLayout();
    auto *timelineToolbarRowSecondary = new QHBoxLayout();

    auto *zoomOutButton = new QPushButton("Zoom -", this);
    auto *zoomInButton = new QPushButton("Zoom +", this);
    auto *zoomResetButton = new QPushButton("Reset", this);
    auto *prevIButton = new QPushButton("Prev I", this);
    auto *nextIButton = new QPushButton("Next I", this);
    auto *prevFrameButton = new QPushButton("Prev Frame", this);
    auto *nextFrameButton = new QPushButton("Next Frame", this);

    // Capsule grouping: navigation cluster
    prevFrameButton->setProperty("capsule", "first");
    nextFrameButton->setProperty("capsule", "mid");
    prevIButton->setProperty("capsule", "mid");
    nextIButton->setProperty("capsule", "last");

    // Capsule grouping: zoom cluster
    zoomOutButton->setProperty("capsule", "first");
    zoomResetButton->setProperty("capsule", "mid");
    zoomInButton->setProperty("capsule", "last");
    m_zoomPercentLabel = new QLabel("100%", this);
    m_zoomSlider = new QSlider(Qt::Horizontal, this);
    m_zoomSlider->setRange(25, 800);
    m_zoomSlider->setValue(100);
    m_zoomSlider->setMinimumWidth(120);
    m_zoomSlider->setMaximumWidth(200);
    m_jumpGopSpin = new QSpinBox(this);
    m_jumpGopSpin->setMinimum(1);
    m_jumpGopSpin->setMaximum(1);
    auto *jumpGopButton = new QPushButton("Go GOP", this);
    m_jumpFrameSpin = new QSpinBox(this);
    m_jumpFrameSpin->setMinimum(0);
    m_jumpFrameSpin->setMaximum(0);
    m_densityModeCombo = new QComboBox(this);
    m_densityModeCombo->addItem("Frame Mode", static_cast<int>(TimelineView::FrameMode));
    m_densityModeCombo->addItem("GOP Compact", static_cast<int>(TimelineView::GopCompactMode));
    auto *jumpFrameButton = new QPushButton("Go Frame", this);
    jumpGopButton->setToolTip("Jump to target GOP");
    jumpFrameButton->setToolTip("Jump to target frame");
    auto *bSeparatorCheck = new QCheckBox("B separator", this);
    bSeparatorCheck->setChecked(true);
    auto createLegendChip = [this](const QString &text, const QString &color) {
        auto *label = new QLabel(text, this);
        label->setAlignment(Qt::AlignCenter);
        label->setFixedHeight(22);
        label->setMinimumWidth(34);
        label->setStyleSheet(QString("QLabel { background:%1; color:white; border-radius:4px; padding:0 6px; }")
                                 .arg(color));
        return label;
    };
    auto *iLegend = createLegendChip("I", "#e05454");
    auto *pLegend = createLegendChip("P", "#498cff");
    auto *bLegend = createLegendChip("B", "#4fb874");
    timelineToolbarRowPrimary->addWidget(new QLabel("Timeline", this));
    timelineToolbarRowPrimary->addSpacing(8);
    timelineToolbarRowPrimary->addWidget(new QLabel("Legend:", this));
    timelineToolbarRowPrimary->addWidget(iLegend);
    timelineToolbarRowPrimary->addWidget(pLegend);
    timelineToolbarRowPrimary->addWidget(bLegend);
    timelineToolbarRowPrimary->addWidget(bSeparatorCheck);
    timelineToolbarRowPrimary->addStretch();
    timelineToolbarRowPrimary->addWidget(prevFrameButton);
    timelineToolbarRowPrimary->addWidget(nextFrameButton);
    timelineToolbarRowPrimary->addWidget(prevIButton);
    timelineToolbarRowPrimary->addWidget(nextIButton);

    timelineToolbarRowSecondary->addWidget(new QLabel("GOP:", this));
    timelineToolbarRowSecondary->addWidget(m_jumpGopSpin);
    timelineToolbarRowSecondary->addWidget(jumpGopButton);
    timelineToolbarRowSecondary->addWidget(new QLabel("Frame:", this));
    timelineToolbarRowSecondary->addWidget(m_jumpFrameSpin);
    timelineToolbarRowSecondary->addWidget(jumpFrameButton);
    timelineToolbarRowSecondary->addWidget(new QLabel("Density:", this));
    timelineToolbarRowSecondary->addWidget(m_densityModeCombo);
    timelineToolbarRowSecondary->addStretch(1);
    timelineToolbarRowSecondary->addWidget(new QLabel("Zoom:", this));
    timelineToolbarRowSecondary->addWidget(zoomOutButton);
    timelineToolbarRowSecondary->addWidget(m_zoomSlider);
    timelineToolbarRowSecondary->addWidget(m_zoomPercentLabel);
    timelineToolbarRowSecondary->addWidget(zoomResetButton);
    timelineToolbarRowSecondary->addWidget(zoomInButton);

    m_timelineView = new TimelineView(this);
    m_overviewBar = new TimelineOverviewBar(this);
    m_timelineScrollArea = new QScrollArea(this);
    m_timelineScrollArea->setWidget(m_timelineView);
    m_timelineScrollArea->setWidgetResizable(false);
    m_timelineScrollArea->setFrameShape(QFrame::NoFrame);
    m_timelineScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_timelineScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_timelineScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    const int timelineScrollHeight = m_timelineView->minimumHeight() +
                                     m_timelineScrollArea->horizontalScrollBar()->sizeHint().height() +
                                     4;
    m_timelineScrollArea->setMinimumHeight(timelineScrollHeight);
    m_timelineScrollArea->setMaximumHeight(timelineScrollHeight);

    m_previewToggle = new QCheckBox("Frame Preview", this);
    m_previewToggle->setChecked(false);
    m_previewToggle->setEnabled(false);
    m_previewToggle->setToolTip("Show video frame at current position (requires video file)");
    timelineToolbarRowSecondary->addWidget(m_previewToggle);

    m_previewWidget = new PreviewFrameWidget(this);
    m_previewWidget->setVisible(false);

    timelineToolbar->addLayout(timelineToolbarRowPrimary);
    timelineToolbar->addLayout(timelineToolbarRowSecondary);

    centerLayout->addLayout(timelineToolbar);
    centerLayout->addWidget(m_overviewBar);
    centerLayout->addWidget(m_timelineScrollArea);
    centerLayout->addWidget(m_previewWidget);
    centerLayout->addStretch(1);

    m_summaryPanel = new SummaryPanel(this);

    // Collapsible GOP histogram
    auto *rightPanel = new QWidget(this);
    rightPanel->setObjectName("summaryCard");
    rightPanel->setMinimumWidth(220);
    rightPanel->setMaximumWidth(340);
    auto *rightPanelLayout = new QVBoxLayout(rightPanel);
    rightPanelLayout->setContentsMargins(8, 10, 8, 8);
    rightPanelLayout->setSpacing(4);

    auto *histToggle = new QToolButton(this);
    histToggle->setText("GOP Size Histogram");
    histToggle->setCheckable(true);
    histToggle->setChecked(false);
    histToggle->setArrowType(Qt::RightArrow);
    histToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    histToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_gopHistogram = new GopHistogramWidget(this);
    m_gopHistogram->setVisible(false);
    m_gopHistogram->setMinimumHeight(110);
    m_gopHistogram->setMaximumHeight(160);

    connect(histToggle, &QToolButton::toggled, this, [histToggle, this](bool checked) {
        m_gopHistogram->setVisible(checked);
        histToggle->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
    });

    rightPanelLayout->addWidget(m_summaryPanel);
    rightPanelLayout->addWidget(histToggle);
    rightPanelLayout->addWidget(m_gopHistogram);
    rightPanelLayout->addStretch(1);

    topPanelLayout->addWidget(leftBox, 1);
    topPanelLayout->addWidget(centerContainer, 5);
    topPanelLayout->addWidget(rightPanel, 1);

    m_frameTableView = new FrameTableView(this);

    mainVerticalSplitter->addWidget(topPanel);
    mainVerticalSplitter->addWidget(m_frameTableView);
    mainVerticalSplitter->setStretchFactor(0, 2);
    mainVerticalSplitter->setStretchFactor(1, 3);

    rootLayout->addLayout(topControlsLayout);
    rootLayout->addWidget(mainVerticalSplitter, 1);

    setCentralWidget(central);
    statusBar()->showMessage("Ready");
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setFixedWidth(180);
    statusBar()->addPermanentWidget(m_progressBar);

    connect(chooseButton, SIGNAL(clicked()), this, SLOT(chooseVideoFile()));
    connect(batchAnalyzeAction, SIGNAL(triggered()), this, SLOT(startBatchAnalysis()));
    connect(m_openAnalysisLogButton, SIGNAL(clicked()), this, SLOT(chooseLogFile()));
    connect(m_analyzeButton, SIGNAL(clicked()), this, SLOT(startAnalysis()));
    connect(m_benchmarkButton, SIGNAL(clicked()), this, SLOT(runBenchmark()));
    connect(exportCsvButton, SIGNAL(clicked()), this, SLOT(exportFrameCsv()));
    connect(exportReportButton, SIGNAL(clicked()), this, SLOT(exportAnalysisReport()));
    connect(reanalyzeButton, SIGNAL(clicked()), this, SLOT(startAnalysis()));
    connect(m_openLogButton, SIGNAL(clicked()), this, SLOT(openLogFile()));
    connect(m_timelineView, SIGNAL(frameSelected(int)), this, SLOT(onFrameSelected(int)));
    connect(m_frameTableView, SIGNAL(frameSelected(int)), this, SLOT(onFrameSelected(int)));
    connect(jumpFrameButton, SIGNAL(clicked()), this, SLOT(jumpToFrame()));
    connect(jumpGopButton, SIGNAL(clicked()), this, SLOT(jumpToGop()));
    connect(prevFrameButton, SIGNAL(clicked()), this, SLOT(selectPreviousFrame()));
    connect(nextFrameButton, SIGNAL(clicked()), this, SLOT(selectNextFrame()));
    connect(prevIButton, SIGNAL(clicked()), this, SLOT(selectPreviousIFrame()));
    connect(nextIButton, SIGNAL(clicked()), this, SLOT(selectNextIFrame()));
    connect(m_frameTableView, &FrameTableView::filterChanged, this,
            [this](bool showI, bool showP, bool showB) {
                m_timelineView->setFrameTypeFilter(showI, showP, showB);
            });
    connect(m_frameTableView, &FrameTableView::rangeFilterChanged, this,
            [this](int startFrame, int endFrame, double startTime, double endTime) {
                m_timelineView->setRangeFilter(startFrame, endFrame, startTime, endTime);
            });
    connect(m_timelineView, &TimelineView::zoomRequested, this,
            [this](double factor, int anchorFrameIndex) {
                applyZoomFactor(m_timelineZoom * factor, anchorFrameIndex);
            });
    connect(m_overviewBar, &TimelineOverviewBar::frameJumpRequested,
            this, &MainWindow::onFrameSelected);
    connect(m_timelineScrollArea->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, [this]() {
                syncOverviewRange();
            });

    connect(zoomInButton, &QPushButton::clicked, this, [this]() {
        int anchorFrame = -1;
        if (!m_frames.isEmpty()) {
            const int centerX = m_timelineScrollArea->horizontalScrollBar()->value() +
                                m_timelineScrollArea->viewport()->width() / 2;
            anchorFrame = m_timelineView->frameAtCanvasPosition(centerX);
        }
        applyZoomFactor(m_timelineZoom * 1.25, anchorFrame);
    });
    connect(zoomOutButton, &QPushButton::clicked, this, [this]() {
        int anchorFrame = -1;
        if (!m_frames.isEmpty()) {
            const int centerX = m_timelineScrollArea->horizontalScrollBar()->value() +
                                m_timelineScrollArea->viewport()->width() / 2;
            anchorFrame = m_timelineView->frameAtCanvasPosition(centerX);
        }
        applyZoomFactor(m_timelineZoom / 1.25, anchorFrame);
    });
    connect(zoomResetButton, &QPushButton::clicked, this, [this]() {
        int anchorFrame = -1;
        if (!m_frames.isEmpty()) {
            const int centerX = m_timelineScrollArea->horizontalScrollBar()->value() +
                                m_timelineScrollArea->viewport()->width() / 2;
            anchorFrame = m_timelineView->frameAtCanvasPosition(centerX);
        }
        applyZoomFactor(1.0, anchorFrame);
    });
    connect(m_zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        applyZoomFactor(static_cast<double>(value) / 100.0);
    });
    connect(m_densityModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                Q_UNUSED(index);
                const int modeValue = m_densityModeCombo->currentData().toInt();
                m_timelineView->setDensityMode(static_cast<TimelineView::DensityMode>(modeValue));
                syncOverviewRange();
            });
    connect(bSeparatorCheck, &QCheckBox::toggled, this,
            [this](bool enabled) {
                m_timelineView->setBFrameSeparatorEnabled(enabled);
            });
    connect(m_previewToggle, &QCheckBox::toggled, this, &MainWindow::onPreviewToggled);
}

void MainWindow::updateSourceInfo()
{
    const QString displayPath = !m_videoPath.isEmpty() ? m_videoPath : m_logPath;
    QFileInfo info(displayPath);
    const QString fileName = info.fileName();
    m_fileNameValue->setText(fileName.isEmpty() ? "-" : wrapLongText(fileName, 36));
    m_codecValue->setText(m_summary.codec.isEmpty() ? "-" : m_summary.codec);
    m_resolutionValue->setText(m_summary.resolution.isEmpty() ? "-" : m_summary.resolution);
    m_fpsValue->setText(m_summary.fpsText.isEmpty() ? "-" : m_summary.fpsText);

    if (m_bitrateValue) {
        m_bitrateValue->setText(m_summary.averageBitrate > 0 ? QString::number(m_summary.averageBitrate, 'f', 2) + " kbps" : "-");
    }
}

void MainWindow::clearResultViews()
{
    m_summary = AnalysisSummary();
    m_frames.clear();
    m_gops.clear();
    m_videoPath.clear();
    m_logPath.clear();
    m_timelineZoom = 1.0;
    m_timelineView->setData(m_frames, m_gops);
    m_overviewBar->setData(m_frames, m_gops);
    m_timelineView->setZoomFactor(m_timelineZoom);
    if (m_zoomSlider) {
        m_zoomSlider->blockSignals(true);
        m_zoomSlider->setValue(100);
        m_zoomSlider->blockSignals(false);
    }
    if (m_zoomPercentLabel) {
        m_zoomPercentLabel->setText("100%");
    }
    m_frameTableView->setFrames(m_frames);
    m_summaryPanel->clearSummary();
    m_gopHistogram->setGops({});
    m_logPathValue->setText("-");
    m_jumpFrameSpin->setRange(0, 0);
    m_jumpFrameSpin->setValue(0);
    m_jumpGopSpin->setRange(1, 1);
    m_jumpGopSpin->setValue(1);
    if (m_densityModeCombo) {
        m_densityModeCombo->setCurrentIndex(0);
    }
    if (m_videoPathEdit) {
        m_videoPathEdit->clear();
    }
    if (m_previewToggle) {
        m_previewToggle->blockSignals(true);
        m_previewToggle->setChecked(false);
        m_previewToggle->setEnabled(false);
        m_previewToggle->blockSignals(false);
    }
    if (m_previewWidget) {
        m_previewWidget->setVisible(false);
        m_previewWidget->showPlaceholder();
    }
    updateSourceInfo();
    syncOverviewRange();
}

void MainWindow::jumpToFrame()
{
    if (m_frames.isEmpty()) {
        return;
    }

    onFrameSelected(m_jumpFrameSpin->value());
}

void MainWindow::jumpToGop()
{
    if (m_gops.isEmpty()) {
        return;
    }

    const int gopOneBased = m_jumpGopSpin->value();
    const int gopIndex = gopOneBased - 1;
    const GopSegment *gop = findGop(gopIndex);
    if (!gop) {
        return;
    }

    onFrameSelected(gop->startFrame);
}

void MainWindow::selectPreviousFrame()
{
    selectFrameByOffset(-1);
}

void MainWindow::selectNextFrame()
{
    selectFrameByOffset(1);
}

void MainWindow::selectPreviousIFrame()
{
    selectAdjacentIFrame(-1);
}

void MainWindow::selectNextIFrame()
{
    selectAdjacentIFrame(1);
}

void MainWindow::applyZoomFactor(double newZoom, int anchorFrameIndex)
{
    const double bounded = qBound(0.25, newZoom, 8.0);
    if (qFuzzyCompare(m_timelineZoom + 1.0, bounded + 1.0)) {
        return;
    }

    m_timelineZoom = bounded;
    m_timelineView->setZoomFactor(m_timelineZoom);

    if (m_zoomPercentLabel) {
        m_zoomPercentLabel->setText(QString("%1%").arg(static_cast<int>(m_timelineZoom * 100.0)));
    }

    if (m_zoomSlider) {
        m_zoomSlider->blockSignals(true);
        m_zoomSlider->setValue(static_cast<int>(m_timelineZoom * 100.0));
        m_zoomSlider->blockSignals(false);
    }

    if (!m_timelineScrollArea || m_frames.isEmpty()) {
        return;
    }

    if (anchorFrameIndex < 0) {
        const int centerX = m_timelineScrollArea->horizontalScrollBar()->value() +
                            m_timelineScrollArea->viewport()->width() / 2;
        anchorFrameIndex = m_timelineView->frameAtCanvasPosition(centerX);
        if (anchorFrameIndex < 0) {
            anchorFrameIndex = m_frames.first().index;
        }
    }

    anchorFrameIndex = qBound(m_frames.first().index, anchorFrameIndex, m_frames.last().index);

    const int targetX = qMax(0, m_timelineView->canvasPositionForFrame(anchorFrameIndex) -
                                m_timelineScrollArea->viewport()->width() / 2);
    m_timelineScrollArea->horizontalScrollBar()->setValue(targetX);
    syncOverviewRange();
}

void MainWindow::syncOverviewRange()
{
    if (!m_overviewBar || !m_timelineScrollArea || m_frames.isEmpty()) {
        return;
    }

    const int scrollX = m_timelineScrollArea->horizontalScrollBar()->value();
    const int rightX = scrollX + m_timelineScrollArea->viewport()->width();

    int first = m_timelineView->frameAtCanvasPosition(scrollX);
    int last = m_timelineView->frameAtCanvasPosition(rightX);

    if (first < 0) {
        first = m_frames.first().index;
    }
    if (last < 0) {
        last = m_frames.last().index;
    }

    m_overviewBar->setVisibleRange(first, last);

    const int contentWidth = qMax(1, m_timelineView->width());
    const double startRatio = static_cast<double>(scrollX) / static_cast<double>(contentWidth);
    const double endRatio = static_cast<double>(qMin(contentWidth, rightX)) / static_cast<double>(contentWidth);
    m_overviewBar->setViewportRatio(startRatio, endRatio);
}

bool MainWindow::loadAnalysisLog(const QString &logPath, const QString &sourceVideoPath)
{
    QString parseError;
    QVector<FrameInfo> parsedFrames;
    if (!ShowInfoParser::parseFile(logPath, parsedFrames, parseError)) {
        m_statusLabel->setText("Parse failed");
        statusBar()->showMessage("Parse failed", 5000);
        QMessageBox::critical(this, "Parse failed", parseError);
        return false;
    }

    m_videoPath = sourceVideoPath;
    m_logPath = logPath;
    m_frames = parsedFrames;
    m_gops = ShowInfoParser::buildGops(m_frames);
    m_summary = ShowInfoParser::buildSummary(m_videoPath, m_logPath, m_frames, m_gops);

    m_timelineView->setData(m_frames, m_gops);
    m_overviewBar->setData(m_frames, m_gops);
    m_frameTableView->setFrames(m_frames);
    m_summaryPanel->setSummary(m_summary);
    m_gopHistogram->setGops(m_gops);

    if (!m_frames.isEmpty()) {
        m_jumpFrameSpin->setRange(m_frames.first().index, m_frames.last().index);
        m_jumpFrameSpin->setValue(m_frames.first().index);
        onFrameSelected(m_frames.first().index);
    }
    if (!m_gops.isEmpty()) {
        m_jumpGopSpin->setRange(1, m_gops.size());
        m_jumpGopSpin->setValue(1);
    }

    m_openLogButton->setEnabled(true);
    m_logPathValue->setText(m_logPath);
    updateSourceInfo();
    syncOverviewRange();
    updatePreviewToggleState();
    m_statusLabel->setText(sourceVideoPath.isEmpty() ? "Analysis log loaded" : "Analyze complete");
    statusBar()->showMessage(m_statusLabel->text(), 4000);
    return true;
}

int MainWindow::currentSelectedFrameIndex() const
{
    if (m_jumpFrameSpin && m_jumpFrameSpin->minimum() <= m_jumpFrameSpin->maximum()) {
        return m_jumpFrameSpin->value();
    }

    if (!m_frames.isEmpty()) {
        return m_frames.first().index;
    }

    return -1;
}

void MainWindow::selectFrameByOffset(int offset)
{
    if (m_frames.isEmpty() || offset == 0) {
        return;
    }

    const int current = currentSelectedFrameIndex();
    for (int i = 0; i < m_frames.size(); ++i) {
        if (m_frames[i].index != current) {
            continue;
        }

        const int nextPos = qBound(0, i + offset, m_frames.size() - 1);
        m_jumpFrameSpin->setValue(m_frames[nextPos].index);
        onFrameSelected(m_frames[nextPos].index);
        return;
    }

    m_jumpFrameSpin->setValue(m_frames.first().index);
    onFrameSelected(m_frames.first().index);
}

void MainWindow::selectAdjacentIFrame(int direction)
{
    if (m_frames.isEmpty() || direction == 0) {
        return;
    }

    const int current = currentSelectedFrameIndex();
    int currentPos = 0;
    for (int i = 0; i < m_frames.size(); ++i) {
        if (m_frames[i].index == current) {
            currentPos = i;
            break;
        }
    }

    for (int i = currentPos + direction; i >= 0 && i < m_frames.size(); i += direction) {
        if (m_frames[i].type == QChar('I')) {
            m_jumpFrameSpin->setValue(m_frames[i].index);
            onFrameSelected(m_frames[i].index);
            return;
        }
    }
}

// ── Recent files ────────────────────────────────────────────────────────────

static const int kMaxRecentFiles = 10;
static const char kRecentKey[]   = "recentFiles";

void MainWindow::addRecentFile(const QString &path)
{
    if (path.isEmpty()) {
        return;
    }
    QSettings settings;
    QStringList files = settings.value(kRecentKey).toStringList();
    files.removeAll(path);
    files.prepend(path);
    while (files.size() > kMaxRecentFiles) {
        files.removeLast();
    }
    settings.setValue(kRecentKey, files);
    rebuildRecentMenu();
}

void MainWindow::rebuildRecentMenu()
{
    if (!m_recentFilesCombo) {
        return;
    }
    const bool oldSignalsBlocked = m_recentFilesCombo->blockSignals(true);
    m_recentFilesCombo->clear();

    QSettings settings;
    const QStringList files = settings.value(kRecentKey).toStringList();
    if (files.isEmpty()) {
        m_recentFilesCombo->addItem("(No recent files)", QString());
        m_recentFilesCombo->setCurrentIndex(0);
        m_recentFilesCombo->blockSignals(oldSignalsBlocked);
        return;
    }

    for (const QString &file : files) {
        m_recentFilesCombo->addItem(file, file);
        m_recentFilesCombo->setItemData(m_recentFilesCombo->count() - 1, file, Qt::ToolTipRole);
    }
    m_recentFilesCombo->insertSeparator(m_recentFilesCombo->count());
    m_recentFilesCombo->addItem("Clear recent files", QStringLiteral("__clear__"));
    m_recentFilesCombo->setCurrentIndex(0);
    m_recentFilesCombo->blockSignals(oldSignalsBlocked);
}

// ── Drag and drop ────────────────────────────────────────────────────────────

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            const QString path = urls.first().toLocalFile();
            const QString ext = QFileInfo(path).suffix().toLower();
            static const QStringList videoExts = {"mp4", "mov", "mkv", "avi", "mxf"};
            static const QStringList logExts   = {"log", "txt"};
            if (videoExts.contains(ext) || logExts.contains(ext)) {
                event->setDropAction(Qt::LinkAction);
                event->accept();
                return;
            }
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }
    const QString path = event->mimeData()->urls().first().toLocalFile();
    if (path.isEmpty()) {
        event->ignore();
        return;
    }
    const QString ext = QFileInfo(path).suffix().toLower();
    static const QStringList videoExts = {"mp4", "mov", "mkv", "avi", "mxf"};
    if (videoExts.contains(ext)) {
        m_videoPath = path;
        m_videoPathEdit->setText(path);
        m_videoPathEdit->setToolTip(path);
        updateSourceInfo();
    } else {
        loadAnalysisLog(path);
    }
    event->setDropAction(Qt::LinkAction);
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Left) {
        selectPreviousFrame();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Right) {
        selectNextFrame();
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

const FrameInfo *MainWindow::findFrame(int frameIndex) const
{
    for (const FrameInfo &frame : m_frames) {
        if (frame.index == frameIndex) {
            return &frame;
        }
    }
    return nullptr;
}

const GopSegment *MainWindow::findGop(int gopIndex) const
{
    if (gopIndex < 0 || gopIndex >= m_gops.size()) {
        return nullptr;
    }
    return &m_gops[gopIndex];
}

void MainWindow::updatePreviewToggleState()
{
    if (!m_previewToggle) {
        return;
    }
    const bool hasVideo = !m_videoPath.isEmpty() && QFileInfo::exists(m_videoPath);
    m_previewToggle->setEnabled(hasVideo);
    if (!hasVideo) {
        m_previewToggle->blockSignals(true);
        m_previewToggle->setChecked(false);
        m_previewToggle->blockSignals(false);
        if (m_previewWidget) {
            m_previewWidget->setVisible(false);
        }
    }
}

void MainWindow::onPreviewToggled(bool enabled)
{
    if (!m_previewWidget) {
        return;
    }
    m_previewWidget->setVisible(enabled);
    if (enabled) {
        const FrameInfo *frame = findFrame(currentSelectedFrameIndex());
        if (frame) {
            updateFramePreview(frame);
        } else {
            m_previewWidget->showPlaceholder();
        }
    }
}

void MainWindow::updateFramePreview(const FrameInfo *frame)
{
    if (!m_previewToggle || !m_previewToggle->isChecked() || !m_previewWidget) {
        return;
    }
    if (m_videoPath.isEmpty() || !QFileInfo::exists(m_videoPath)) {
        return;
    }
    if (!frame) {
        m_previewWidget->showPlaceholder();
        return;
    }

    const int seq = ++m_previewRequestSeq;
    const double ptsTime = frame->ptsTime;
    const int frameIndex = frame->index;
    const QString videoPath = m_videoPath;

    m_previewWidget->showLoading();

    auto *watcher = new QFutureWatcher<FrameCaptureResult>(this);
    connect(watcher, &QFutureWatcher<FrameCaptureResult>::finished, this,
            [this, watcher, seq]() {
                const FrameCaptureResult result = watcher->result();
                watcher->deleteLater();
                // Discard stale results from rapid navigation
                if (seq != m_previewRequestSeq || !m_previewWidget) {
                    return;
                }
                if (result.ok) {
                    m_previewWidget->showImage(result.outputPath);
                } else {
                    m_previewWidget->showError(result.errorMessage);
                }
            });

    watcher->setFuture(QtConcurrent::run([videoPath, ptsTime, frameIndex, seq]() {
        FrameCaptureResult result;
        result.requestSeq = seq;
        result.ok = FfmpegRunner::captureFrame(
            videoPath, ptsTime, frameIndex, result.outputPath, result.errorMessage);
        return result;
    }));
}
