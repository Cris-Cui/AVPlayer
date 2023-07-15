#ifndef AVPLAYER_H
#define AVPLAYER_H

#include <QThread>
#include <QImage>

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/pixfmt.h"
    #include "libswscale/swscale.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/time.h"
    #include <SDL.h>
}

#include "Model/DataStructure/packetqueue.h"

class AVPlayer : public QThread
{
    Q_OBJECT
public:
    explicit AVPlayer(QObject *parent = nullptr);
signals:
    /**
     * @brief SIG_getOneImage 获得视频帧信号
     * @param img 视频帧, 跨线程操作, 不能传引用
     */
    void SIG_getOneImage(QImage img);
public:
    /**
     * @brief setFileName filename的setter方法
     * @param newFileName 文件名
     */
    void setFileName(const QString &filename);

    void TestPlayAudio();
private:
    /**
     * @brief run 线程入口函数
     */
    void run();
    /**
     * @brief 音频SDL回调函数
     * @param[in]     userdata 音频解码器上下文
     * @param[in,out] stream   要播放的缓冲区
     * @param[in,out] len      填充stream的长度
     */
    static void audio_callback(void *userdata, Uint8 *stream, int len);
    /**
     * @brief AVPlayer::audio_decode_frame 音频解码函数
     * @param[in] pcodec_ctx 音频解码器上下文
     * @param[in,out] audio_buf 存储解码后的数据
     * @param buf_size
     * @return
     */
    static int audio_decode_frame(AVCodecContext *pcodec_ctx, uint8_t *audio_buf, int buf_size);
    /**
     * @brief 查找输入文件中的音频流和视频流的索引
     * @param[in] pformat_ctx       音视频流上下文
     * @param[in,out] video_stream  视频流索引
     * @param[in,out] audio_stream  音频流索引
     */
    static void find_av_stream_index(AVFormatContext* pformat_ctx, int& video_stream, int& audio_stream);
private:
    static AVFrame wanted_frame;
    static PacketQueue audio_queue;
    static int quit;
    /// @brief 文件名
    QString filename_;
};

#endif // AVPLAYER_H
