#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QVector>

#include "../models/FrameInfo.h"
#include "../models/GopSegment.h"

class TimelineView : public QWidget
{
    Q_OBJECT

public:
    enum DensityMode {
        FrameMode = 0,
        GopCompactMode = 1
    };

    explicit TimelineView(QWidget *parent = nullptr);

    void setData(const QVector<FrameInfo> &frames, const QVector<GopSegment> &gops);
    void setFrameTypeFilter(bool showI, bool showP, bool showB);
    void setZoomFactor(double zoomFactor);
    void setSelectedFrame(int frameIndex);
    void setDensityMode(DensityMode mode);
    void setBFrameSeparatorEnabled(bool enabled);
    int frameAtPosition(int x) const;
    int frameAtCanvasPosition(int canvasX) const;
    int canvasPositionForFrame(int frameIndex) const;

signals:
    void frameSelected(int frameIndex);
    void zoomRequested(double factor, int anchorFrameIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    QSize sizeHint() const override;

private:
    QColor colorForType(QChar type) const;
    bool typeVisible(QChar type) const;
    int frameByFramePosition(int framePos) const;
    int frameByGopPosition(int gopPos) const;
    void updateCanvasSize();

private:
    QVector<FrameInfo> m_frames;
    QVector<GopSegment> m_gops;
    bool m_showI = true;
    bool m_showP = true;
    bool m_showB = true;
    DensityMode m_densityMode = FrameMode;
    double m_zoomFactor = 1.0;
    int m_selectedFrame = -1;
    int m_hoveredFrame = -1;
    int m_barWidth = 4;
    int m_gopWidth = 14;
    bool m_bFrameSeparatorEnabled = true;
};

#endif // TIMELINEVIEW_H
