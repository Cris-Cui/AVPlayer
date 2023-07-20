# 音视频播放器

## 背景知识

*   视频

    > &#x9;	所谓视频，其实就是将一张一张的图片连续的放出来，就像放幻灯片一样，由于人眼的惰性，因此只要图片的数量足够多，就会觉得是连续的动作。所以，只需要将一张一张的图片保存下来，这样就可以构成一个视频了。但是，由于目前网络和存储空间的限制，直接存储图片显然不可行。举个例子:在视频聊天中，假定从摄像头采集的图片每张大小200KB,每秒钟发送大概15帧图片，这样每秒钟需要的流量就需要3M(意味着需要24M的宽带)显然这个要求太高了。于是，就可以考虑将这些图片压缩来减少大小。并且视频基本上都是连续的两帧图像差别不是太大。因此，在记录下第一张完整的图像之后以后的每张图像都是只记录下和上一帧图像不一样的地方，直到出现了差别很大的图像，才重新记录一帧完整的图像 (这个完整的图像就叫做关键帧)这样就可以很大程度上减小空间了。上面提到的压缩专业术语叫做视频编码，目前主流的编码格式有国际电联的H.261、H.263、H.264、H.265，运动静止图像专家组的M-JPEG和国际标准化组织运动图像专家组的MPEG系列标准，此外在互联网上被广泛应用的还有Real-Networks的RealVideo、微软公司的WMV以及Apple公司的QuickTime等。编码格式这么多，我们应该选择哪种呢? 经过查阅资料目前比较好的选择是H.264

*   H.264的优势

    > 1.低码率 (Low Bit Rate): 和MPEG2和MPEG4 ASP等压缩技术相比，在同等图像质量下采用H.264技术压缩后的数据量只有MPEG2的1/8，MPEG4的1/3。
    >
    > 2.高质量的图像: H.264能提供连续、流畅的高质量图像 (DVD质量)
    >
    > 3.容错能力强\:H.264提供了解决在不稳定网络环境下容易发生的丢包等错误的必要工具
    >
    > 4.网络适应性强: H264提供了网络抽象层 (Network Abstraction Layer)，使得H.264的文件能容易地在不同网络上传输(例如互联网，CDMA，GPRS，WCDMA，CDMA2000等)。
    >
    > 5.高压缩率，H.264的压缩比达到惊人的102:1。
    >
    > H.264最大的优势是具有很高的数据压缩比率，在同等图像质量的条件下,H.264的压缩比是MPEG-2的2倍以上，是MPEG-4的1.5\~2倍。
    >
    > 举个例子，原始文件的大小如果为88GB，采用MPEG-2压缩标准压缩后变成3.5GB,压缩比为25:1，而采用H.264压缩标准压缩后变为879MB，从88GB到879MB，H.264的压缩比达到惊人的102:1。
    >
    > 低码率 (Low Bit Rate) 对H264的高的压缩比起到了重要的作用，和MPEG-2和MPEG-4 ASP等压缩技术相比，H.264压缩技术将大大节省用户的下载时间和数据流量收费。
    >
    > 尤其值得一提的是，H.264在具有高压缩比的同时还拥有高质量流畅的图像，正因为如此，经过H.264压缩的视频数据，在网络传输过程中所需要的带宽更少，也更加经济。

    *   为什么选择SDL播放声音?&#x20;

        > 播放声音有很多种方式:&#x20;
        >
        > 以windows系统为例，可以使用如下方法播放音频:
        >
        > &#x20;1直接调用系统API的wavein、waveout等函数&#x20;
        >
        > 2.使用directsound播放
        >
        > 3.使用qt 的多媒体库QMultiMedia的QAudioOutput播放&#x20;
        >
        > &#x9;	前两个方法都只能在windows上使用，且相当难用，这个对于新手来说要把它们用好并稳定运行比较难。QAudioOutput借助QMultiMedia库来使用,运行稳定较差&#x20;
        >
        > 基本上有下面几个主流的音频开源库:&#x20;
        >
        > &#x9;1: OpenAL:这个库比较好，强大，跨平台，不过这个库的资料相对比较少。LGPL;
        >
        > &#x9;2\:PortAudio:这个库也很不错，接口简单，方便获取 设备，播放音频。没有看到硬件混音接口，或许多开几个播放接口就可以实现。GPL，但是可以不开源自己的程序，其官方网站是这么写的，除非是我理解错了。可以登录其官方网站查看版权。&#x20;
        >
        > &#x9;3\:SDL:很有名的跨平台库&#x20;
        >
        > &#x9;		使用SDL的好处:&#x20;
        >
        > &#x9;				1.网上资料多，学习起来方便
        >
        > &#x9;				2.跨平台，Windows、Linux、Android、IOS通吃，基本上这4个系统就够我们用了。意味着我们可以使用相同的代码在这些系统上直接运行。
        >
        > &#x9;				3.库体积相对比较小

## 播放视频

### FFmpeg常用API

```cpp
  	/**
     * av_register_all(): 初始化FFMPEG(已弃用)
     * 初始化FFmpeg库，注册所有可用的编解码器和其他相关组件。
     * 自 FFmpeg 4.0 起，不再需要显式调用 av_register_all() 函数来注册容器、解码器和编码器。相反，FFmpeg 库会自动在首次使用相应组件时进行注册。
     */
     
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
     
    /**
     * pFormatCtx->streams[audioStream]: 通过 audioStream 变量获取音频流的指针，pFormatCtx 是一个 AVFormatContext 结构体指针，存储了文件的音视频流信息。
     * pCodecCtx = ...->codec：获取音频流的编解码器上下文，并将其赋值给 pCodecCtx，即 AVCodecContext 结构体指针。
     * pCodecCtx->codec_id：从解码器上下文中获取编解码器的 ID。
     * 根据编解码器的 ID 查找对应的解码器，并将其赋值给 pCodec
     */
    audio_dec_ctx = av_fmt_ctx->streams[audio_stream]->codec;
     
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
     
    /**
     * 函数原型: AVCodec* avcodec_find_decoder(enum AVCodecID id);
     * 函数作用: 在 FFmpeg 库中进行解码器的查找工作。
     * 参数说明:
     *  id: 解码器的id
     * 返回值:
     *  返回查找到的解码器
     */
     
     
     
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
```

## 播放音频

### SDL常用API

```cpp
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
     
    /**
     * 函数原型: void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on);
     * 函数作用: 用于暂停或继续播放音频设备。
     * 参数列表:
     *  dev:      SDL_AudioDeviceID 类型，表示要操作的音频设备的 ID。
     *  pause_on: 整数类型，用于指定是否暂停音频设备的播放。值为非零表示暂停，值为 0 表示继续播放。
     */
```

## 音视频同步

### 简易同步

按照视频的绝对时间来定时播放视频帧(如果当前时间快于视频时间就sleep) 认为音频不需要同步，音频是连续的

### 优化同步

以音频为绝对时间(与播放时间相同), 视频时钟如果快于音频时钟, 需要sleep等待

## 播放控制

### 功能列表

#### **TODO**

*   [ ] 打开文件
*   [x] 播放
*   [x] 暂停
*   [ ] 进度显示和调整
*   [ ] 播放时间显示

## 出现的bug

#### QT程序异常结束

##### 启动异常结束

**与动态链接库有关**

1.  直接访问外部库文件

```cpp
//-L+文件路径+\
//这个是直接通过路径访问外部库里面的dll和lib文件
LIBS += -LE:/opcv/opencv/build/x64/vc14/lib \
        -lopencv_world344
```

1.  引用外部库文件 **需要在构建文件release/debug中加入手动加入lib文件对应的dll**

```cpp
LIBS += E:/opcv/opencv/build/x64/vc14/lib/opencv_world344.lib
```



## 遇到的问题

##### 1. QT程序异常结束

分为两种情况:

- 启动异常结束
  **与动态链接库有关**
  1.  直接访问外部库文件

  ```cpp
  //-L+文件路径+\
  //这个是直接通过路径访问外部库里面的dll和lib文件
  LIBS += -LE:/opcv/opencv/build/x64/vc14/lib \
          -lopencv_world344
  ```

  2.  引用外部库文件
  **需要在构建文件release/debug中加入手动加入lib文件对应的dll**

  ```cpp
  LIBS += E:/opcv/opencv/build/x64/vc14/lib/opencv_world344.lib
  ```

- 运行中异常结束

##### 2. QLabel标签截断内容

*   [x] scaledContents勾选，**但不保证等比例缩放**




