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

    void SlotPlayerStateChanged(int state);
    /**
     * @brief on_pb_start_clicked 开始按钮点击函数
     */
    void on_pb_start_clicked();
    /**
     * @brief on_pb_resume_clicked 恢复播放按钮点击函数
     */
    void on_pb_resume_clicked();
    /**
     * @brief on_pb_pause_clicked 暂停按钮点击函数
     */
    void on_pb_pause_clicked();

    void on_pb_stop_clicked();

private:
    Ui::VideoDialog *ui;
    AVPlayer* m_player;
    bool is_stop;
};
#endif // VIDEODIALOG_H
