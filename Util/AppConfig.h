#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QDebug>

#define cout qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]"

#define FILE_NAME ("C:/HuaweiShare/Backup/HuaweiVideos/nova7se/22.mp4")
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 //1 second of 48khz 32bit audio
#define SDL_AUDIO_BUFFER_SIZE 1024

#endif // APPCONFIG_H
