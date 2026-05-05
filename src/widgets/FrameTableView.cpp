#include "FrameTableView.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QVBoxLayout>

FrameTableView::FrameTableView(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *rangeLayout = new QHBoxLayout();
    m_startFrameFilter = new QSpinBox(this);
    m_endFrameFilter = new QSpinBox(this);
    m_startTimeFilter = new QDoubleSpinBox(this);
    m_endTimeFilter = new QDoubleSpinBox(this);
    m_clearFilterButton = new QPushButton("Clear Filter", this);

    m_startFrameFilter->setRange(0, 0);
    m_endFrameFilter->setRange(0, 0);
    m_startFrameFilter->setValue(0);
    m_endFrameFilter->setValue(0);

    m_startTimeFilter->setRange(0.0, 0.0);
    m_endTimeFilter->setRange(0.0, 0.0);
    m_startTimeFilter->setDecimals(3);
    m_endTimeFilter->setDecimals(3);
    m_startTimeFilter->setSingleStep(0.5);
    m_endTimeFilter->setSingleStep(0.5);
    m_startTimeFilter->setValue(0.0);
    m_endTimeFilter->setValue(0.0);

    rangeLayout->addWidget(new QLabel("Range:", this));
    rangeLayout->addWidget(new QLabel("Frame", this));
    rangeLayout->addWidget(m_startFrameFilter);
    rangeLayout->addWidget(new QLabel("to", this));
    rangeLayout->addWidget(m_endFrameFilter);
    rangeLayout->addSpacing(12);
    rangeLayout->addWidget(new QLabel("Time (s)", this));
    rangeLayout->addWidget(m_startTimeFilter);
    rangeLayout->addWidget(new QLabel("to", this));
    rangeLayout->addWidget(m_endTimeFilter);
    rangeLayout->addWidget(m_clearFilterButton);
    rangeLayout->addStretch();

    auto *filterLayout = new QHBoxLayout();
    m_typeFilter = new QComboBox(this);
    m_typeFilter->addItems(QStringList() << "All" << "I" << "P" << "B");

    m_keyOnly = new QCheckBox("Key frame only", this);

    filterLayout->addWidget(new QLabel("Type:", this));
    filterLayout->addWidget(m_typeFilter);
    filterLayout->addWidget(m_keyOnly);
    filterLayout->addStretch();

    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels(
        QStringList() << "Frame" << "PTS Time" << "Type" << "Key" << "GOP #" << "In GOP" << "Duration" << "Raw");
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Interactive);
    m_table->setColumnWidth(7, 900);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setWordWrap(false);
    m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    mainLayout->addLayout(rangeLayout);
    mainLayout->addLayout(filterLayout);
    mainLayout->addWidget(m_table);

    connect(m_typeFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(rebuildTable()));
    connect(m_keyOnly, SIGNAL(stateChanged(int)), this, SLOT(rebuildTable()));
    connect(m_startFrameFilter, SIGNAL(valueChanged(int)), this, SLOT(rebuildTable()));
    connect(m_endFrameFilter, SIGNAL(valueChanged(int)), this, SLOT(rebuildTable()));
    connect(m_startTimeFilter, SIGNAL(valueChanged(double)), this, SLOT(rebuildTable()));
    connect(m_endTimeFilter, SIGNAL(valueChanged(double)), this, SLOT(rebuildTable()));
    connect(m_clearFilterButton, &QPushButton::clicked, this, &FrameTableView::resetRangeFilter);
    connect(m_table, SIGNAL(cellClicked(int,int)), this, SLOT(onCellClicked(int,int)));
}

void FrameTableView::setFrames(const QVector<FrameInfo> &frames)
{
    m_frames = frames;

    if (m_frames.isEmpty()) {
        m_minFrame = 0;
        m_maxFrame = 0;
        m_minTime = 0.0;
        m_maxTime = 0.0;
        resetRangeFilter();
        rebuildTable();
        return;
    }

    m_minFrame = m_frames.first().index;
    m_maxFrame = m_frames.last().index;
    m_minTime = m_frames.first().ptsTime;
    m_maxTime = m_frames.last().ptsTime;

    {
        QSignalBlocker startFrameBlocker(m_startFrameFilter);
        QSignalBlocker endFrameBlocker(m_endFrameFilter);
        QSignalBlocker startTimeBlocker(m_startTimeFilter);
        QSignalBlocker endTimeBlocker(m_endTimeFilter);
        m_startFrameFilter->setRange(m_minFrame, m_maxFrame);
        m_endFrameFilter->setRange(m_minFrame, m_maxFrame);
        m_startTimeFilter->setRange(m_minTime, m_maxTime);
        m_endTimeFilter->setRange(m_minTime, m_maxTime);
        m_startFrameFilter->setValue(m_minFrame);
        m_endFrameFilter->setValue(m_maxFrame);
        m_startTimeFilter->setValue(m_minTime);
        m_endTimeFilter->setValue(m_maxTime);
    }

    rebuildTable();
}

void FrameTableView::setSelectedFrame(int frameIndex)
{
    for (auto it = m_rowToFrameIndex.constBegin(); it != m_rowToFrameIndex.constEnd(); ++it) {
        if (it.value() == frameIndex) {
            QSignalBlocker blocker(m_table);
            m_table->selectRow(it.key());
            return;
        }
    }
}

void FrameTableView::rebuildTable()
{
    emitFilterState();

    m_rowToFrameIndex.clear();
    m_table->setRowCount(0);

    int row = 0;
    for (const FrameInfo &frame : m_frames) {
        if (!accepts(frame)) {
            continue;
        }

        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(frame.index)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(frame.ptsTime, 'f', 4)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString(frame.type)));
        m_table->setItem(row, 3, new QTableWidgetItem(frame.isKey ? "Y" : "N"));
        m_table->setItem(row, 4, new QTableWidgetItem(frame.gopIndex >= 0 ? QString::number(frame.gopIndex + 1) : "-"));
        m_table->setItem(row, 5, new QTableWidgetItem(frame.indexInGop >= 0 ? QString::number(frame.indexInGop) : "-"));
        m_table->setItem(row, 6, new QTableWidgetItem(QString::number(frame.durationTime, 'f', 4)));
        m_table->setItem(row, 7, new QTableWidgetItem(frame.rawLine));

        m_rowToFrameIndex.insert(row, frame.index);
        ++row;
    }
}

void FrameTableView::onCellClicked(int row, int)
{
    if (!m_rowToFrameIndex.contains(row)) {
        return;
    }

    emit frameSelected(m_rowToFrameIndex.value(row));
}

bool FrameTableView::accepts(const FrameInfo &frame) const
{
    const QString type = m_typeFilter->currentText();
    if (type != "All" && type != QString(frame.type)) {
        return false;
    }

    if (m_keyOnly->isChecked() && !frame.isKey) {
        return false;
    }

    const int startFrame = m_startFrameFilter->value();
    const int endFrame = m_endFrameFilter->value();
    const double startTime = m_startTimeFilter->value();
    const double endTime = m_endTimeFilter->value();

    if (frame.index < startFrame) {
        return false;
    }

    if (frame.index > endFrame) {
        return false;
    }

    if (frame.ptsTime < startTime) {
        return false;
    }

    if (frame.ptsTime > endTime) {
        return false;
    }

    return true;
}

void FrameTableView::emitFilterState()
{
    const QString type = m_typeFilter->currentText();
    const bool showI = (type == "All" || type == "I");
    const bool showP = (type == "All" || type == "P");
    const bool showB = (type == "All" || type == "B");
    emit filterChanged(showI, showP, showB);
    emit rangeFilterChanged(m_startFrameFilter->value(),
                            m_endFrameFilter->value(),
                            m_startTimeFilter->value(),
                            m_endTimeFilter->value());
}

void FrameTableView::resetRangeFilter()
{
    QSignalBlocker startFrameBlocker(m_startFrameFilter);
    QSignalBlocker endFrameBlocker(m_endFrameFilter);
    QSignalBlocker startTimeBlocker(m_startTimeFilter);
    QSignalBlocker endTimeBlocker(m_endTimeFilter);

    m_startFrameFilter->setValue(m_minFrame);
    m_endFrameFilter->setValue(m_maxFrame);
    m_startTimeFilter->setValue(m_minTime);
    m_endTimeFilter->setValue(m_maxTime);

    rebuildTable();
}
