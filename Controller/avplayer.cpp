#include "avplayer.h"

AVPlayer::AVPlayer(QObject *parent) : QThread(parent)
{
    /// @todo 1. 初始化FFmpeg和SDL
    av_register_all();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        cout << "Couldn't init SDL:" << SDL_GetError();
        exit(-1);
    }
    av_state_ = new AVState;
    av_state_->player = this;
    player_state_ = kStop;
    cout << avcodec_configuration();
}

void AVPlayer::set_filename(const QString &filename)
{
    if (player_state_ != kStop) return;
    filename_ = filename;
    player_state_ = kPlaying;
    this->start();
}

/**
 * 编码格式: H264 ——decode——> YUV ————> RGB
 * 数据类型: AVPacket       AVFrame   AVFrame
 * 变量名称: packet         pFrame    pFrameRGB
 */
void AVPlayer::run()
{
    // player_state_ = kPlaying;
    av_state_->is_read_frame_finished = false;
    av_state_->is_run_finished = false;
    av_state_->is_quit = false;
    av_state_->video_clock = 0;
    av_state_->audio_clock = 0;
    av_state_->is_video_thread_finished = false;
    /// @todo 2. 打开音视频文件，并获取音视频流索引
    if (avformat_open_input(&av_state_->av_fmt_ctx, filename_.toStdString().c_str(), NULL, NULL) != 0) {
        cout << "Couldn't open input file:" << filename_;
        exit(-1);
    }
    if (avformat_find_stream_info(av_state_->av_fmt_ctx, NULL) < 0) {
        cout << "Not Found Stream Info from " << filename_;
        exit(-1);
    }
    av_dump_format(av_state_->av_fmt_ctx, 0, filename_.toStdString().c_str(), false);  // 显示文件信息
    find_av_stream_index(av_state_->av_fmt_ctx, av_state_->video_stream_index, av_state_->audio_stream_index); // 获取音视频流索引
    /// @todo 3. 查找对应的音视频解码器并打开解码器
    /////////////////////////VIDEO//////////////////////////////
    if (av_state_->video_stream_index != -1) {
        av_state_->video_dec_ctx = av_state_->av_fmt_ctx->streams[av_state_->video_stream_index]->codec;
        av_state_->video_dec_codec = avcodec_find_decoder(av_state_->video_dec_ctx->codec_id);
        if (av_state_->video_dec_codec == nullptr) {
            cout << "Codec not found.";
            return;
        }
        if (avcodec_open2(av_state_->video_dec_ctx, av_state_->video_dec_codec, nullptr) < 0) {
            cout << "Could not open codec.";
            return;
        }
        av_state_->video_stream = av_state_->av_fmt_ctx->streams[av_state_->video_stream_index];
        av_state_->video_que = new PacketQueue;
        packet_queue_init(av_state_->video_que);
        av_state_->video_tid = SDL_CreateThread(VideoDecodeThread, "VideoDecodeThread", av_state_); // 视频解码线程
    }
    /////////////////////////AUDIO//////////////////////////////
    SDL_AudioSpec wanted_spec;                  // 想要打开的音频设置
    SDL_AudioSpec spec;                         // 实际打开的音频设置
    if (av_state_->audio_stream_index != -1) {
        av_state_->audio_dec_ctx = av_state_->av_fmt_ctx->streams[av_state_->audio_stream_index]->codec;
        av_state_->audio_dec_codec = avcodec_find_decoder(av_state_->audio_dec_ctx->codec_id);
        if (av_state_->audio_dec_codec == nullptr) {
            cout << "Codec not found.";
            return;
        }
        if (avcodec_open2(av_state_->audio_dec_ctx, av_state_->audio_dec_codec, nullptr) < 0) {
            cout << "Could not open codec.";
            return;
        }
        av_state_->audio_stream = av_state_->av_fmt_ctx->streams[av_state_->audio_stream_index];
        // 设置音频信息, 用来打开音频设备
        SDL_LockAudio();
        wanted_spec.freq = av_state_->audio_dec_ctx->sample_rate;  // 设置音频的采样率为解码器上下文 (pCodecCtx) 的采样率。
        switch (av_state_->audio_stream->codec->sample_fmt) {      // 设置音频的样本格式为 AUDIO_S16SYS，表示每个样本为有符号的16位整数。
        case AV_SAMPLE_FMT_U8:
            wanted_spec.format = AUDIO_S8;
            break;
        case AV_SAMPLE_FMT_S16:
            wanted_spec.format = AUDIO_S16SYS;
            break;
        default:
            wanted_spec.format = AUDIO_S16SYS;
            break;
        };
        wanted_spec.channels = av_state_->audio_dec_ctx->channels; // 设置音频的声道数为解码器上下文 (pCodecCtx) 的声道数。
        wanted_spec.silence  = 0;                               // 设置静音值为 0，表示没有静音。
        wanted_spec.samples  = SDL_AUDIO_BUFFER_SIZE;           // 设置音频缓冲区的大小为 SDL_AUDIO_BUFFER_SIZE，用于存储解码后的音频数据。读取第一帧后调整
        wanted_spec.callback = AudioCallBack;                  // 设置音频回调函数为 audio_callback，用于从音频队列中获取数据。
        wanted_spec.userdata = av_state_;                         // 将解码器上下文 (pCodecCtx) 作为回调函数的用户数据传递，以便在回调函数中进行解码操作。
        /**
         * wanted_spec:想要打开的
         * spec:实际打开的,可以不用这个，函数中直接用 NULL,下面用到 spec 用 wanted_spec 代替。
         * 这里会开一个线程，调用 callback。
         * SDL_OpenAudioDevice->open_audio_device(开线程)->SDL_RunAudio->fill(指向 callback 函数)
         */
        av_state_->audioID = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0,0), 0, &wanted_spec, &spec, 0);
        if(av_state_->audioID < 0 ) { //第二次打开 audio 会返回-1
            cout << "Couldn't open Audio:" << SDL_GetError();
            exit(-1);
        }
        // 设置参数, 供解码时候用
        switch (av_state_->audio_stream->codec->sample_fmt) { // 设置解码后音频的样本格式为 AV_SAMPLE_FMT_S16，表示每个样本为有符号的16位整数。
        case AV_SAMPLE_FMT_U8:
            av_state_->out_frame.format = AV_SAMPLE_FMT_U8;
            break;
        case AV_SAMPLE_FMT_S16:
            av_state_->out_frame.format = AV_SAMPLE_FMT_S16;
            break;
        default:
            av_state_->out_frame.format = AV_SAMPLE_FMT_S16;
            break;
        };
        av_state_->out_frame.sample_rate    = av_state_->audio_dec_ctx->sample_rate;     // 设置解码后音频的采样率为实际打开的音频设备的采样率。
        av_state_->out_frame.channel_layout = av_get_default_channel_layout(av_state_->audio_dec_ctx->channels); //设置解码后音频的声道布局为实际打开的音频设备的声道布局。
        av_state_->out_frame.channels       = av_state_->audio_dec_ctx->channels;        // 设置解码后音频的声道数为实际打开的音频设备的声道数。
        av_state_->audio_que = new PacketQueue;
        packet_queue_init(av_state_->audio_que);
        SDL_UnlockAudio();
        // SDL 播放声音 0 播放
        SDL_PauseAudioDevice(av_state_->audioID, 0);
    }
    /// @todo 9. 循环读取音频帧，并将解码前的压缩数据放入音频同步队列。
    AVPacket* packet = (AVPacket*)malloc(sizeof(AVPacket)); // 解码前的压缩数据(视频对应H.264等码流数据，音频对应AAC/MP3等码流数据)
    player_state_ = kPlaying;
    int delay_count = 0;
    Q_EMIT SIG_TotalTime(GetTotalTime());   // 给UI线程发送当前视频的总时长
    while(true) {
        if (av_state_->is_quit) {
            break;
        }
        if (av_state_->is_pause) {  // 播放暂停探测
            SDL_Delay(10);
            continue;
        }
        // 当队列里面的数据超过某个大小的时候 就暂停读取 防止一下子就把视频读完了，导致的空间分配不足
        if (av_state_->audio_stream_index != -1 && av_state_->audio_que->size > MAX_AUDIO_SIZE) {
            SDL_Delay(10);
            continue;
        }
        if (av_state_->video_stream_index != -1 && av_state_->video_que->size > MAX_VIDEO_SIZE) {
            SDL_Delay(10);
            continue;
        }
        if( av_state_->is_seek ) {
            // 跳转标志位 is_seek --> 1 清除队列里的缓存 3s --> 3min 3s 里面的数据 存在 队列和解码器
            // 3s 在解码器里面的数据和 3min 的会合在一起 引起花屏 --> 解决方案 清理解码器缓存AV_flush_...
            //什么时候清理 -->要告诉它 , 所以要来标志包 FLUSH_DATA "FLUSH"
            //关键帧--比如 10 秒 --> 15 秒 跳转关键帧 只能是 10 或 15 , 如果你要跳到 13 , 做法是跳到10 然后 10-13 的包全扔掉

            int stream_index = -1;
            int64_t seek_target = av_state_->seek_pos;//微秒
            if (av_state_->video_stream_index >= 0)
                stream_index = av_state_->video_stream_index;
            else if (av_state_->audio_stream_index >= 0)
                stream_index = av_state_->audio_stream_index;
            AVRational aVRational = {1, AV_TIME_BASE};
            if (stream_index >= 0) {
                seek_target = av_rescale_q(seek_target, aVRational,
                                           av_state_->av_fmt_ctx->streams[stream_index]->time_base); //跳转到的位置
            }
            if (av_seek_frame(av_state_->av_fmt_ctx, stream_index, seek_target, AVSEEK_FLAG_BACKWARD) < 0) {
                fprintf(stderr, "%s: error while seeking\n",av_state_->av_fmt_ctx->filename);
            } else {
                if (av_state_->audio_stream_index >= 0) {
                    AVPacket *packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个 packet
                    av_new_packet(packet, 10);
                    strcpy((char*)packet->data,FLUSH_DATA);
                    packet_queue_flush(av_state_->audio_que); //清除队列
                    packet_queue_put(av_state_->audio_que, packet); //往队列中存入用来清除的包
                }
                if (av_state_->video_stream_index >= 0) {
                    AVPacket *packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个 packet
                    av_new_packet(packet, 10);
                    strcpy((char*)packet->data,FLUSH_DATA);
                    packet_queue_flush(av_state_->video_que); //清除队列
                    packet_queue_put(av_state_->video_que, packet); //往队列中存入用来清除的包
                    av_state_->video_clock = 0; //考虑到向左快退 避免卡死
                    //视频解码过快会等音频 循环 SDL_Delay 在循环过程中 音频时钟会改变 , 快退 音频时钟变小
                }
            }
            av_state_->is_seek = false;
            av_state_->seek_time = av_state_->seek_pos ; //精确到微妙 seek_time 是用来做视频音频的时钟调整 --关键帧
            av_state_->seek_flag_audio = 1; //在视频音频循环中 , 判断, AVPacket 是 FLUSH_DATA清空解码器缓存
            av_state_->seek_flag_video = 1;
        }
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
        if (av_read_frame(av_state_->av_fmt_ctx, packet) < 0) {
            // 出现负值, 不一定是发生了错误, 也有可能是发生了错误，通过做一下延时来解决
            delay_count ++;
            if (delay_count >= 300) {
                av_state_->is_read_frame_finished = true;
                delay_count = 0;
            }
            if (av_state_->is_quit) {
                break;
            }
            SDL_Delay(10);
            continue;
        }
        delay_count = 0;
        if (packet->stream_index == av_state_->audio_stream_index) {
            packet_queue_put(av_state_->audio_que, packet);
        } else if (packet->stream_index == av_state_->video_stream_index) {
            packet_queue_put(av_state_->video_que, packet);
        } else {
            av_free_packet(packet);
        }
    }

    while(!av_state_->is_quit) { // 当退出循环时, 等待音频和视频解码结束
        SDL_Delay(100);
    }

    /// @todo 释放资源
    if(av_state_->video_stream_index != -1) {
        packet_queue_flush(av_state_->video_que); // 队列回收
    }
    if(av_state_->audio_stream_index != -1 && av_state_->audio_que->nb_packets != 0) {
        packet_queue_flush(av_state_->audio_que);
    }
    while(av_state_->video_stream_index != -1 && !av_state_->is_video_thread_finished) {
        SDL_Delay(10);  // 等待视频解码线程结束
    }
    //回收空间
    if(av_state_->audio_stream_index != -1 ) avcodec_close(av_state_->audio_dec_ctx);
    if(av_state_->video_stream_index != -1 ) avcodec_close(av_state_->video_dec_ctx);
    avformat_close_input(&av_state_->av_fmt_ctx);

    av_state_->is_run_finished = true;   // run线程退出标志
    player_state_ = kStop; // 视频播放结束

    if (av_state_->audioID != 0) { // 关闭 SDL 音频设备
        SDL_LockAudio();
        SDL_PauseAudioDevice(av_state_->audioID, 1);//停止播放,即停止音频回调函数
        SDL_CloseAudioDevice( av_state_->audioID);
        SDL_UnlockAudio();
        av_state_->audioID = 0;
    }
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

void AVPlayer::AudioCallBack(void *userdata, Uint8 *stream, int len) {
    AVState* is = (AVState*)userdata;
    int len1;               // 每次从音频缓冲区拷贝到SDL缓冲区的数据长度
    int audio_data_size;    // 解码后的音频数据的大小
    memset(stream, 0, len);
    if (is->is_pause) return;
    /*
     * len 是由 SDL 传入的 SDL 缓冲区的大小，如果这个缓冲未满，我们就一直往里填充数据
     * audio_buf_index 和 audio_buf_size 标示我们自己用来放置解码出来的数据的缓冲区，
     * 这些数据待 copy 到 SDL 缓冲区， 当 audio_buf_index >= audio_buf_size 的时候意味着我
     * 们的缓冲为空，没有数据可供 copy，这时候需要调用 audio_decode_frame 来解码出更多的桢数据
     */
    while (len > 0) // 当SDL缓冲区还有空间时，不断从音频缓冲区中获取数据并填充到SDL缓冲区中。
    {
        if (is->audio_buf_index >= is->audio_buf_size) { // 当音频缓冲区中的数据已经被使用完时，需要调用audio_decode_frame函数解码更多的音频数据。
            audio_data_size = audio_decode_frame(is, is->audio_buf, sizeof(is->audio_buf));
            /* audio_data_size < 0 标示没能解码出数据，我们默认播放静音 */
            if (audio_data_size < 0) {
                /* silence */
                is->audio_buf_size = 1024;
                /* 清零，静音 */
                memset(is->audio_buf, 0, is->audio_buf_size);
            } else {
                is->audio_buf_size = audio_data_size;
            }
            is->audio_buf_index = 0;
        }
        /* 查看 stream 可用空间，决定一次 copy 多少数据，剩下的下次继续 copy */
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }
        //SDL_MixAudio 并不能用
        // memcpy(stream, (uint8_t *) audio_buf + audio_buf_index, len1);
        memset(stream, 0, len1);
        SDL_MixAudioFormat(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, 100);
        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
}

int AVPlayer::VideoDecodeThread(void *arg) {
    AVState *is = (AVState*)arg;
    is->is_video_thread_finished = false;
    AVPacket pkt1, *packet = &pkt1;
    int ret, got_picture, numBytes;
    double video_pts = 0; //当前视频的 pts
    double audio_pts = 0; //音频 pts
    ///解码视频相关
    AVFrame *pFrame, *pFrameRGB;
    uint8_t *out_buffer_rgb; //解码后的 rgb 数据
    struct SwsContext *img_convert_ctx; //用于解码后的视频格式转换
    AVCodecContext *pCodecCtx = is->video_dec_ctx; //视频解码器
    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    ///这里我们改成了 将解码后的 YUV 数据转换成 RGB32
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                     pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                                     AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB32,
                                  pCodecCtx->width,pCodecCtx->height);
    out_buffer_rgb = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill((AVPicture *) pFrameRGB, out_buffer_rgb, AV_PIX_FMT_RGB32,
                   pCodecCtx->width, pCodecCtx->height);
    while(1) {
        if (is->is_quit) {
            break;
        }
        if (is->is_pause) {  // 播放暂停探测
            SDL_Delay(10);
            continue;
        }
        if (packet_queue_get(is->video_que, packet, 1) <= 0) {
            if (is->is_read_frame_finished && is->audio_que->nb_packets == 0) { // run线程结束或者播放到结束
                break;//队列里面没有数据了读取完毕了
            } else {
                SDL_Delay(1); //只是队列里面暂时没有数据而已
                continue;
            }
        }
        /// @todo 理解
        if (strcmp((char*)packet->data, FLUSH_DATA) == 0) {
            avcodec_flush_buffers(is->video_stream->codec);
            av_free_packet(packet);
            is->video_clock = 0;
            continue;
        }
        while(1) {
            if (is->is_quit) {
                break;
            }
            if (is->audio_que->size == 0) break;
            audio_pts = is->audio_clock;
            video_pts = is->video_clock;
            if (video_pts <= audio_pts) break;
            SDL_Delay(5);
        }
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,packet);
        if (packet->dts == AV_NOPTS_VALUE && pFrame->opaque&& *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE) {
            video_pts = *(uint64_t *) pFrame->opaque;
        } else if (packet->dts != AV_NOPTS_VALUE) {
            video_pts = packet->dts;
        } else {
            video_pts = 0;
        }
        video_pts *= 1000000 *av_q2d(is->video_stream->time_base);
        video_pts = SynchronizeVideo(is, pFrame, video_pts);//视频时钟补偿

        if (is->seek_flag_video) {  // 跳转到关键帧
            //发生了跳转 则跳过关键帧到目的时间的这几帧
            if (video_pts < is->seek_time) {
                av_free_packet(packet);
                continue;
            }else {
                is->seek_flag_video = 0;
            }
        }
        if (got_picture) {
            sws_scale(img_convert_ctx,
                      (uint8_t const * const *) pFrame->data,
                      pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
                      pFrameRGB->linesize);
            //把这个 RGB 数据 用 QImage 加载
            QImage tmpImg((uchar*)out_buffer_rgb,pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB32);
            QImage image = tmpImg.copy(); //把图像复制一份 传递给界面显示
            is->player->SendGetOneImage(image); //调用激发信号的函数
        }
        av_free_packet(packet);
    }

    if (!is->is_quit) {
        is->is_quit = true;
    }

    av_free(pFrame);
    av_free(pFrameRGB);
    av_free(out_buffer_rgb);
    is->is_video_thread_finished = true;
    // 清屏
    QImage img; //把图像复制一份 传递给界面显示
    img.fill(Qt::black);
    is->player->SendGetOneImage(img); //调用激发信号的函数
    return 0;
}

//对于音频来说，一个 packet 里面，可能含有多帧(frame)数据。
int AVPlayer::audio_decode_frame(AVState *is, uint8_t *audio_buf, int buf_size) {
    AVPacket pkt;
    uint8_t *audio_pkt_data = NULL;
    int audio_pkt_size = 0;
    int len1, data_size;
    int sampleSize = 0;
    AVCodecContext *aCodecCtx = is->audio_dec_ctx;
    AVFrame *audioFrame = av_frame_alloc();
    PacketQueue *audioq = is->audio_que;
    AVFrame wanted_frame = is->out_frame;
    if( !aCodecCtx|| !audioFrame ||!audioq) return -1;
    /*static*/ struct SwrContext *swr_ctx = NULL;
    int convert_len;
    int n = 0;
    while(true) {
        if( is->is_quit ) {
            return -1;
        }
        if (is->is_pause) {  // 播放暂停探测
            SDL_Delay(10);
            continue;
        }
        if( !audioq ) return -1;
        if(packet_queue_get(audioq, &pkt, 0) <= 0) {
            if( is->is_read_frame_finished && is->audio_que->nb_packets == 0 ) {
                is->is_quit = true;
            }
            return -1;
        }
        /// @todo
        if(strcmp((char*)pkt.data, FLUSH_DATA) == 0) {
            avcodec_flush_buffers(is->audio_stream->codec);
            av_free_packet(&pkt);
            continue;
        }
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
        while(audio_pkt_size > 0)
        {
            if( is->is_quit ) {
                return -1;
            }
            int got_picture;
            memset(audioFrame, 0, sizeof(AVFrame));
            int ret =avcodec_decode_audio4( aCodecCtx, audioFrame, &got_picture, &pkt);
            if( ret < 0 ) {
                printf("Error in decoding audio frame.\n");
                return 0;
            }
            //一帧一个声道读取数据字节数是 nb_samples , channels 为声道数 2 表示 16 位2 个字节
            //data_size = audioFrame->nb_samples * wanted_frame.channels * 2;
            switch( is->out_frame.format )
            {
                case AV_SAMPLE_FMT_U8:
                    data_size = audioFrame->nb_samples * is->out_frame.channels * 1;
                    break;
                case AV_SAMPLE_FMT_S16:
                    data_size = audioFrame->nb_samples * is->out_frame.channels * 2;
                    break;
                default:
                    data_size = audioFrame->nb_samples * is->out_frame.channels * 2;
                    break;
            }
            //sampleSize 表示一帧(大小 nb_samples)audioFrame 音频数据对应的字节数.
            sampleSize = av_samples_get_buffer_size(NULL, is->audio_dec_ctx->channels,
                                                    audioFrame->nb_samples,
                                                    is->audio_dec_ctx->sample_fmt, 1);
            //n 表示每次采样的字节数
            n =
                    av_get_bytes_per_sample(is->audio_dec_ctx->sample_fmt)*is->audio_dec_ctx->channels;
            //时钟每次要加一帧数据的时间= 一帧数据的大小/一秒钟采样 sample_rate 多次对应的字节数.
            is->audio_clock += (double)sampleSize*1000000 / (double) (n* is->audio_dec_ctx->sample_rate);
            // x s / f = ( bytes / f ) / (bytes / s ) ; 时钟是微秒级别 * 1000000
            if( is->seek_flag_audio) {
                if( is ->audio_clock < is->seek_time) { //没有到目的时间
                    if( pkt.pts != AV_NOPTS_VALUE) {
                        is->audio_clock = av_q2d( is->audio_stream->time_base )*pkt.pts *1000000 ; //取音频时钟 可能精度不够
                    }
                    break;
                }
                else {
                    if( pkt.pts != AV_NOPTS_VALUE) {
                        is->audio_clock = av_q2d( is->audio_stream->time_base )*pkt.pts *1000000 ; //取音频时钟 可能精度不够
                    }
                    is->seek_flag_audio = 0 ;
                }
            }
            if( got_picture ) {
                swr_ctx = swr_alloc_set_opts(NULL, wanted_frame.channel_layout,
                                             (AVSampleFormat)wanted_frame.format,
                                             wanted_frame.sample_rate,
                                             audioFrame->channel_layout,
                                             (AVSampleFormat)audioFrame->format,
                                             audioFrame->sample_rate, 0, NULL);
                //初始化
                if (swr_ctx == NULL || swr_init(swr_ctx) < 0)
                {
                    printf("swr_init error\n");
                    break;
                }
                convert_len = swr_convert(swr_ctx, &audio_buf,
                                          AVCODEC_MAX_AUDIO_FRAME_SIZE,
                                          (const uint8_t **)audioFrame->data,
                                          audioFrame->nb_samples);
            }
            audio_pkt_size -= ret;
            if (audioFrame->nb_samples <= 0)
            {
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

void AVPlayer::SendGetOneImage(QImage &img) {
    emit SIG_getOneImage(img);
}

void AVPlayer::Pause() {
    av_state_->is_pause = true;
    if (player_state_ != kPlaying) return;
    player_state_ = kPause;
}

void AVPlayer::Play() { // 暂停到播放, 并不是停止到播放
    av_state_->is_pause = false;
    if (player_state_ != kPause) return;
    player_state_ = kPlaying;
}

void AVPlayer::Stop(bool is_wait)
{
    av_state_->is_quit = true;
    if (is_wait) {
        while(!av_state_->is_run_finished || !av_state_->is_video_thread_finished) {    // 等待run函数退出
            SDL_Delay(10);
        }
    }
    player_state_ = kStop;
    // Q_EMIT SIG_PlayerStateChanged(kStop);
}

void AVPlayer::Seek(int64_t pos)
{
    if(!av_state_->is_seek) {
        av_state_->seek_pos = pos;
        av_state_->is_seek  = true;
    }
}

double AVPlayer::SynchronizeVideo(AVState *is, AVFrame *src_frame, double pts)
{
    double frame_delay;
    if (pts != 0) {
        /* if we have pts, set video clock to it */
        is->video_clock = pts;
    } else {
        /* if we aren't given a pts, set it to the clock */
        pts = is->video_clock;
    }
    /* update the video clock */
    frame_delay = av_q2d(is->video_stream->codec->time_base);
    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    is->video_clock += frame_delay;
    return pts;
}

double AVPlayer::GetCurrentTime()
{
    return av_state_->audio_clock;
}

int64_t AVPlayer::GetTotalTime()
{
    if (av_state_->av_fmt_ctx) return av_state_->av_fmt_ctx->duration;
    else return -1;
}

