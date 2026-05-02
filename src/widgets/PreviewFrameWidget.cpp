#include "PreviewFrameWidget.h"

#include <QLabel>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVBoxLayout>

PreviewFrameWidget::PreviewFrameWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(160);
    setMaximumHeight(320);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(0);

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_label->setStyleSheet(
        "QLabel { background: #111; color: #777; border: 1px solid #333; font-size: 12px; }");
    layout->addWidget(m_label);

    showPlaceholder();
}

void PreviewFrameWidget::showPlaceholder(const QString &text)
{
    m_hasImage = false;
    m_pixmap   = QPixmap();
    m_label->clear();
    m_label->setText(text.isEmpty() ? "Select a frame to preview" : text);
}

void PreviewFrameWidget::showLoading()
{
    m_hasImage = false;
    m_pixmap   = QPixmap();
    m_label->clear();
    m_label->setText("Loading preview...");
}

void PreviewFrameWidget::showImage(const QString &imagePath)
{
    const QPixmap px(imagePath);
    if (px.isNull()) {
        showError("Failed to load captured frame");
        return;
    }
    m_pixmap   = px;
    m_hasImage = true;
    m_label->setText(QString());
    applyScaledPixmap();
}

void PreviewFrameWidget::showError(const QString &errorText)
{
    m_hasImage = false;
    m_pixmap   = QPixmap();
    m_label->clear();
    m_label->setText("Preview unavailable:\n" + errorText);
}

void PreviewFrameWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_hasImage) {
        applyScaledPixmap();
    }
}

void PreviewFrameWidget::applyScaledPixmap()
{
    if (m_pixmap.isNull() || !m_label) {
        return;
    }
    const QSize available = m_label->size();
    if (available.isEmpty()) {
        return;
    }
    m_label->setPixmap(
        m_pixmap.scaled(available, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
