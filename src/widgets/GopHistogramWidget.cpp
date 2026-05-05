#include "GopHistogramWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

GopHistogramWidget::GopHistogramWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumHeight(100);
}

void GopHistogramWidget::setGops(const QVector<GopSegment> &gops)
{
    m_gops = gops;
    rebuild();
    update();
}

void GopHistogramWidget::rebuild()
{
    m_buckets.clear();
    m_maxCount = 0;
    m_hoveredBucket = -1;

    if (m_gops.isEmpty()) {
        return;
    }

    int minSize = m_gops.first().size;
    int maxSize = m_gops.first().size;
    for (const GopSegment &g : m_gops) {
        minSize = qMin(minSize, g.size);
        maxSize = qMax(maxSize, g.size);
    }

    const int range = maxSize - minSize;
    const int bucketCount = (range == 0) ? 1
                                         : qBound(5, m_gops.size() / 4, 20);
    const double bucketWidth = (range == 0) ? 1.0
                                            : static_cast<double>(range + 1) / bucketCount;

    m_buckets.resize(bucketCount);
    for (int i = 0; i < bucketCount; ++i) {
        m_buckets[i].minSize = minSize + static_cast<int>(i * bucketWidth);
        m_buckets[i].maxSize = (i == bucketCount - 1)
                                   ? maxSize
                                   : minSize + static_cast<int>((i + 1) * bucketWidth) - 1;
        m_buckets[i].count = 0;
    }

    for (const GopSegment &g : m_gops) {
        int idx = (range == 0) ? 0
                               : static_cast<int>((g.size - minSize) / bucketWidth);
        idx = qBound(0, idx, bucketCount - 1);
        m_buckets[idx].count++;
        m_buckets[idx].gopIndices.append(g.gopIndex + 1);
    }

    for (const Bucket &b : m_buckets) {
        m_maxCount = qMax(m_maxCount, b.count);
    }
}

int GopHistogramWidget::bucketAt(int x) const
{
    if (m_buckets.isEmpty()) {
        return -1;
    }
    const int leftMargin  = 30;
    const int rightMargin = 8;
    const int plotW = width() - leftMargin - rightMargin;
    if (plotW <= 0) {
        return -1;
    }
    const double bw = static_cast<double>(plotW) / m_buckets.size();
    const int idx = static_cast<int>((x - leftMargin) / bw);
    return (idx >= 0 && idx < m_buckets.size()) ? idx : -1;
}

void GopHistogramWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(26, 31, 40));

    if (m_buckets.isEmpty()) {
        painter.setPen(QColor(160, 160, 170));
        painter.drawText(rect(), Qt::AlignCenter, "No GOP data");
        return;
    }

    const int leftMargin  = 30;
    const int rightMargin = 8;
    const int topMargin   = 8;
    const int bottomMargin = 20;

    const int plotW = width() - leftMargin - rightMargin;
    const int plotH = height() - topMargin - bottomMargin;
    if (plotW <= 0 || plotH <= 0) {
        return;
    }

    // Y-axis ticks
    painter.setPen(QColor(90, 100, 120));
    painter.drawLine(leftMargin, topMargin, leftMargin, topMargin + plotH);
    painter.drawLine(leftMargin, topMargin + plotH, leftMargin + plotW, topMargin + plotH);

    // draw a few Y gridlines
    const int ySteps = qMin(m_maxCount, 5);
    painter.setFont(QFont(font().family(), 7));
    for (int i = 0; i <= ySteps; ++i) {
        const int y = topMargin + plotH - (plotH * i / ySteps);
        painter.setPen(QColor(55, 65, 80));
        painter.drawLine(leftMargin, y, leftMargin + plotW, y);
        painter.setPen(QColor(140, 148, 160));
        painter.drawText(0, y - 5, leftMargin - 3, 14,
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(m_maxCount * i / ySteps));
    }

    // bars
    const double bw = static_cast<double>(plotW) / m_buckets.size();
    const QColor barColor(100, 170, 230);
    const QColor barHover(160, 210, 255);

    for (int i = 0; i < m_buckets.size(); ++i) {
        const Bucket &b = m_buckets[i];
        if (b.count == 0) {
            continue;
        }
        const int barH = (m_maxCount > 0) ? (plotH * b.count / m_maxCount) : 0;
        const int bx = leftMargin + static_cast<int>(i * bw) + 1;
        const int bWidth = qMax(1, static_cast<int>(bw) - 2);
        const int by = topMargin + plotH - barH;
        const QColor color = (i == m_hoveredBucket) ? barHover : barColor;
        painter.fillRect(bx, by, bWidth, barH, color);
    }

    // X-axis labels — only first, middle and last bucket
    painter.setPen(QColor(140, 148, 160));
    painter.setFont(QFont(font().family(), 7));
    QList<int> labelIdx = {0, m_buckets.size() / 2, m_buckets.size() - 1};
    for (int i : labelIdx) {
        if (i < 0 || i >= m_buckets.size()) {
            continue;
        }
        const int bx = leftMargin + static_cast<int>(i * bw);
        const int bWidth = static_cast<int>(bw);
        painter.drawText(bx, topMargin + plotH + 2, bWidth, bottomMargin - 2,
                         Qt::AlignCenter, QString::number(m_buckets[i].minSize));
    }
}

void GopHistogramWidget::mouseMoveEvent(QMouseEvent *event)
{
    const int idx = bucketAt(event->pos().x());
    if (idx != m_hoveredBucket) {
        m_hoveredBucket = idx;
        update();
    }
    if (idx >= 0 && idx < m_buckets.size()) {
        const Bucket &b = m_buckets[idx];
        QString tip = QString("Size: %1 - %2\nCount: %3\nGOPs: ")
                          .arg(b.minSize).arg(b.maxSize).arg(b.count);
        const int maxShow = 20;
        QStringList nums;
        for (int i = 0; i < qMin(b.gopIndices.size(), maxShow); ++i) {
            nums.append(QString::number(b.gopIndices[i]));
        }
        if (b.gopIndices.size() > maxShow) {
            nums.append(QString("(+%1 more)").arg(b.gopIndices.size() - maxShow));
        }
        tip += nums.join(", ");
        QToolTip::showText(event->globalPos(), tip, this);
    }
}

void GopHistogramWidget::leaveEvent(QEvent *)
{
    m_hoveredBucket = -1;
    update();
}

QSize GopHistogramWidget::sizeHint() const
{
    return {200, 120};
}
