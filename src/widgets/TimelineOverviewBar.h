#ifndef TIMELINEOVERVIEWBAR_H
#define TIMELINEOVERVIEWBAR_H

#include <QWidget>
#include <QVector>

#include "../models/FrameInfo.h"
#include "../models/GopSegment.h"

class TimelineOverviewBar : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineOverviewBar(QWidget *parent = nullptr);

    void setData(const QVector<FrameInfo> &frames, const QVector<GopSegment> &gops);
    void setVisibleRange(int firstFrameIndex, int lastFrameIndex);
    void setViewportRatio(double startRatio, double endRatio);
    void setSelectedFrame(int frameIndex);

signals:
    void frameJumpRequested(int frameIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    int frameForX(int x) const;

private:
    QVector<FrameInfo> m_frames;
    QVector<GopSegment> m_gops;
    int m_visibleFirstFrame = -1;
    int m_visibleLastFrame = -1;
    int m_selectedFrame = -1;
    double m_viewportStartRatio = 0.0;
    double m_viewportEndRatio = 1.0;
};

#endif // TIMELINEOVERVIEWBAR_H
