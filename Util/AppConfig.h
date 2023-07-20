#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QDebug>

#define cout qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]"

#define FILE_NAME ("C:/Users/28568/Desktop/1.mp4")

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 //1 second of 48khz 32bit audio
#define SDL_AUDIO_BUFFER_SIZE 1024

#define MAX_AUDIO_SIZE (1024*16*25*10)  //音频阈值
#define MAX_VIDEO_SIZE (1024*255*25*2)  //视频阈值

#define FLUSH_DATA ("FLUSH")

#endif // APPCONFIG_H
