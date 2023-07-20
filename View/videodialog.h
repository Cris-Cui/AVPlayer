#ifndef VIDEODIALOG_H
#define VIDEODIALOG_H

#include <QDialog>
#include <QImage>
#include <QPixmap>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>
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
     * @brief on_pb_resume_clicked 恢复播放按钮点击函数
     */
    void on_pb_resume_clicked();
    /**
     * @brief on_pb_pause_clicked 暂停按钮点击函数
     */
    void on_pb_pause_clicked();
    /**
     * @brief on_pb_stop_clicked 停止按钮点击函数
     */
    void on_pb_stop_clicked();
    /**
     * @brief on_pb_openfile_clicked 打开视频文件按钮点击函数
     */
    void on_pb_openfile_clicked();

public slots:
    /**
     * @brief SlotGetTotalTime 获取视频全部时间
     * @param usec 时间, 单位微秒
     */
    void SlotGetTotalTime(qint64 usec);
    /**
     * @brief SlotTimerTimeOut 定时器超时响应
     */
    void SlotTimerTimeOut();
    /**
     * @brief SlotVideoSliderValueChanged 播放进度条跳转
     * @param value 跳转到的秒数
     */
    void SlotVideoSliderValueChanged(int value);
private:
    Ui::VideoDialog *ui;
    /// 音视频处理播放者
    AVPlayer* player_;
    bool is_stop = false;
    /// 定时器
    QTimer* timer_;
};
#endif // VIDEODIALOG_H
