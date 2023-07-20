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
#include "Util/AppConfig.h"

class AVPlayer;

struct AVState {
    AVFormatContext *av_fmt_ctx     = nullptr;  // 文件音视频流上下文
    /////////////////////////VIDEO//////////////////////////////
    int video_stream_index          = -1;       // 解码器需要的视频流索引
    AVStream* video_stream          = nullptr;  // 视频流
    PacketQueue* video_que          = nullptr;  // 视频帧缓冲队列
    AVCodecContext* video_dec_ctx   = nullptr;  // 视频解码器上下文
    AVCodec* video_dec_codec        = nullptr;  // DEPRECATED: 视频解码器
    double video_clock              = 0;        // 视频时钟
    SDL_Thread* video_tid           = nullptr;  // 视频线程id
    /////////////////////////AUDIO//////////////////////////////
    int audio_stream_index          = -1;       // 解码器需要的音频流索引
    AVCodecContext* audio_dec_ctx   = nullptr;  // 存储音频解码器的上下文信息, 包含了解码器的参数、状态以及相关的信息。
    AVCodec *audio_dec_codec        = nullptr;  // 音频解码器
    AVStream* audio_stream          = nullptr;  // 音频流
    PacketQueue* audio_que          = nullptr;  // 音频缓存队列
    double audio_clock              = 0;        // 音频时钟
    SDL_AudioDeviceID audioID       = 0;        // 音频id
    AVFrame out_frame;                          // 设置参数，供音频解码后的 swr_alloc_set_opts 使用。
    uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
    unsigned int audio_buf_size = 0;
    unsigned int audio_buf_index = 0;
    /////////////////////////播放控制////////////////////////////
    bool is_pause                   = false; // 暂停标志
    bool is_quit                    = false; // 停止标志
    bool is_read_frame_finished     = false; // run线程读音视频流上下文是否读取完毕
    bool is_run_finished            = true; // run读取线程是否结束
    bool is_video_thread_finished   = true; // 视频线程是否结束
    int seek_req; //跳转标志 -- 读线程
    int64_t seek_pos; //跳转的位置 -- 微秒
    int seek_flag_audio;//跳转标志 -- 用于音频线程中
    int seek_flag_video;//跳转标志 -- 用于视频线程中
    double seek_time; //跳转的时间(秒) 值和 seek_pos 是一样的
    int64_t start_time              = 0;        // 单位:微秒
    ////////////////////////////////////////////////////////////
    AVPlayer* player                = nullptr;
};

enum PlayerState {
    /// 播放
    kPlaying = 0,
    /// 暂停
    kPause   = 1,
    /// 停止
    kStop    = 2
};

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
    /**
     * @brief SIG_PlayerStateChanged  播放状态改变信号
     * @param state
     */
    void SIG_PlayerStateChanged(int state);
    /**
     * @brief SIG_TotalTime 发送音视频总时长(微秒)信号
     * @param usec
     */
    void SIG_TotalTime(qint64 usec);
public:
    /**
     * @brief setFileName filename的setter方法
     * @param newFileName 文件名
     */
    void set_filename(const QString &filename);

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
    static void AudioCallBack(void *userdata, Uint8 *stream, int len);
    /**
     * @brief video_thread 视频解码线程函数
     * @param arg
     * @return
     */
    static int VideoDecodeThread(void *arg);
    /**
     * @brief AVPlayer::audio_decode_frame 音频解码函数
     * @param[in] pcodec_ctx 音频解码器上下文
     * @param[in,out] audio_buf 存储解码后的数据
     * @param buf_size
     * @return
     */
    static int audio_decode_frame(AVState *is, uint8_t *audio_buf, int buf_size);
    /**
     * @brief 查找输入文件中的音频流和视频流的索引
     * @param[in] pformat_ctx       音视频流上下文
     * @param[in,out] video_stream  视频流索引
     * @param[in,out] audio_stream  音频流索引
     */
    static void find_av_stream_index(AVFormatContext* pformat_ctx, int& video_stream, int& audio_stream);
public:
    /**
     * @brief SendGetOneImage 发送图片信号
     * @param img
     */
    void SendGetOneImage(QImage& img);
public:
    /**
     * @brief Pause 播放控制-暂停
     */
    void Pause();
    /**
     * @brief Play 播放控制-继续播放
     */
    void Play();
    /**
     * @brief Stop 播放控制-视频停止
     */
    void Stop(bool is_wait);
public:
    /**
     * @brief SynchronizeVideo 时间补偿函数--视频延时
     * @param is
     * @param src_frame
     * @param pts
     * @return
     */
    static double SynchronizeVideo(AVState* is, AVFrame* src_frame, double pts);
    /**
     * @brief GetCurrentTime 获取当前时钟(微秒)
     * @return 返回音频时钟
     */
    double GetCurrentTime();
    /**
     * @brief GetTotalTime 获取总时间(微秒)
     * @return 音视频上下文存在返回duration属性；否则返回-1
     */
    int64_t GetTotalTime();
public:
    /// @brief 文件名
    QString filename_;
    /// @brief 解码音频视频所有相关信息
    AVState* av_state_;
    /// @brief 播放状态
    PlayerState player_state_;
};

#endif // AVPLAYER_H
