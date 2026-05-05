#include "TimelineView.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QWheelEvent>

TimelineView::TimelineView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(260);
    setMouseTracking(true);
    updateCanvasSize();
}

void TimelineView::setData(const QVector<FrameInfo> &frames, const QVector<GopSegment> &gops)
{
    m_frames = frames;
    m_gops = gops;
    m_selectedFrame = -1;
    updateCanvasSize();
    update();
}

void TimelineView::setDensityMode(TimelineView::DensityMode mode)
{
    if (m_densityMode == mode) {
        return;
    }

    m_densityMode = mode;
    updateCanvasSize();
    update();
}

void TimelineView::setFrameTypeFilter(bool showI, bool showP, bool showB)
{
    m_showI = showI;
    m_showP = showP;
    m_showB = showB;
    update();
}

void TimelineView::setRangeFilter(int startFrame, int endFrame, double startTime, double endTime)
{
    m_rangeStartFrame = startFrame;
    m_rangeEndFrame = endFrame;
    m_rangeStartTime = startTime;
    m_rangeEndTime = endTime;
    update();
}

void TimelineView::setZoomFactor(double zoomFactor)
{
    m_zoomFactor = qBound(0.25, zoomFactor, 8.0);
    updateCanvasSize();
    update();
}

void TimelineView::setSelectedFrame(int frameIndex)
{
    m_selectedFrame = frameIndex;
    update();
}

void TimelineView::setBFrameSeparatorEnabled(bool enabled)
{
    if (m_bFrameSeparatorEnabled == enabled) {
        return;
    }

    m_bFrameSeparatorEnabled = enabled;
    update();
}

void TimelineView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    QLinearGradient background(0, 0, 0, height());
    background.setColorAt(0.0, QColor(21, 26, 33));
    background.setColorAt(1.0, QColor(14, 18, 24));
    painter.fillRect(rect(), background);

    if (m_frames.isEmpty()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "No analysis data.");
        return;
    }

    const int rulerTop = 18;
    const int rulerBottom = 44;
    const int laneTop = 54;
    const int laneHeight = 88;
    const int barTop = laneTop + 12;
    const int barHeight = 44;

    painter.setPen(QColor(118, 126, 142));
    painter.drawText(12, 14, "Frame Timeline");
    painter.setPen(QColor(57, 66, 78));
    painter.drawLine(0, rulerBottom, width(), rulerBottom);
    painter.drawLine(0, laneTop + laneHeight, width(), laneTop + laneHeight);

    const int unitWidth = (m_densityMode == FrameMode) ? m_barWidth : m_gopWidth;

    for (int g = 0; g < m_gops.size(); ++g) {
        const GopSegment &seg = m_gops[g];
        const int localStart = (m_densityMode == FrameMode) ?
                                   (seg.startFrame - m_frames.first().index) :
                                   seg.gopIndex;
        const int x = localStart * unitWidth;
        const int w = qMax(1, (m_densityMode == FrameMode ? seg.size : 1) * unitWidth);

        if (g % 2 == 0) {
            painter.fillRect(QRect(x, laneTop, w, laneHeight), QColor(255, 255, 255, 8));
        }

        painter.setPen(QColor(90, 102, 120, 90));
        painter.drawLine(x, laneTop, x, laneTop + laneHeight);

        if (w > 56) {
            painter.setPen(QColor(132, 142, 160));
            painter.drawText(x + 6, laneTop + 16, QString("G%1").arg(seg.gopIndex + 1));
        }
    }

    const int pxStep = qMax(70, unitWidth * 50);
    const int frameStep = qMax(1, pxStep / qMax(1, unitWidth));
    painter.setPen(QColor(93, 104, 121));
    const int rulerCount = (m_densityMode == FrameMode) ? m_frames.size() : qMax(1, m_gops.size());
    for (int i = 0; i < rulerCount; i += frameStep) {
        const int x = i * unitWidth;
        painter.drawLine(x, rulerBottom - 8, x, rulerBottom);
        if ((i / frameStep) % 2 == 0) {
            if (m_densityMode == FrameMode) {
                painter.drawText(x + 3, rulerTop + 10, QString::number(m_frames[i].index));
            } else if (i < m_gops.size()) {
                painter.drawText(x + 3, rulerTop + 10, QString("G%1").arg(i + 1));
            }
        }
    }

    if (m_densityMode == FrameMode) {
        for (int i = 0; i < m_frames.size(); ++i) {
            const FrameInfo &frame = m_frames[i];
            if (!typeVisible(frame.type)) {
                continue;
            }

            const int x = i * m_barWidth;
            int typeBarHeight = 24;
            if (frame.type == QChar('I')) {
                typeBarHeight = 52;
            } else if (frame.type == QChar('P')) {
                typeBarHeight = 38;
            }
            const int typeBarTop = barTop + (barHeight - typeBarHeight);
            QRect barRect(x, typeBarTop, m_barWidth, typeBarHeight);
            QColor frameColor = colorForType(frame.type);
            if (!rangeVisible(frame)) {
                frameColor.setAlpha(70);
            }
            painter.fillRect(barRect, frameColor);

            if (frame.type == QChar('B') && m_bFrameSeparatorEnabled) {
                QPen dashPen(QColor(223, 236, 248, 210), 1, Qt::DashLine);
                painter.setPen(dashPen);
                if (m_barWidth >= 3) {
                    painter.drawRect(barRect.adjusted(0, 0, -1, -1));
                } else {
                    painter.drawLine(x + m_barWidth - 1, typeBarTop, x + m_barWidth - 1, typeBarTop + typeBarHeight);
                }
            }

            if (frame.index == m_hoveredFrame) {
                painter.fillRect(barRect.adjusted(0, 0, 0, 0), QColor(255, 255, 255, 35));
            }

            if (frame.index == m_selectedFrame) {
                painter.setPen(QPen(QColor(255, 255, 255), 2));
                painter.drawRect(barRect.adjusted(0, 0, -1, -1));

                painter.setPen(QColor(255, 255, 255, 140));
                painter.drawLine(x + m_barWidth / 2, laneTop, x + m_barWidth / 2, laneTop + laneHeight);
            }

            if (frame.type == QChar('I')) {
                painter.setPen(QPen(QColor(191, 109, 109, 150), 1));
                painter.drawLine(x, laneTop, x, laneTop + laneHeight);
            }
        }
    } else {
        for (const GopSegment &seg : m_gops) {
            const int x = seg.gopIndex * m_gopWidth;
            QRect gopRect(x, barTop, m_gopWidth, barHeight);

            int iCount = 0;
            int pCount = 0;
            int bCount = 0;
            int visibleCount = 0;
            for (const FrameInfo &frame : m_frames) {
                if (frame.index < seg.startFrame || frame.index > seg.endFrame) {
                    continue;
                }
                if (rangeVisible(frame)) {
                    ++visibleCount;
                }
                if (frame.type == QChar('I')) {
                    ++iCount;
                } else if (frame.type == QChar('P')) {
                    ++pCount;
                } else if (frame.type == QChar('B')) {
                    ++bCount;
                }
            }

            const int total = qMax(1, iCount + pCount + bCount);
            const int iHeight = qMax(iCount > 0 ? 1 : 0, (barHeight * iCount) / total);
            const int pHeight = qMax(pCount > 0 ? 1 : 0, (barHeight * pCount) / total);
            int bHeight = barHeight - iHeight - pHeight;
            if (bCount > 0 && bHeight <= 0) {
                bHeight = 1;
            }

            int drawY = barTop;
            const int totalVisible = qMax(0, visibleCount);
            const bool fullyFiltered = (totalVisible == 0);
            const bool partiallyFiltered = (totalVisible > 0 && totalVisible < total);
            if (iHeight > 0) {
                QColor color = colorForType(QChar('I'));
                if (fullyFiltered) {
                    color.setAlpha(70);
                } else if (partiallyFiltered) {
                    color.setAlpha(150);
                }
                painter.fillRect(QRect(x, drawY, m_gopWidth, iHeight), color);
                drawY += iHeight;
            }
            if (pHeight > 0) {
                QColor color = colorForType(QChar('P'));
                if (fullyFiltered) {
                    color.setAlpha(70);
                } else if (partiallyFiltered) {
                    color.setAlpha(150);
                }
                painter.fillRect(QRect(x, drawY, m_gopWidth, pHeight), color);
                drawY += pHeight;
            }
            if (drawY < barTop + barHeight) {
                QColor color = colorForType(QChar('B'));
                if (fullyFiltered) {
                    color.setAlpha(70);
                } else if (partiallyFiltered) {
                    color.setAlpha(150);
                }
                painter.fillRect(QRect(x, drawY, m_gopWidth, barTop + barHeight - drawY), color);
            }

            painter.setPen(QColor(255, 255, 255, 22));
            painter.drawRect(gopRect.adjusted(0, 0, -1, -1));

            if (seg.startFrame == m_hoveredFrame ||
                (m_hoveredFrame >= seg.startFrame && m_hoveredFrame <= seg.endFrame)) {
                painter.fillRect(gopRect, QColor(255, 255, 255, 30));
            }

            if (m_selectedFrame >= seg.startFrame && m_selectedFrame <= seg.endFrame) {
                painter.setPen(QPen(QColor(255, 255, 255), 2));
                painter.drawRect(gopRect.adjusted(0, 0, -1, -1));
            }

            if (m_gopWidth >= 28) {
                painter.setPen(QColor(15, 18, 24, 170));
                painter.drawText(gopRect.adjusted(2, 2, -2, -2),
                                 Qt::AlignCenter,
                                 QString("G%1\n%2")
                                     .arg(seg.gopIndex + 1)
                                     .arg(seg.size));
            } else if (m_gopWidth >= 16 && ((seg.gopIndex % 5) == 0 || seg.gopIndex == 0 || seg.gopIndex == m_gops.size() - 1)) {
                painter.setPen(QColor(15, 18, 24, 170));
                painter.drawText(gopRect.adjusted(2, 0, -2, 0), Qt::AlignCenter, QString::number(seg.gopIndex + 1));
            }
        }
    }

    painter.setPen(QColor(190, 190, 200));
    painter.drawText(12, laneTop + laneHeight + 24,
                     QString("Frames: %1   GOPs: %2   Zoom: %3x   Hint: Ctrl + Mouse Wheel to zoom")
                         .arg(m_frames.size())
                         .arg(m_gops.size())
                         .arg(m_zoomFactor, 0, 'f', 2));
}

void TimelineView::mousePressEvent(QMouseEvent *event)
{
    if (m_frames.isEmpty()) {
        return;
    }

    const int selected = frameAtPosition(event->pos().x());
    if (selected < 0) {
        return;
    }

    m_selectedFrame = selected;
    emit frameSelected(m_selectedFrame);
    update();
}

void TimelineView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_frames.isEmpty()) {
        return;
    }

    const int hovered = frameAtPosition(event->pos().x());
    if (hovered != m_hoveredFrame) {
        m_hoveredFrame = hovered;
        update();
    }

    if (hovered < 0) {
        return;
    }

    if (m_densityMode == GopCompactMode) {
        for (const GopSegment &seg : m_gops) {
            if (hovered >= seg.startFrame && hovered <= seg.endFrame) {
                QToolTip::showText(event->globalPos(),
                                   QString("GOP: %1\nRange: %2 - %3\nSize: %4\nTime: %5 - %6 s")
                                       .arg(seg.gopIndex + 1)
                                       .arg(seg.startFrame)
                                       .arg(seg.endFrame)
                                       .arg(seg.size)
                                       .arg(seg.startTime, 0, 'f', 3)
                                       .arg(seg.endTime, 0, 'f', 3),
                                   this);
                return;
            }
        }
    }

    for (const FrameInfo &frame : m_frames) {
        if (frame.index == hovered) {
            QToolTip::showText(event->globalPos(),
                               QString("Frame: %1\nTime: %2 s\nType: %3\nGOP: %4")
                                   .arg(frame.index)
                                   .arg(frame.ptsTime, 0, 'f', 4)
                                   .arg(frame.type)
                                   .arg(frame.gopIndex + 1),
                               this);
            break;
        }
    }
}

void TimelineView::leaveEvent(QEvent *)
{
    m_hoveredFrame = -1;
    QToolTip::hideText();
    update();
}

void TimelineView::wheelEvent(QWheelEvent *event)
{
    if (!(event->modifiers() & Qt::ControlModifier)) {
        QWidget::wheelEvent(event);
        return;
    }

    const int anchor = frameAtPosition(event->pos().x());
    if (anchor < 0) {
        event->accept();
        return;
    }

    const int delta = event->angleDelta().y();
    if (delta > 0) {
        emit zoomRequested(1.15, anchor);
    } else if (delta < 0) {
        emit zoomRequested(1.0 / 1.15, anchor);
    }
    event->accept();
}

QSize TimelineView::sizeHint() const
{
    const int canvasCount = (m_densityMode == FrameMode) ? m_frames.size() : qMax(1, m_gops.size());
    const int unitWidth = (m_densityMode == FrameMode) ? m_barWidth : m_gopWidth;
    const int w = qMax(1200, canvasCount * unitWidth);
    return QSize(w, 260);
}

QColor TimelineView::colorForType(QChar type) const
{
    if (type == QChar('I')) {
        return QColor(224, 84, 84);
    }
    if (type == QChar('P')) {
        return QColor(73, 140, 255);
    }
    if (type == QChar('B')) {
        return QColor(79, 184, 116);
    }
    return QColor(145, 145, 145);
}

bool TimelineView::typeVisible(QChar type) const
{
    if (type == QChar('I')) {
        return m_showI;
    }
    if (type == QChar('P')) {
        return m_showP;
    }
    if (type == QChar('B')) {
        return m_showB;
    }
    return true;
}

bool TimelineView::rangeVisible(const FrameInfo &frame) const
{
    if (m_rangeStartFrame >= 0 && frame.index < m_rangeStartFrame) {
        return false;
    }

    if (m_rangeEndFrame >= 0 && frame.index > m_rangeEndFrame) {
        return false;
    }

    if (m_rangeStartTime >= 0.0 && frame.ptsTime < m_rangeStartTime) {
        return false;
    }

    if (m_rangeEndTime >= 0.0 && frame.ptsTime > m_rangeEndTime) {
        return false;
    }

    return true;
}

int TimelineView::frameAtPosition(int x) const
{
    if (m_densityMode == FrameMode) {
        return frameByFramePosition(x / qMax(1, m_barWidth));
    }
    return frameByGopPosition(x / qMax(1, m_gopWidth));
}

int TimelineView::frameAtCanvasPosition(int canvasX) const
{
    if (m_densityMode == FrameMode) {
        return frameByFramePosition(canvasX / qMax(1, m_barWidth));
    }
    return frameByGopPosition(canvasX / qMax(1, m_gopWidth));
}

int TimelineView::canvasPositionForFrame(int frameIndex) const
{
    if (m_frames.isEmpty()) {
        return 0;
    }

    if (m_densityMode == FrameMode) {
        const int offset = qBound(0, frameIndex - m_frames.first().index, m_frames.size() - 1);
        return offset * qMax(1, m_barWidth);
    }

    if (m_gops.isEmpty()) {
        return 0;
    }

    for (const GopSegment &seg : m_gops) {
        if (frameIndex >= seg.startFrame && frameIndex <= seg.endFrame) {
            return seg.gopIndex * qMax(1, m_gopWidth);
        }
    }

    if (frameIndex < m_gops.first().startFrame) {
        return 0;
    }
    return m_gops.last().gopIndex * qMax(1, m_gopWidth);
}

int TimelineView::frameByFramePosition(int framePos) const
{
    if (m_frames.isEmpty() || framePos < 0 || framePos >= m_frames.size()) {
        return -1;
    }
    return m_frames[framePos].index;
}

int TimelineView::frameByGopPosition(int gopPos) const
{
    if (m_gops.isEmpty() || gopPos < 0 || gopPos >= m_gops.size()) {
        return -1;
    }
    return m_gops[gopPos].startFrame;
}

void TimelineView::updateCanvasSize()
{
    m_barWidth = qMax(2, static_cast<int>(4.0 * m_zoomFactor));
    m_gopWidth = qMax(4, static_cast<int>(12.0 * m_zoomFactor));
    const QSize hint = sizeHint();
    setMinimumWidth(hint.width());
    resize(hint.width(), hint.height());
    updateGeometry();
}
