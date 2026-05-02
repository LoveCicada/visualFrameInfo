#ifndef PREVIEWFRAMEWIDGET_H
#define PREVIEWFRAMEWIDGET_H

#include <QPixmap>
#include <QWidget>

class QLabel;

// Displays a single video frame thumbnail extracted by ffmpeg.
// Supports placeholder, loading, image, and error states.
class PreviewFrameWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewFrameWidget(QWidget *parent = nullptr);

    void showPlaceholder(const QString &text = QString());
    void showLoading();
    void showImage(const QString &imagePath);
    void showError(const QString &errorText);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void applyScaledPixmap();

    QLabel  *m_label   = nullptr;
    QPixmap  m_pixmap;
    bool     m_hasImage = false;
};

#endif // PREVIEWFRAMEWIDGET_H
