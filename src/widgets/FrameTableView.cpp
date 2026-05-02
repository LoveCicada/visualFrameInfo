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

    mainLayout->addLayout(filterLayout);
    mainLayout->addWidget(m_table);

    connect(m_typeFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(rebuildTable()));
    connect(m_keyOnly, SIGNAL(stateChanged(int)), this, SLOT(rebuildTable()));
    connect(m_table, SIGNAL(cellClicked(int,int)), this, SLOT(onCellClicked(int,int)));
}

void FrameTableView::setFrames(const QVector<FrameInfo> &frames)
{
    m_frames = frames;
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

    return true;
}

void FrameTableView::emitFilterState()
{
    const QString type = m_typeFilter->currentText();
    const bool showI = (type == "All" || type == "I");
    const bool showP = (type == "All" || type == "P");
    const bool showB = (type == "All" || type == "B");
    emit filterChanged(showI, showP, showB);
}
