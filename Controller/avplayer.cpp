#include "avplayer.h"
#include "Util/AppConfig.h"

// 静态成员初始化
AVFrame AVPlayer::wanted_frame;
PacketQueue AVPlayer::audio_queue;
int AVPlayer::quit = 0;

AVPlayer::AVPlayer(QObject *parent) : QThread(parent)
{
    cout << avcodec_configuration();
}

void AVPlayer::setFileName(const QString &filename)
{
    filename_ = filename;
}

/**
 * 编码格式: H264 ——decode——> YUV ————> RGB
 * 数据类型: AVPacket       AVFrame   AVFrame
 * 变量名称: packet         pFrame    pFrameRGB
 */
void AVPlayer::TestPlayAudio() {
    /// @todo 0. 申请变量
    AVFormatContext *pFormatCtx = nullptr;  // AV 文件视频流的”文件指针”
    int audioStream             = -1;       // 解码器需要的音频流索引
    int videoStream             = -1;       // 解码器需要的视频流索引
    AVCodecContext *pCodecCtx   = nullptr;  // 存储音频解码器的上下文信息, 包含了解码器的参数、状态以及相关的信息。
    AVCodec *pCodec             = nullptr;  // 音频解码器
    AVPacket packet ;                       // 解码前的压缩数据(视频对应H.264等码流数据，音频对应AAC/MP3等码流数据)
    AVFrame *pframe             = nullptr;  // 解码之后的非压缩数据(视频对应RGB/YUV像素数据，音频对应PCM采样数据)
    char filename[256]          = FILE_NAME;
    /* ---SDL--- */
    SDL_AudioSpec wanted_spec; //SDL 音频设置
    SDL_AudioSpec spec;        //SDL 音频设置
    /// @todo 1. 初始化 FFmpeg 和 SDL。
    /**
     * av_register_all(): 初始化FFMPEG(已弃用)
     * 初始化FFmpeg库，注册所有可用的编解码器和其他相关组件。
     * 自 FFmpeg 4.0 起，不再需要显式调用 av_register_all() 函数来注册容器、解码器和编码器。相反，FFmpeg 库会自动在首次使用相应组件时进行注册。
     */
    av_register_all();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        cout << "Couldn't init SDL:" << SDL_GetError();
        exit(-1);
    }
    /// @todo 2. 打开音频文件，并获取音频流的索引。
    /**
     * 函数原型: int avformat_open_input(AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options);
     * 函数作用: 用于打开媒体文件并初始化 AVFormatContext 结构体。它是解码和处理音视频文件的入口点之一。
     * 参数说明:
     *  ps：一个指向 AVFormatContext 指针的指针。该函数将分配并填充一个 AVFormatContext 结构体，并将其赋值给 *ps。
     *  url：要打开的媒体文件的URL或文件名。可以是本地文件路径，也可以是网络上的 URL。
     *  fmt：强制指定要使用的输入格式。通常情况下，可以将其设置为 NULL，让 FFmpeg 根据文件名自动猜测输入格式。
     *  options：附加的选项，以字典的形式传递。可以用于设置解码器参数、输入格式参数等。
     * 返回值:
     *  成功打开媒体文件时，返回零或正数作为流的索引值。
     *  打开失败时，返回一个负数错误代码，表示打开文件出现错误。
     */
    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        cout << "Couldn't open input file";
        exit(-1);
    }
    /**
     * 函数原型: int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options);
     * 函数作用: 获取输入文件的流信息，如音频流、视频流等，并存储在AVFormatContext结构体中。
     * 参数说明:
     *  ic:      输入文件的AVFormatContext结构体指针。
     *  options: 可选参数，用于设置附加选项，通常为NULL。
     * 返回值:
     *  0：  成功获取文件流信息。
     *  < 0：获取文件流信息失败。
     */
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        cout << "Not Found Stream Info";
        exit(-1);
    }
    /**
     * 函数原型: void av_dump_format(AVFormatContext *ic, int index, const char *url, int is_output);
     * 函数作用: 打印输入或输出文件的详细信息。
     * 参数说明:
     *  ic: 输入文件的AVFormatContext结构体指针。
     *  index: 输入文件的流索引，通常为-1表示打印所有流。
     *  url: 输入文件的URL或文件名。
     *  is_output: 标志参数，用于指示是否为输出文件。
     * 返回值: 无
     */
    av_dump_format(pFormatCtx, 0, filename, false);             // 显示文件信息，十分好用的一个函数
    find_av_stream_index(pFormatCtx, videoStream, audioStream); // 获取音频流的索引
    if (audioStream == -1) {
        cout << "Couldn't find audioStream index";
        exit(-1);
    }
    cout << "audio_stream = " << audioStream;                   // 打印音频流索引
    /// @todo 3. 找到对应的音频解码器并打开解码器。
    /**
     * pFormatCtx->streams[audioStream]: 通过 audioStream 变量获取音频流的指针，pFormatCtx 是一个 AVFormatContext 结构体指针，存储了文件的音视频流信息。
     * pCodecCtx = ...->codec：获取音频流的编解码器上下文，并将其赋值给 pCodecCtx，即 AVCodecContext 结构体指针。
     * pCodecCtx->codec_id：从解码器上下文中获取编解码器的 ID。
     * 根据编解码器的 ID 查找对应的解码器，并将其赋值给 pCodec
     */
    pCodecCtx = pFormatCtx->streams[audioStream]->codec;
    /**
     * 函数原型: AVCodec* avcodec_find_decoder(enum AVCodecID id);
     * 函数作用: 在 FFmpeg 库中进行解码器的查找工作。
     * 参数说明:
     *  id: 解码器的id
     * 返回值:
     *  返回查找到的解码器
     */
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (!pCodec) {
        cout << "Couldn't find decoder";
        exit(-1);
    }
    /**
     * 函数原型: int avcodec_open2(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);
     * 函数作用: 将解码器参数设置到解码器上下文中，并初始化解码器的状态。
     * 参数说明:
     *  avctx：指向解码器上下文 (AVCodecContext) 的指针。解码器上下文包含了解码器的各种参数和状态信息。
     *  codec：指向要打开的解码器 (AVCodec) 的指针。该解码器将用于解码音视频数据。
     *  options：指向 AVDictionary 字典指针的指针，用于传递额外的选项参数给解码器。可以设置为 NULL，表示不使用额外选项。
     * 返回值:
     *  返回值大于等于 0 表示成功，小于 0 表示打开解码器失败。
     * @details
     *  函数的工作流程如下：
     *      1. 检查输入的解码器上下文指针 avctx 是否为 NULL，如果是则返回错误。
     *      2. 检查输入的解码器指针 codec 是否为 NULL，如果是则返回错误。
     *      3. 检查解码器上下文的 codec 字段是否已经设置为有效的解码器。如果已经设置，表示解码器已经打开，直接返回成功。
     *      4. 调用解码器的 init() 函数，对解码器上下文进行初始化。
     *      5. 将解码器的参数设置到解码器上下文中。这些参数包括解码器的采样率、声道数、音频格式、图像大小等。
     *      6. 调用解码器的 open() 函数，打开解码器并初始化解码器的状态。
     *      7. 返回打开解码器的结果，大于等于 0 表示成功，小于 0 表示打开解码器失败。
     */
    avcodec_open2(pCodecCtx, pCodec, NULL);
    /// @todo 4. 初始化音频队列。
    packet_queue_init(&audio_queue);
    /// @todo 5. 设置音频设备参数，并打开音频设备。
    wanted_spec.freq     = pCodecCtx->sample_rate;  // 设置音频的采样率为解码器上下文 (pCodecCtx) 的采样率。
    wanted_spec.format   = AUDIO_S16SYS;            // 设置音频的样本格式为 AUDIO_S16SYS，表示每个样本为有符号的16位整数。
    wanted_spec.channels = pCodecCtx->channels;     // 设置音频的声道数为解码器上下文 (pCodecCtx) 的声道数。
    wanted_spec.silence  = 0;                       // 设置静音值为 0，表示没有静音。
    wanted_spec.samples  = SDL_AUDIO_BUFFER_SIZE;   // 设置音频缓冲区的大小为 SDL_AUDIO_BUFFER_SIZE，用于存储解码后的音频数据。读取第一帧后调整
    wanted_spec.callback = audio_callback;          // 设置音频回调函数为 audio_callback，用于从音频队列中获取数据。
    wanted_spec.userdata = pCodecCtx;               // 将解码器上下文 (pCodecCtx) 作为回调函数的用户数据传递，以便在回调函数中进行解码操作。
    /**
     * wanted_spec:想要打开的
     * spec:实际打开的,可以不用这个，函数中直接用 NULL,下面用到 spec 用 wanted_spec 代替。
     * 这里会开一个线程，调用 callback。
     * SDL_OpenAudioDevice->open_audio_device(开线程)->SDL_RunAudio->fill(指向 callback 函数)
     */
    /**
     * 函数原型: SDL_AudioDeviceID SDL_OpenAudioDevice(const char *device, int iscapture, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int allowed_changes);
     * 函数作用: 打开音频设备并配置音频参数
     * 参数说明:
     *  device: 字符串类型，指定要打开的音频设备的名称。如果为 NULL，则使用默认音频设备。
     *  iscapture: 整数类型，用于指定是否为录制音频设备。值为 0 表示播放音频设备，非 0 表示录制音频设备。
     *  desired: SDL_AudioSpec 结构体指针，指定所需的音频参数，包括采样率、音频格式、声道数等。
     *  obtained: SDL_AudioSpec 结构体指针，用于返回实际打开的音频设备的参数，包括实际的采样率、音频格式、声道数等。
     *  allowed_changes: 整数类型，指定允许的参数变化。如果为 0，则只接受完全匹配的参数。如果为负数，则接受任何参数变化。如果为正数，则表示允许的最大变化次数。
     * @details
     *  SDL_GetAudioDeviceName(0,0) 获取默认音频设备的名称。
     *  0: 输入设备 ID，用于录制音频，此处设置为 0 表示不使用输入设备。
     *  &wanted_spec: 所需的音频参数结构体，包括采样率、音频格式、声道数等。
     *  &spec: 实际打开的音频设备的参数结构体，用于返回实际打开的音频参数。
     *  0: 附加的打开选项。
     */
    SDL_AudioDeviceID id = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0,0), 0, &wanted_spec, &spec, 0);
    if( id < 0 ) { //第二次打开 audio 会返回-1
        cout << "Couldn't open Audio:" << SDL_GetError();
        exit(-1);
    }
    /// @todo 6. 设置参数，供解码时候用, swr_alloc_set_opts 的 in 部分参数
    wanted_frame.format         = AV_SAMPLE_FMT_S16;    // 设置解码后音频的样本格式为 AV_SAMPLE_FMT_S16，表示每个样本为有符号的16位整数。
    wanted_frame.sample_rate    = spec.freq;            // 设置解码后音频的采样率为实际打开的音频设备的采样率。
    wanted_frame.channel_layout = av_get_default_channel_layout(spec.channels); //设置解码后音频的声道布局为实际打开的音频设备的声道布局。
    wanted_frame.channels       = spec.channels;        // 设置解码后音频的声道数为实际打开的音频设备的声道数。
    /// @todo 7. SDL 开始播放音频。 {0: 播放}
    /**
     * 函数原型: void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on);
     * 函数作用: 用于暂停或继续播放音频设备。
     * 参数列表:
     *  dev:      SDL_AudioDeviceID 类型，表示要操作的音频设备的 ID。
     *  pause_on: 整数类型，用于指定是否暂停音频设备的播放。值为非零表示暂停，值为 0 表示继续播放。
     */
    SDL_PauseAudioDevice(id,0);
    /// @todo 8. 循环读取音频帧，并将解码前的压缩数据放入音频同步队列。
    /**
     * 函数原型: int av_read_frame(AVFormatContext *s, AVPacket *pkt);
     * 函数作用: 从输入媒体文件中读取下一个数据包（packet）。
     * 参数列表:
     *  s：指向AVFormatContext结构体的指针，表示输入媒体文件的上下文（context）。AVFormatContext包含了输入文件的相关信息和状态。
     *  pkt：指向AVPacket结构体的指针，用于存储读取到的数据包。
     * 函数返回值:
     *  如果成功读取到数据包，则返回0。
     *  如果读取到的数据包是媒体文件的结束标志，或者发生了错误，则返回负值。
     */
    while(av_read_frame(pFormatCtx, &packet) >= 0) { //读一个 packet，数据放在 packet.data
        /**
         * @todo 架构选择
         * av_read_frame从Context上下文中读编码之后，解码之前的数据，其读取速度要大于解码的速度
         * 在使用FFmpeg来解码数据，SDL播放数据时，造成了读取速度大于处理速度，为了同步，两种方案:
         * 1. 在读取完一帧数据后判断是否同步，通过sleep函数阻塞在此处
         * 2. 使用任务队列来缓存数据，保证数据的有序性
         */
        if (packet.stream_index == audioStream) {   // 数据包的流索引是否与音频流索引相匹配
            packet_queue_put(&audio_queue, &packet);    // 将读取到的音频数据包放入音频队列中进行后续处理
        }
        else {
            av_free_packet(&packet);    // 释放不是音频数据包的数据包的相关资源。
        }
    }
    /// @todo 9. 等待音频队列为空。
    while( audio_queue.nb_packets != 0) {
        SDL_Delay(100);
    }
    /// @todo 10. 回收空间，关闭解码器和输入文件。
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    printf("play finished\n");
}

void AVPlayer::run()
{
    /// @todo 1. 初始化变量
    /// @todo 2. 初始化FFmpeg和SDL
    /// @todo 3. 打开音视频文件，并获取音视频流索引
    /// @todo 4. 查找对应的音视频解码器并打开解码器
    /// @todo 5. 初始化音频队列和视频队列
    /// @todo 6. 设置音频设备参数，并打开音频设备
    /// @todo 7. 设置参数, 供解码时候用
    /// @todo 8. SDL开始播放音频 {0: 播放}
    /// @todo 9. 循环读取音频帧，并将解码前的压缩数据放入音频同步队列。
    /// @todo 10. 分配解码视频帧需要的AVFrame, AVPacket
    /// @todo 11. 将解码后的YUV数据转换为RGB32 YUV420P 格式视频数据
    /// @todo 10. 等待音频队列为空。

    /// @todo 1. 初始化变量
    const char* filename = filename_.toStdString().c_str();
    AVFormatContext *av_fmt_ctx     = nullptr;  // 文件音视频流上下文
    /////////////////////////VIDEO//////////////////////////////
    int video_stream                = -1;       // 解码器需要的视频流索引
    AVCodecContext* video_dec_ctx   = nullptr;  // 视频解码器上下文
    AVCodec* video_dec_codec        = nullptr;  // 视频解码器
    AVFrame* frame                  = nullptr;  // 解码之后的非压缩数据(视频对应RGB/YUV像素数据，音频对应PCM采样数据)
    AVFrame* frame_rgb              = nullptr;
    int y_size                      = 0;
    int64_t pts                     = 0;        // 当前视频的pts[解码时间戳]
    static struct SwsContext* img_convert_ctx;
    int ret, got_picture;
    int64_t start_time;
    /////////////////////////AUDIO//////////////////////////////
    int audio_stream                = -1;       // 解码器需要的音频流索引
    AVCodecContext* audio_dec_ctx   = nullptr;  // 存储音频解码器的上下文信息, 包含了解码器的参数、状态以及相关的信息。
    AVCodec *audio_dec_codec        = nullptr;  // 音频解码器
    SDL_AudioSpec wanted_spec;                  // 想要打开的音频设置
    SDL_AudioSpec spec;                         // 实际打开的音频设置
    ////////////////////////////////////////////////////////////
    AVPacket* packet = (AVPacket*)malloc(sizeof(AVPacket)); // 解码前的压缩数据(视频对应H.264等码流数据，音频对应AAC/MP3等码流数据)

    /// @todo 2. 初始化FFmpeg和SDL
    av_register_all();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        cout << "Couldn't init SDL:" << SDL_GetError();
        exit(-1);
    }
    /// @todo 3. 打开音视频文件，并获取音视频流索引
    if (avformat_open_input(&av_fmt_ctx, filename, NULL, NULL) != 0) {
        cout << "Couldn't open input file:" << filename;
        exit(-1);
    }
    if (avformat_find_stream_info(av_fmt_ctx, NULL) < 0) {
        cout << "Not Found Stream Info from " << filename;
        exit(-1);
    }
    av_dump_format(av_fmt_ctx, 0, filename, false);             // 显示文件信息
    find_av_stream_index(av_fmt_ctx, video_stream, audio_stream); // 获取音视频流索引
    /// @todo 4. 查找对应的音视频解码器并打开解码器
    /////////////////////////VIDEO//////////////////////////////
    video_dec_ctx = av_fmt_ctx->streams[video_stream]->codec;
    video_dec_codec = avcodec_find_decoder(video_dec_ctx->codec_id);    // 查找解码器
    if (!video_dec_codec) {
        cout << "Unsupported video_dec_codec";
        avcodec_free_context(&video_dec_ctx);
        return;
    }
    if (avcodec_open2(video_dec_ctx, video_dec_codec, NULL) < 0) {
        cout << "Failed to open video_dec_codec: ";
        avcodec_free_context(&video_dec_ctx);
        return;
    }
    /////////////////////////AUDIO//////////////////////////////
    audio_dec_ctx = av_fmt_ctx->streams[audio_stream]->codec;
    audio_dec_codec = avcodec_find_decoder(audio_dec_ctx->codec_id);
    if (!audio_dec_codec) {
        cout << "Unsupported audio_dec_codec";
        avcodec_free_context(&audio_dec_ctx);
        exit(-1);
    }
    if (avcodec_open2(audio_dec_ctx, audio_dec_codec, NULL) < 0) {
        cout << "Failed to open audio_dec_codec: ";
        avcodec_free_context(&audio_dec_ctx);
        return;
    }
    /// @todo 5. 初始化音频队列和视频队列
    packet_queue_init(&audio_queue);
    /// @todo 6. 设置音频设备参数，并打开音频设备
    wanted_spec.freq     = audio_dec_ctx->sample_rate;  // 设置音频的采样率为解码器上下文 (pCodecCtx) 的采样率。
    wanted_spec.format   = AUDIO_S16SYS;                // 设置音频的样本格式为 AUDIO_S16SYS，表示每个样本为有符号的16位整数。
    wanted_spec.channels = audio_dec_ctx->channels;     // 设置音频的声道数为解码器上下文 (pCodecCtx) 的声道数。
    wanted_spec.silence  = 0;                           // 设置静音值为 0，表示没有静音。
    wanted_spec.samples  = SDL_AUDIO_BUFFER_SIZE;       // 设置音频缓冲区的大小为 SDL_AUDIO_BUFFER_SIZE，用于存储解码后的音频数据。读取第一帧后调整
    wanted_spec.callback = audio_callback;              // 设置音频回调函数为 audio_callback，用于从音频队列中获取数据。
    wanted_spec.userdata = audio_dec_ctx;               // 将解码器上下文 (pCodecCtx) 作为回调函数的用户数据传递，以便在回调函数中进行解码操作。
    /**
     * wanted_spec:想要打开的
     * spec:实际打开的,可以不用这个，函数中直接用 NULL,下面用到 spec 用 wanted_spec 代替。
     * 这里会开一个线程，调用 callback。
     * SDL_OpenAudioDevice->open_audio_device(开线程)->SDL_RunAudio->fill(指向 callback 函数)
     */
    SDL_AudioDeviceID id = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0,0), 0, &wanted_spec, &spec, 0);
    if( id < 0 ) { //第二次打开 audio 会返回-1
        cout << "Couldn't open Audio:" << SDL_GetError();
        exit(-1);
    }
    /// @todo 7. 设置参数, 供解码时候用
    wanted_frame.format         = AV_SAMPLE_FMT_S16;    // 设置解码后音频的样本格式为 AV_SAMPLE_FMT_S16，表示每个样本为有符号的16位整数。
    wanted_frame.sample_rate    = spec.freq;            // 设置解码后音频的采样率为实际打开的音频设备的采样率。
    wanted_frame.channel_layout = av_get_default_channel_layout(spec.channels); //设置解码后音频的声道布局为实际打开的音频设备的声道布局。
    wanted_frame.channels       = spec.channels;        // 设置解码后音频的声道数为实际打开的音频设备的声道数。
    /// @todo 8. SDL开始播放音频
    SDL_PauseAudioDevice(id,0);
    /// @todo 9. 循环读取音频帧，并将解码前的压缩数据放入音频同步队列。
//    while(av_read_frame(av_fmt_ctx, packet) >= 0) { //读一个 packet，数据放在 packet.data
//        /**
//         * @todo 架构选择
//         * av_read_frame从Context上下文中读编码之后，解码之前的数据，其读取速度要大于解码的速度
//         * 在使用FFmpeg来解码数据，SDL播放数据时，造成了读取速度大于处理速度，为了同步，两种方案:
//         * 1. 在读取完一帧数据后判断是否同步，通过sleep函数阻塞在此处
//         * 2. 使用任务队列来缓存数据，保证数据的有序性
//         */
//        if (packet->stream_index == audio_stream) {   // 数据包的流索引是否与音频流索引相匹配
//            packet_queue_put(&audio_queue, packet);    // 将读取到的音频数据包放入音频队列中进行后续处理
//        }
//        else {
//            av_free_packet(packet);    // 释放不是音频数据包的数据包的相关资源。
//        }
//    }
    /// @todo 10. 将解码后的YUV数据转换为RGB32 YUV420P 格式视频数据
    frame = av_frame_alloc();
    frame_rgb = av_frame_alloc();
    y_size = video_dec_ctx->width * video_dec_ctx->height;
    av_new_packet(packet, y_size);
    img_convert_ctx = sws_getContext(video_dec_ctx->width, video_dec_ctx->height, video_dec_ctx->pix_fmt, video_dec_ctx->width, video_dec_ctx->height, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    auto numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, video_dec_ctx->width, video_dec_ctx->height);
    uint8_t* out_buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill((AVPicture*)frame_rgb, out_buffer, AV_PIX_FMT_RGB32, video_dec_ctx->width, video_dec_ctx->height);
    start_time = av_gettime();
    while(true) {
        if (av_read_frame(av_fmt_ctx, packet) < 0) break; // 读取一帧视频存入到AVPacket类型中
        // 视频简易同步
        int64_t realTime = av_gettime() - start_time;   // 主时钟时间
        while(pts > realTime) {
            msleep(10);
            realTime = av_gettime() - start_time;
        }
        if (packet->stream_index == audio_stream) {   // 数据包的流索引是否与音频流索引相匹配
            packet_queue_put(&audio_queue, packet);    // 将读取到的音频数据包放入音频队列中进行后续处理
        }
        else if (packet->stream_index == video_stream) { // 生成图片
            ret = avcodec_decode_video2(video_dec_ctx, frame, &got_picture, packet);
            if (ret < 0) {
                cout << "decode error";
                return;
            }
            // 获取显示时间pts
            if (packet->dts == AV_NOPTS_VALUE && frame->opaque && *(uint64_t*)frame->opaque != AV_NOPTS_VALUE) {
                pts = *(uint64_t*)frame->opaque;
            } else if (packet->dts != AV_NOPTS_VALUE) {
                pts = packet->dts;
            } else {
                pts = 0;
            }
            pts *= 1000000 * av_q2d(av_fmt_ctx->streams[video_stream]->time_base);  // 绝对时间
            if (got_picture) {
                sws_scale(img_convert_ctx, (uint8_t const* const*)frame->data, frame->linesize, 0, video_dec_ctx->height, frame_rgb->data, frame_rgb->linesize);
                QImage tmpImage((uchar*)out_buffer, video_dec_ctx->width, video_dec_ctx->height, QImage::Format_RGB32);
                QImage image = tmpImage.copy();
                emit SIG_getOneImage(image);
            }
        }
        else {
            av_free_packet(packet);
        }
    }
    /// @todo 10. 等待音频队列为空。
    while(audio_queue.nb_packets != 0) {
        SDL_Delay(100);
    }
    /// @todo 释放资源
    av_free(out_buffer);
    av_free(frame_rgb);
    avformat_close_input(&av_fmt_ctx);  // 释放pFormatCtx相关资源和内存
    avcodec_close(video_dec_ctx);
    avcodec_close(audio_dec_ctx);
}

//len 的值常为 2048,表示一次发送多少。
//audio_buf_size：一直为样本缓冲区的大小，wanted_spec.samples.（一般每次解码这么多，文件不同，这个值不同)
//audio_buf_index： 标记发送到哪里了。
//这个函数的工作模式是:
//1. 解码数据放到 audio_buf, 大小放 audio_buf_size。(audio_buf_size = audio_size;这句设置）
//2. 调用一次 callback 只能发送 len 个字节,而每次取回的解码数据可能比 len 大，一次发不完。
//3. 发不完的时候，会 len == 0，不继续循环，退出函数，继续调用 callback，进行下一次发送。
//4. 由于上次没发完，这次不取数据，发上次的剩余的，audio_buf_size 标记发送到哪里了。
//5. 注意，callback 每次一定要发且仅发 len 个数据，否则不会退出。
//如果没发够，缓冲区又没有了，就再取。发够了，就退出，留给下一个发，以此循环。
//三个变量设置为 static 就是为了保存上次数据，也可以用全局变量，但是感觉这样更好。
//13.回调函数中将从队列中取数据, 解码后填充到播放缓冲区.

void AVPlayer::audio_callback(void *userdata, Uint8 *stream, int len) {
    AVCodecContext* pcodec_ctx = (AVCodecContext*)userdata;
    int len1;               // 每次从音频缓冲区拷贝到SDL缓冲区的数据长度
    int audio_data_size;    // 解码后的音频数据的大小
    static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];   // 存储解码后音频数据的缓冲区
    static unsigned int audio_buf_size = 0;     // 跟踪音频缓冲区的大小
    static unsigned int audio_buf_index = 0;    // 当前已使用的位置
    /*
     * len 是由 SDL 传入的 SDL 缓冲区的大小，如果这个缓冲未满，我们就一直往里填充数据
     * audio_buf_index 和 audio_buf_size 标示我们自己用来放置解码出来的数据的缓冲区，
     * 这些数据待 copy 到 SDL 缓冲区， 当 audio_buf_index >= audio_buf_size 的时候意味着我
     * 们的缓冲为空，没有数据可供 copy，这时候需要调用 audio_decode_frame 来解码出更多的桢数据
     */
    while (len > 0) // 当SDL缓冲区还有空间时，不断从音频缓冲区中获取数据并填充到SDL缓冲区中。
    {
        if (audio_buf_index >= audio_buf_size) { // 当音频缓冲区中的数据已经被使用完时，需要调用audio_decode_frame函数解码更多的音频数据。
            audio_data_size = audio_decode_frame(pcodec_ctx, audio_buf, sizeof(audio_buf));
            /* audio_data_size < 0 标示没能解码出数据，我们默认播放静音 */
            if (audio_data_size < 0) {
                /* silence */
                audio_buf_size = 1024;
                /* 清零，静音 */
                memset(audio_buf, 0, audio_buf_size);
            } else {
                audio_buf_size = audio_data_size;
            }
            audio_buf_index = 0;
        }
        /* 查看 stream 可用空间，决定一次 copy 多少数据，剩下的下次继续 copy */
        len1 = audio_buf_size - audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }
        //SDL_MixAudio 并不能用
        memcpy(stream, (uint8_t *) audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}

//对于音频来说，一个 packet 里面，可能含有多帧(frame)数据。

int AVPlayer::audio_decode_frame(AVCodecContext *pcodec_ctx, uint8_t *audio_buf, int buf_size) {
    static AVPacket pkt;                                // 静态变量pkt用于存储音频数据包
    static uint8_t* audio_pkt_data    = nullptr;        // 跟踪音频数据包的数据
    static int audio_pkt_size         = 0;              // 跟踪音频数据包的大小
    int sampleSize                    = 0;              // 跟踪音频样本的大小
    AVCodecContext *aCodecCtx         = pcodec_ctx;     // 音频解码器上下文
    AVFrame *audioFrame               = nullptr;        // 存储解码后的音频帧数据
    PacketQueue *audioq               = &audio_queue;
    static struct SwrContext *swr_ctx = nullptr;        // 存储音频重采样上下文。
    int convert_len;
    int len1, data_size;                                // 在解码过程中跟踪解码数据的长度。
    int n = 0;
    while(true) {   // 无限循环，直到解码成功或发生退出条件。
        if( quit ) return -1;
        if(packet_queue_get(audioq, &pkt, 0) <= 0) { // 从音频队列中获取数据包，如果获取失败则返回-1。
            return -1;
        }
        audioFrame = av_frame_alloc();
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
        while(audio_pkt_size > 0) {
            if( quit ) return -1;
            int got_picture;
            memset(audioFrame, 0, sizeof(AVFrame));
            int ret = avcodec_decode_audio4( aCodecCtx, audioFrame, &got_picture, &pkt);
            if( ret < 0 ) {
                cout << "Error in decoding audio frame.";
                //exit(0);
            }
            //一帧一个声道读取数据是 nb_samples , channels 为声道数 2 表示 16 位 2 个字节
            data_size = audioFrame->nb_samples * wanted_frame.channels * 2;
            if( got_picture ) {
                if (swr_ctx != NULL) {
                    swr_free(&swr_ctx);
                    swr_ctx = NULL;
                }
                swr_ctx = swr_alloc_set_opts(NULL, wanted_frame.channel_layout,
                                             (AVSampleFormat)wanted_frame.format,wanted_frame.sample_rate,
                                             audioFrame->channel_layout,(AVSampleFormat)audioFrame->format,
                                             audioFrame->sample_rate, 0, NULL);
                //初始化
                if (swr_ctx == NULL || swr_init(swr_ctx) < 0) {
                    printf("swr_init error\n");
                    break;
                }
                convert_len = swr_convert(swr_ctx, &audio_buf,
                                          AVCODEC_MAX_AUDIO_FRAME_SIZE,
                                          (const uint8_t **)audioFrame->data,
                                          audioFrame->nb_samples);
            }
            audio_pkt_size -= ret;
            if (audioFrame->nb_samples <= 0) {
                continue;
            }
            av_free_packet(&pkt);
            return data_size ;
        }
        av_free_packet(&pkt);
    }
}

void AVPlayer::find_av_stream_index(AVFormatContext* pformat_ctx, int& video_stream, int& audio_stream) {
    assert(video_stream != NULL || audio_stream != NULL); // 断言确保 video_stream 和 audio_stream 不同时为 NULL，否则会触发断言错误。
    int i = 0;
    int audio_index = -1;
    int video_index = -1;
    for (i = 0; i < pformat_ctx->nb_streams; i++) { // 遍历输入文件的所有流。
        if (pformat_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) { // 判断当前流的编解码器类型是否为视频类型。如果是，则将视频流索引 video_index 更新为当前流的索引。
            video_index = i;
        }
        if (pformat_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) { // 判断当前流的编解码器类型是否为音频类型。如果是，则将音频流索引 audio_index 更新为当前流的索引。
            audio_index = i;
        }
    }
    video_stream = video_index;
    audio_stream = audio_index;
}

