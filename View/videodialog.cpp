#include "videodialog.h"
#include "ui_videodialog.h"

#include "Util/AppConfig.h"

VideoDialog::VideoDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VideoDialog)
{
    ui->setupUi(this);
    m_player = new AVPlayer;
    m_player->setFileName(FILE_NAME);
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


void VideoDialog::on_pushButton_clicked()
{
    m_player->start();
}

