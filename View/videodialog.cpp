#include "videodialog.h"
#include "ui_videodialog.h"

#include "Util/AppConfig.h"

VideoDialog::VideoDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::VideoDialog) {
    ui->setupUi(this);
    timer_ = new QTimer;
    connect(timer_, SIGNAL(timeout()), this, SLOT(SlotTimerTimeOut()));
    timer_->setInterval(500);   // 每隔500毫秒定时器超时

    player_ = new AVPlayer;
    connect(player_, SIGNAL(SIG_getOneImage(QImage)), this, SLOT(slot_refreshImage(QImage)));
    connect(player_, SIGNAL(SIG_TotalTime(qint64)), this, SLOT(SlotGetTotalTime(qint64)));
    connect(player_, SIGNAL(SIG_PlayerStateChanged(int)), this, SLOT(SlotPlayerStateChanged(int)));
    connect( ui->slider_process, SIGNAL(SIG_valueChanged(int)), this, SLOT(SlotVideoSliderValueChanged(int)));

    SlotPlayerStateChanged(kStop);
}

VideoDialog::~VideoDialog()
{
    if (player_ && player_->isRunning()) {
        player_->terminate();
        player_->wait(100);
    }
    delete ui;
}

void VideoDialog::slot_refreshImage(QImage image)
{
    /**
     * @todo 调整画面显示，使播放控件与画面比例相当
     */
    ui->label->setPixmap(QPixmap::fromImage(image));
}

void VideoDialog::SlotPlayerStateChanged(int state)
{
    switch (state) {
        case kStop:
            cout << "AVPlayer::Stop";
            timer_->stop();
            // 恢复UI信息
            ui->slider_process->setValue(0);
            ui->lb_total_time->setText("00:00:00");
            ui->lb_current_time->setText("00:00:00");
            ui->lb_video_filename->setText("0");
            this->update();
            is_stop = true;
        break;
        case kPlaying:
            cout << "AVPlayer::Playing";
            timer_->start();
            this->update();
            is_stop = false;
        break;
    }
}

void VideoDialog::SlotTimerTimeOut() {
    if (QObject::sender() == timer_) {
        qint64 sec = player_->GetCurrentTime() / 1000000;
        ui->slider_process->setValue(sec);
        QString hh = QString("00%1").arg(sec / 3600);
        QString mm = QString("00%1").arg(sec / 60 % 60);
        QString ss = QString("00%1").arg(sec % 60);
        QString time = QString("%1:%2:%3").arg(hh.right(2)).arg(mm.right(2)).arg(ss.right(2));
        ui->lb_current_time->setText(time);
        if(ui->slider_process->value() == ui->slider_process->maximum() && player_->player_state_ == kStop) {
            SlotPlayerStateChanged(kStop);
        } else if(ui->slider_process->value() + 1 == ui->slider_process->maximum() && player_->player_state_ == kStop) {
            SlotPlayerStateChanged(kStop);
        }
    }
}

void VideoDialog::SlotVideoSliderValueChanged(int value)
{
    if( QObject::sender() == ui->slider_process) {
        cout << "changed value:" << value;
        player_->Seek((qint64)value*1000000); //value 秒
    }
}

void VideoDialog::on_pb_resume_clicked()
{
    if (is_stop) player_->start();
    else player_->Play();
}


void VideoDialog::on_pb_pause_clicked()
{
    player_->Pause();
}


void VideoDialog::on_pb_stop_clicked()
{
    player_->Stop(true);
}


void VideoDialog::on_pb_openfile_clicked()
{
    player_->Stop( true );
    //打开文件 弹出对话框 参数:父窗口, 标题, 默认路径, 筛选器
    QString path = QFileDialog::getOpenFileName(this,"选择要播放的文件" , "C:/Users/28568/Desktop/",
                                                "视频文件 (*.flv *.rmvb *.avi *.mp4 *.mkv);; 所有文件(*.*);;");
    if(!path.isEmpty()) {
        cout << path;
        QFileInfo info(path);
        if(info.exists()) {
            player_->Stop( true ); //如果播放 你要先关闭

            player_->set_filename(path);
            ui->lb_video_filename->setText(info.baseName());
            SlotPlayerStateChanged(kPlaying);
        } else {
            QMessageBox::information( this, "提示" , "打开文件失败");
        }
    }
}

void VideoDialog::SlotGetTotalTime(qint64 usec) {
    qint64 sec = usec / 1000000;
    ui->slider_process->setRange(0, sec);   // 初始化进度条范围

    QString hh = QString("00%1").arg(sec / 3600);
    QString mm = QString("00%1").arg(sec / 60 % 60);
    QString ss = QString("00%1").arg(sec % 60);
    QString time = QString("%1:%2:%3").arg(hh.right(2)).arg(mm.right(2)).arg(ss.right(2));
    ui->lb_total_time->setText(time);
}

