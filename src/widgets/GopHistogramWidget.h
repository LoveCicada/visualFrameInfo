#ifndef GOPHISTOGRAMWIDGET_H
#define GOPHISTOGRAMWIDGET_H

#include <QVector>
#include <QWidget>

#include "../models/GopSegment.h"

class GopHistogramWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GopHistogramWidget(QWidget *parent = nullptr);

    void setGops(const QVector<GopSegment> &gops);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    QSize sizeHint() const override;

private:
    struct Bucket {
        int minSize = 0;
        int maxSize = 0;
        int count = 0;
        QVector<int> gopIndices; // 1-based for display
    };

    void rebuild();
    int bucketAt(int x) const;

    QVector<GopSegment> m_gops;
    QVector<Bucket> m_buckets;
    int m_hoveredBucket = -1;
    int m_maxCount = 0;
};

#endif // GOPHISTOGRAMWIDGET_H
