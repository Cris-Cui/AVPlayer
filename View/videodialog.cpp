#include "videodialog.h"
#include "ui_videodialog.h"

#include "Util/AppConfig.h"

VideoDialog::VideoDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::VideoDialog) {
    ui->setupUi(this);
    m_player = new AVPlayer;
    // QString file_name = FILE_NAME;
    // m_player->set_filename(file_name);
    connect(m_player, SIGNAL(SIG_getOneImage(QImage)), this, SLOT(slot_refreshImage(QImage)));
}

VideoDialog::~VideoDialog()
{
    if (m_player && m_player->isRunning()) {
        m_player->terminate();
        m_player->wait(100);
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
            is_stop = true;
        break;
        case kPlaying:
            cout << "AVPlayer::Playing";
            is_stop = false;
        break;
    }
}

void VideoDialog::on_pb_resume_clicked()
{
    if (is_stop) m_player->start();
    else m_player->Play();
}


void VideoDialog::on_pb_pause_clicked()
{
    m_player->Pause();
}


void VideoDialog::on_pb_stop_clicked()
{
    m_player->Stop(true);
}


void VideoDialog::on_pb_openfile_clicked()
{
    m_player->Stop( true );
    //打开文件 弹出对话框 参数:父窗口, 标题, 默认路径, 筛选器
    QString path = QFileDialog::getOpenFileName(this,"选择要播放的文件" , "C:/Users/28568/Desktop/",
                                                "视频文件 (*.flv *.rmvb *.avi *.mp4 *.mkv);; 所有文件(*.*);;");
    if(!path.isEmpty()) {
        cout << path;
        QFileInfo info(path);
        if(info.exists()) {
            m_player->Stop( true ); //如果播放 你要先关闭

            m_player->set_filename(path);
            ui->lb_video_filename->setText(info.baseName());
            // slot_PlayerStateChanged(PlayerState::Playing);
        } else {
            QMessageBox::information( this, "提示" , "打开文件失败");
        }
    }
}

