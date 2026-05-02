#ifndef FRAMETABLEVIEW_H
#define FRAMETABLEVIEW_H

#include <QCheckBox>
#include <QComboBox>
#include <QMap>
#include <QTableWidget>
#include <QWidget>

#include "../models/FrameInfo.h"

class FrameTableView : public QWidget
{
    Q_OBJECT

public:
    explicit FrameTableView(QWidget *parent = nullptr);

    void setFrames(const QVector<FrameInfo> &frames);
    void setSelectedFrame(int frameIndex);

signals:
    void frameSelected(int frameIndex);
    void filterChanged(bool showI, bool showP, bool showB);

private slots:
    void rebuildTable();
    void onCellClicked(int row, int column);

private:
    bool accepts(const FrameInfo &frame) const;
    void emitFilterState();

private:
    QComboBox *m_typeFilter = nullptr;
    QCheckBox *m_keyOnly = nullptr;
    QTableWidget *m_table = nullptr;

    QVector<FrameInfo> m_frames;
    QMap<int, int> m_rowToFrameIndex;
};

#endif // FRAMETABLEVIEW_H
