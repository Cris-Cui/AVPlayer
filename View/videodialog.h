#ifndef VIDEODIALOG_H
#define VIDEODIALOG_H

#include <QDialog>
#include <QImage>
#include <QPixmap>
#include "Controller/avplayer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class VideoDialog; }
QT_END_NAMESPACE

class VideoDialog : public QDialog
{
    Q_OBJECT

public:
    VideoDialog(QWidget *parent = nullptr);
    ~VideoDialog();

private slots:
    void slot_refreshImage(QImage);
    void on_pushButton_clicked();

private:
    Ui::VideoDialog *ui;
    AVPlayer* m_player;
};
#endif // VIDEODIALOG_H
