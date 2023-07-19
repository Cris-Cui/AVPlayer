#include "videodialog.h"
#include "ui_videodialog.h"

#include "Util/AppConfig.h"

VideoDialog::VideoDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VideoDialog)
{
    ui->setupUi(this);
    m_player = new AVPlayer;
    QString file_name = FILE_NAME;
    m_player->set_fileName(file_name);
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

void VideoDialog::on_pb_start_clicked()
{
    m_player->start();
}


void VideoDialog::on_pb_resume_clicked()
{
    m_player->Play();
}


void VideoDialog::on_pb_pause_clicked()
{
    m_player->Pause();
}


void VideoDialog::on_pb_stop_clicked()
{
    m_player->Stop(true);
}

