#include "TimelineOverviewBar.h"

#include <QMouseEvent>
#include <QPainter>

TimelineOverviewBar::TimelineOverviewBar(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(28);
    setMaximumHeight(36);
    setMouseTracking(true);
}

void TimelineOverviewBar::setData(const QVector<FrameInfo> &frames, const QVector<GopSegment> &gops)
{
    m_frames = frames;
    m_gops = gops;
    m_selectedFrame = -1;
    if (!m_frames.isEmpty()) {
        m_visibleFirstFrame = m_frames.first().index;
        m_visibleLastFrame = m_frames.last().index;
    } else {
        m_visibleFirstFrame = -1;
        m_visibleLastFrame = -1;
    }
    update();
}

void TimelineOverviewBar::setVisibleRange(int firstFrameIndex, int lastFrameIndex)
{
    m_visibleFirstFrame = firstFrameIndex;
    m_visibleLastFrame = lastFrameIndex;
    update();
}

void TimelineOverviewBar::setViewportRatio(double startRatio, double endRatio)
{
    m_viewportStartRatio = qBound(0.0, startRatio, 1.0);
    m_viewportEndRatio = qBound(0.0, endRatio, 1.0);
    if (m_viewportEndRatio < m_viewportStartRatio) {
        qSwap(m_viewportStartRatio, m_viewportEndRatio);
    }
    update();
}

void TimelineOverviewBar::setSelectedFrame(int frameIndex)
{
    m_selectedFrame = frameIndex;
    update();
}

void TimelineOverviewBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(18, 23, 30));

    if (m_frames.isEmpty()) {
        painter.setPen(QColor(170, 180, 195));
        painter.drawText(rect(), Qt::AlignCenter, "Overview");
        return;
    }

    const int left = 6;
    const int right = width() - 6;
    const int top = 6;
    const int barHeight = height() - 12;
    const int totalFrames = m_frames.last().index - m_frames.first().index + 1;
    if (totalFrames <= 0 || right <= left) {
        return;
    }

    for (int x = left; x < right; ++x) {
        const int f0 = m_frames.first().index + ((x - left) * totalFrames) / (right - left);
        const int f1 = m_frames.first().index + ((x - left + 1) * totalFrames) / (right - left);

        int iCount = 0;
        int pCount = 0;
        int bCount = 0;
        for (const FrameInfo &frame : m_frames) {
            if (frame.index < f0 || frame.index >= f1) {
                continue;
            }
            if (frame.type == QChar('I')) {
                ++iCount;
            } else if (frame.type == QChar('P')) {
                ++pCount;
            } else if (frame.type == QChar('B')) {
                ++bCount;
            }
        }

        QColor c(70, 76, 86);
        if (iCount >= pCount && iCount >= bCount && iCount > 0) {
            c = QColor(224, 84, 84);
        } else if (pCount >= bCount && pCount > 0) {
            c = QColor(73, 140, 255);
        } else if (bCount > 0) {
            c = QColor(79, 184, 116);
        }

        painter.setPen(c);
        painter.drawLine(x, top, x, top + barHeight);
    }

    for (const GopSegment &gop : m_gops) {
        const int gx = left + ((gop.startFrame - m_frames.first().index) * (right - left)) / totalFrames;
        painter.setPen(QColor(255, 255, 255, 75));
        painter.drawLine(gx, top, gx, top + barHeight);
    }

    if (m_viewportEndRatio >= m_viewportStartRatio) {
        int vx1 = left + static_cast<int>((right - left) * m_viewportStartRatio);
        int vx2 = left + static_cast<int>((right - left) * m_viewportEndRatio);
        vx1 = qBound(left, vx1, right);
        vx2 = qBound(left, vx2, right);
        if (vx2 < vx1) {
            qSwap(vx1, vx2);
        }
        painter.fillRect(QRect(vx1, top, qMax(2, vx2 - vx1), barHeight), QColor(255, 255, 255, 28));
        painter.setPen(QColor(255, 255, 255, 145));
        painter.drawRect(QRect(vx1, top, qMax(2, vx2 - vx1), barHeight));
    }

    if (m_selectedFrame >= 0) {
        const int sx = left + ((m_selectedFrame - m_frames.first().index) * (right - left)) / totalFrames;
        painter.setPen(QColor(255, 223, 90));
        painter.drawLine(sx, 2, sx, height() - 2);
    }
}

void TimelineOverviewBar::mousePressEvent(QMouseEvent *event)
{
    const int frameIndex = frameForX(event->pos().x());
    if (frameIndex >= 0) {
        emit frameJumpRequested(frameIndex);
    }
}

void TimelineOverviewBar::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    const int frameIndex = frameForX(event->pos().x());
    if (frameIndex >= 0) {
        emit frameJumpRequested(frameIndex);
    }
}

int TimelineOverviewBar::frameForX(int x) const
{
    if (m_frames.isEmpty()) {
        return -1;
    }

    const int left = 6;
    const int right = width() - 6;
    if (x < left || x > right || right <= left) {
        return -1;
    }

    const int totalFrames = m_frames.last().index - m_frames.first().index + 1;
    if (totalFrames <= 0) {
        return -1;
    }

    const int frame = m_frames.first().index + ((x - left) * totalFrames) / (right - left);
    return qBound(m_frames.first().index, frame, m_frames.last().index);
}
