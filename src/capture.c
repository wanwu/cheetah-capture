#include <emscripten.h>
#include <string.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t duration;
    uint8_t *data;
} ImageData;

typedef struct
{
    int lastKeyframe;
    int lastIframe;
} FrameInfo;

// double av_q2d(AVRational r) {
//     return r.num / (double) r.den;
// }

char* iso8859_1_to_utf8(const char* input) {
    if (input == NULL) return NULL;

    // 计算 UTF-8 字符串的长度
    size_t len = 0;
    for (const char* p = input; *p != '\0'; p++) {
        unsigned char ch = (unsigned char)*p;
        len += (ch < 128) ? 1 : 2;  // 128-255 范围的字符需要两个字节
    }

    // 分配内存（+1 为了 null terminator）
    char* output = (char*)malloc(len + 1);
    if (output == NULL) {
        perror("malloc failed");
        return NULL;
    }

    // 填充输出字符串
    char* out_ptr = output;
    for (const char* p = input; *p != '\0'; p++) {
        unsigned char ch = (unsigned char)*p;
        if (ch < 128) {
            *out_ptr++ = ch;
        } else {
            *out_ptr++ = 0xC0 | (ch >> 6);         // 前两位标记字节
            *out_ptr++ = 0x80 | (ch & 0x3F);       // 后六位数据字节
        }
    }
    *out_ptr = '\0';  // null-terminated

    return output;
}


AVFrame *initAVFrame(AVCodecContext *pCodecCtx, uint8_t **frameBuffer)
{
    AVFrame *pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL)
    {
        return NULL;
    }

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);

    *frameBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize,
    *frameBuffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);

    return pFrameRGB;
}


// 返回的字符串是静态或动态分配的，调用者需要检查是否需要 free
// 如果返回的指针等于传入的 def，则不需要 free；否则需要 free
static const char *dump_metadata(void *ctx, AVDictionary *m, const char *indent, const char *def)
{
    if (!m) {
        return def;  // 返回默认值，不需要 free
    }
    
    AVDictionaryEntry *result = av_dict_get(m, indent, NULL, 0);
    
    // 先检查 result 是否为 NULL
    if (!result) {
        printf("未找到 metadata key: %s, 使用默认值: %s\n", indent, def);
        return def;  // 返回默认值，不需要 free
    }
    
    // 再检查 result->value
    if (result->value) {
        printf("遍历到的内容 %s, 值是 %s\n", indent, result->value);
        return iso8859_1_to_utf8(result->value);  // 返回 malloc 的内存，需要 free
    }
    
    return def;  // 返回默认值，不需要 free
}

int getIframes(AVFormatContext *qFormatCtx, int videoStream, int timeStamp, int *framesList, int idx)
{
    // ===============寻找I帧
    AVPacket qPacket;
    int count = idx;
    while (av_read_frame(qFormatCtx, &qPacket) >= 0)
    { // 读出完整的帧
        if (qPacket.stream_index == videoStream)
        {
            if (count > 199)
            {
                break;
            }
            if (qPacket.flags == 1)
            {
                framesList[count] = qPacket.pts;
                count++;
            }
        }
    }
    return count;
    // if(key_frame_timestamp == stream->start_time){
    //     return -1;
    // }
    // ================================
}

AVFrame *readAVFrame(AVCodecContext *pCodecCtx, AVFormatContext *pFormatCtx, AVFrame *pFrameRGB,
int videoStream, int timeStamp, FrameInfo *frameInfo, int *Iframe, int icount)
{
    struct SwsContext *sws_ctx = NULL;

    AVPacket packet;
    AVFrame *pFrame = NULL;

    pFrame = av_frame_alloc();

    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
    pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    // av_read_frame分配数据
    int FirstPts = pFrame->pts;
    // 计算每一帧最近的关键帧 然后算差值
    int chazhi = 0;
    int nearInum = 0;
    int flag = 0;
    for (int i = 0; i < icount; i++)
    {
        if (Iframe[i] > timeStamp)
        {
            break;
        }
        if (flag == 0 || chazhi > (timeStamp - Iframe[i]))
        {
            flag = 1;
            nearInum = Iframe[i];
            chazhi = timeStamp - Iframe[i];
        }
    }
    if (nearInum < 0)
    {
        printf("get nearly frame error");
    }
    if (nearInum <= frameInfo->lastKeyframe)
    {
        //  说明已经前进超过了这个帧 重新计算差值
        chazhi = timeStamp - frameInfo->lastKeyframe;
    }
    else
    {
        chazhi = timeStamp - nearInum;
        // printf("命中进来了差值%d,nearInum%d\n", chazhi, nearInum);
        // seek到所在的帧上
        int ret = av_seek_frame(pFormatCtx, videoStream, nearInum, AVSEEK_FLAG_BACKWARD);
        // int ret = av_seek_frame(pFormatCtx, videoStream, timeStamp, AVSEEK_FLAG_BACKWARD);
        if (ret < 0)
        {
            fprintf(stderr, "av_seek_frame failed\n");
            return NULL;
        }
    }
    // printf("chazhi===>chazhi%d,nearInum %d,timeStamp %d,frameInfo->lastKeyframe %d\n", chazhi, nearInum, timeStamp, frameInfo->lastKeyframe);
    int buzhang = 0;
    int isSeekFrame = 0;
    // 遍历 寻找最接近当前ms的关键帧
    while (av_read_frame(pFormatCtx, &packet) >= 0)
    {
        // 读出完整的帧
        if (packet.stream_index == videoStream)
        {
            if (avcodec_send_packet(pCodecCtx, &packet) != 0)
            {
                fprintf(stderr, "avcodec_send_packet failed\n");
                av_packet_unref(&packet);
                continue;
            }
            if (avcodec_receive_frame(pCodecCtx, pFrame) == 0)
            { // 从解码器内部缓存中提取解码后的音视频帧 返回0是成功
                // printf("pFrame%d\n", FirstPts);
                int nowTime = pFrame->pts;
                if (nowTime < nearInum) {
                    continue;
                }
                // 如果是第0帧 插入frameList
                if (icount == 0)
                {
                    Iframe[0] = nowTime;
                }
                printf("nowtime==>%d,frameInfo->lastKeyframe%d,chazhi%d,base是%d\n", nowTime, frameInfo->lastKeyframe, chazhi, pFormatCtx->streams[videoStream]->time_base.den
                            / pFormatCtx->streams[videoStream]->time_base.num);
                if (
                    (nowTime - frameInfo->lastKeyframe) >= chazhi
                    || (
                        (nowTime - frameInfo->lastKeyframe)
                        >= (
                            pFormatCtx->streams[videoStream]->time_base.den
                            / pFormatCtx->streams[videoStream]->time_base.num
                        ) * 2
                    )
                )
                { // 继续解包寻找需要的帧
                    // buffer为目标
                    sws_scale(sws_ctx, (uint8_t const *const *)pFrame->data, pFrame->linesize, 0,
                    pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                    // sws_scale做分辨率像素视频格式转换
                    // sws_freeContext(sws_ctx);
                    // av_frame_free(&pFrame);
                    // av_packet_unref(&packet);
                    buzhang = nowTime;
                    isSeekFrame = 1;
                    // return pFrameRGB;
                    break;
                }
            }
        }
        // 这一次找到的时间
    }
    // printf("这一次找到的是%d,isSeekFrame%d\n", buzhang, isSeekFrame);
    frameInfo->lastKeyframe = buzhang;
    // 得到最接近的关键帧之后 给赋值到指针上面 作为上一次的Iframe
    frameInfo->lastIframe = nearInum;
    sws_freeContext(sws_ctx);
    av_frame_free(&pFrame);
    av_packet_unref(&packet);

    return pFrameRGB;
}

// 读取帧数据并返回 uint8 buffer
uint8_t *getFrameBuffer(AVFrame *pFrame, AVCodecContext *pCodecCtx)
{
    int width = pCodecCtx->width;
    int height = pCodecCtx->height;

    uint8_t *buffer = (uint8_t *)malloc(height * width * 3);
    for (int y = 0; y < height; y++)
    {
        memcpy(buffer + y * pFrame->linesize[0], pFrame->data[0] + y * pFrame->linesize[0], width * 3);
    }
    return buffer;
}
const char *get_js_code(ImageData ptr, int id)
{
    static char buf[1024];
    sprintf(buf, "transpostFrame(%d,%d)", ptr, id);
    return buf;
}
ImageData *getSpecificFrame(AVCodecContext *pNewCodecCtx, AVFormatContext *pFormatCtx,
int videoStream, int time, FrameInfo *frameInfo, int *Iframe, int counts, int id)
{
    uint8_t *frameBuffer = 0;
    AVFrame *pFrameRGB = initAVFrame(pNewCodecCtx, &frameBuffer);

    pFrameRGB = readAVFrame(pNewCodecCtx, pFormatCtx, pFrameRGB, videoStream, time, frameInfo, Iframe, counts);
    ImageData *imageData = NULL;
    imageData = (ImageData *)malloc(sizeof(ImageData));
    imageData->width = (uint32_t)pNewCodecCtx->width;
    imageData->height = (uint32_t)pNewCodecCtx->height;
    // 计算视频的实际时长
    if(pFormatCtx->duration!=AV_NOPTS_VALUE){
        int hours,mins,secs,us;
        int64_t duration=pFormatCtx->duration+5000;
        secs=duration/AV_TIME_BASE;
        us=duration%AV_TIME_BASE;//1000000
        mins=secs/60;
        secs%=60;
        hours=mins/60;
        mins%=60;
        int totalMilliseconds = (hours * 3600000) + (mins * 60000) + (secs * 1000) + (100 * us / AV_TIME_BASE);
        imageData->duration = (uint32_t)totalMilliseconds;
    } else {
        imageData->duration = 0;
    }
    imageData->data = getFrameBuffer(pFrameRGB, pNewCodecCtx);
    emscripten_run_script(get_js_code(*imageData, id));
    av_frame_free(&pFrameRGB);
    av_free(frameBuffer);
    return imageData;
}
AVCodecContext *initFileAndGetInfo(AVFormatContext *pFormatCtx, char *path, AVCodec *pCodec, int *videoStream)
{
    if (avformat_open_input(&pFormatCtx, path, NULL, NULL) < 0)
    {
        fprintf(stderr, "avformat_open_input failed\n");
        return NULL;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        fprintf(stderr, "avformat_find_stream_info failed\n");
        return NULL;
    }
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            *videoStream = i;
            break;
        }
    }
    // if (*videoStream == -1) {
    //     return -1;
    // }
    return pFormatCtx->streams[*videoStream]->codec;
}
AVCodec *initDecoder(AVCodecContext *pCodecCtx, AVFormatContext *pFormatCtx)
{
    printf("pCodecCtx->codec_id%d", pCodecCtx->codec_id);
    return avcodec_find_decoder(pCodecCtx->codec_id);
}
// count 是抽帧数目，关键帧数目不够补齐
// 返回值：1=成功，0=失败
int captureByCount(int count, char *path, int id)
{
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    if (!pFormatCtx)
    {
        fprintf(stderr, "Could not allocate AVFormatContext\n");
        return 0;
    }
    // printf("%d", res);
    AVCodec *pCodec = NULL;
    // AVCodecContext *≈=NULL;
    int Iframe[200];
    // 先获取全部的i帧 只需要第一次执行
    // 第一帧立即抽出 第一帧一定是I帧 时间0
    int videoStream = -1;
    AVCodecContext *pNewCodecCtx = NULL;
    ImageData *dataList = NULL;
    FrameInfo *frameInfo = NULL;
    
    // 初始化读取文件
    AVCodecContext *pCodecCtx = initFileAndGetInfo(pFormatCtx, path, pCodec, &videoStream);
    if (pCodecCtx == NULL)
    {
        fprintf(stderr, "initFileAndGetInfo failed\n");
        avformat_close_input(&pFormatCtx);
        return 0;
    }
    
    const char *def_desc = "";
    const char *description = dump_metadata(NULL, pFormatCtx->metadata, "description", def_desc);
    static char setDescription[1024];
    sprintf(setDescription, "setDescription(`%s`, %d)", description, id);
    emscripten_run_script(setDescription);
    // 如果返回的不是默认值，需要释放内存
    if (description != def_desc) {
        free((void*)description);
    }
    
    pCodec = initDecoder(pCodecCtx, pFormatCtx);
    if (pCodec == NULL)
    {
        fprintf(stderr, "avcodec_find_decoder failed\n");
        avformat_close_input(&pFormatCtx);
        return 0;
    }
    
    pNewCodecCtx = avcodec_alloc_context3(pCodec);
    // pNewCodecCtx = pNewCodecCtx2;
    if (!pNewCodecCtx)
    {
        fprintf(stderr, "Could not allocate AVCodecContext\n");
        avformat_close_input(&pFormatCtx);
        return 0;
    }
    
    if (avcodec_copy_context(pNewCodecCtx, pCodecCtx) != 0)
    {
        fprintf(stderr, "avcodec_copy_context failed\n");
        avcodec_free_context(&pNewCodecCtx);
        avformat_close_input(&pFormatCtx);
        return 0;
    }

    if (avcodec_open2(pNewCodecCtx, pCodec, NULL) < 0)
    {
        fprintf(stderr, "avcodec_open2 failed\n");
        avcodec_free_context(&pNewCodecCtx);
        avformat_close_input(&pFormatCtx);
        return 0;
    }

    int duration = pFormatCtx->duration;
    if (duration == -1)
    {
        fprintf(stderr, "get duration failed\n");
        avcodec_close(pNewCodecCtx);
        avcodec_free_context(&pNewCodecCtx);
        avformat_close_input(&pFormatCtx);
        return 0;
    }
    
    dataList = (ImageData *)malloc(sizeof(ImageData) * count);
    if (!dataList)
    {
        fprintf(stderr, "Could not allocate dataList\n");
        avcodec_close(pNewCodecCtx);
        avcodec_free_context(&pNewCodecCtx);
        avformat_close_input(&pFormatCtx);
        return 0;
    }
    
    frameInfo = (FrameInfo *)malloc(sizeof(FrameInfo));
    if (!frameInfo)
    {
        fprintf(stderr, "Could not allocate frameInfo\n");
        free(dataList);
        avcodec_close(pNewCodecCtx);
        avcodec_free_context(&pNewCodecCtx);
        avformat_close_input(&pFormatCtx);
        return 0;
    }
    // 初始化结构体
    frameInfo->lastKeyframe = 0;
    frameInfo->lastIframe = -1;
    AVStream *st = pFormatCtx->streams[videoStream];
    const char *def_rotate = "0";
    const char *rotate = dump_metadata(NULL, st->metadata, "rotate", def_rotate);
    static char setAngle[1024];
    sprintf(setAngle, "setAngle(%s, %d)", rotate, id);
    emscripten_run_script(setAngle);
    // 如果返回的不是默认值，需要释放内存
    if (rotate != def_rotate) {
        free((void*)rotate);
    }
    dataList[0] = *(getSpecificFrame(pNewCodecCtx, pFormatCtx, videoStream, 0, frameInfo, Iframe, 0, id));
    // emscripten_run_script(get_js_code(dataList[0])); // 把第一帧传出去
    // TODO:
    // seek I frame
    int iFrameCounts = getIframes(pFormatCtx, videoStream, 0, Iframe, 1);
        // 间隔2s 在ffmpeg里面的时间
    int base = pFormatCtx->streams[videoStream]->time_base.den / pFormatCtx->streams[videoStream]->time_base.num;
    printf("===>总共是%d, duration是%d,base是%d\n", iFrameCounts, duration, base);
    // 换算成他的时间单位
    int tmpDuration = duration / 1000 * base / 1000;
    for (int t = 0; t < iFrameCounts; t++)
    {
        printf("===>时间是%d对应的时间%dtempduration是%d\n", t, Iframe[t], tmpDuration);
    }
    // 判断最后一帧和结束时间的差值
    int lastIFrameBtwEnd = tmpDuration - Iframe[iFrameCounts - 1];
    int noramlInterval = 2 * base;
    // 如果最后一帧和结束时间的差值大于2s则可插入剩余图片
    int lastFrameAdd = lastIFrameBtwEnd > noramlInterval ? 0: 1;
    // 找到关键帧数目后 根据长度计算补齐位置
    int timeFrameList[count + 1];
    // 遍历 补齐剩下缺失的帧
    if (iFrameCounts >= count)
    {
        // 超出需要删掉 按照区间取n个
        int multiple = (iFrameCounts - 1) / (count - 1);
        for (int i = 0; i < count; i++)
        {
            timeFrameList[i] = Iframe[multiple*i];
        }
    }
    else
    {
        int remain = count - iFrameCounts;
        if (iFrameCounts == 1)
        {
            // 特殊处理
            // 第0帧是i帧 其他的塞进去
            timeFrameList[0] = 0;
            for (int i = 0; i < count; i++)
            {
                timeFrameList[i] = timeFrameList[i - 1]
                + (tmpDuration / count > noramlInterval ? noramlInterval : tmpDuration / count);
            }
        }
        else
        {
            int interval = remain / (iFrameCounts - lastFrameAdd);
            int left = remain % (iFrameCounts - lastFrameAdd);
            // 缺失需要补齐
            int index = 0;
            for (int i = 0; i < iFrameCounts; i++)
            {
                // 先塞i帧
                timeFrameList[index] = Iframe[i];
                index++;
                // 最后一张图后面没有 均分在前面
                if ((i != iFrameCounts - 1) || (lastFrameAdd == 0))
                {
                    int frameNumWillBePush = interval + (i < left ? 1 : 0);
                    int intervalBetweenTowIFrame = (i != iFrameCounts - 1) ? Iframe[i + 1] - Iframe[i]: lastIFrameBtwEnd;
                    int groupTimeInterval = (intervalBetweenTowIFrame * 0.1);
                    int realInterval = 0;
                    // 如果比10%还要密集 直接取区间均值
                    if (frameNumWillBePush > 10)
                    {
                        realInterval = (intervalBetweenTowIFrame / count > noramlInterval ? noramlInterval : intervalBetweenTowIFrame / count);
                    }
                    else
                    {
                        realInterval = (groupTimeInterval > noramlInterval) ? noramlInterval : groupTimeInterval;
                    }
                    for (int j = 0; j < frameNumWillBePush; j++)
                    {
                        // 均分前面需要算时间间隔 2s还是10%
                        timeFrameList[index] = Iframe[i] + (j + 1) * realInterval;
                        index++;
                    }
                }
            }
        }
        // for (int i = 0; i < count ; i++) {
        //     printf("最终时间是第%dzhang，第%ds\n", i,timeFrameList[i]);
        // }
    }
    // 剩下逻辑和指定时间抽的类似
    av_seek_frame(pFormatCtx, videoStream, 0, AVSEEK_FLAG_BACKWARD);

    for (int idx = 1; idx < count; idx++)
    {
        dataList[idx] = *(getSpecificFrame(pNewCodecCtx, pFormatCtx,
        videoStream, timeFrameList[idx], frameInfo, Iframe, iFrameCounts, id));
    }

    // 清理资源
    free(frameInfo);
    free(dataList);
    avcodec_close(pNewCodecCtx);
    avcodec_free_context(&pNewCodecCtx);
    // 注意：pCodec 是从 avcodec_find_decoder 返回的，不需要也不应该释放
    // 注意：pCodecCtx 是 pFormatCtx->streams[videoStream]->codec 的引用，会随 pFormatCtx 释放
    avformat_close_input(&pFormatCtx);
    // dataList 已通过 transpostFrame 传递，返回 1 表示成功
    return 1;
}
// 传入毫秒截取指定位置视频画面
ImageData **captureByMs(char *ms, char *path, int id)
{
    // 把接受到的ms进行分割 变成数组
    char copyStr[100];
    strcpy(copyStr, ms);
    char *remain = "";
    char *delim = ",";
    char *p = strtok_r(ms, delim, &remain);
    // 获取本次需要抽的帧的长度
    int len = 0;
    while (p)
    {
        len++;
        printf("%s\n", p);
        p = strtok_r(NULL, delim, &remain);
    }
    // 把每一帧的时间存起来 方便后面取
    int frameData[len + 1];
    char *ptrRemain = "";
    int current = 0;
    char *ptr = strtok_r(copyStr, delim, &ptrRemain);
    while (ptr)
    {
        frameData[current] = atol(ptr);
        // printf("aaaaaaaaaaaooo%d\n", frameData[current]);
        // 存储
        ptr = strtok_r(NULL, delim, &ptrRemain);
        current++;
    }
    // ===========frame data 记录完毕
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    int videoStream = -1;
    AVCodec *pCodec = NULL;
    // 初始化读取文件
    AVCodecContext *pCodecCtx = initFileAndGetInfo(pFormatCtx, path, pCodec, &videoStream);
    pCodec = initDecoder(pCodecCtx, pFormatCtx);
    if (pCodec == NULL)
    {
        fprintf(stderr, "avcodec_find_decoder failed\n");
        return NULL;
    }
    AVCodecContext *pNewCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_copy_context(pNewCodecCtx, pCodecCtx) != 0)
    {
        fprintf(stderr, "avcodec_copy_context failed\n");
        return NULL;
    }

    if (avcodec_open2(pNewCodecCtx, pCodec, NULL) < 0)
    {
        fprintf(stderr, "avcodec_open2 failed\n");
        return NULL;
    }

    if (!pNewCodecCtx)
    {
        fprintf(stderr, "pNewCodecCtx is NULL\n");
        return NULL;
    }
    // 申请空间 创建结构体指针的数组
    // 声明装imageData类型的指针
    ImageData *dataList = (ImageData *)malloc(sizeof(ImageData) * len);
    int idx = 0;
    FrameInfo *frameInfo;
    frameInfo = (FrameInfo *)malloc(sizeof(FrameInfo));
    // 初始化结构体
    frameInfo->lastKeyframe = 0;
    frameInfo->lastIframe = -1;
    int Iframe[200];
    // 先获取全部的i帧 只需要第一次执行
    // 第一帧立即抽出 第一帧一定是I帧 时间0
    if (frameData[idx] == 0)
    {
        dataList[0] = *(getSpecificFrame(pNewCodecCtx, pFormatCtx, videoStream, 0, frameInfo, Iframe, 0, id));
        idx = 1;
    }
    // TODO:
    // seek I frame
    int counts = getIframes(pFormatCtx, videoStream, 0, Iframe, idx);
    if (counts == -1)
    {
        // 说明关键帧的读取失败了
        printf("get keyframes error!");
    }
    printf("keyframe counts %d...长度%d\n", counts, len);
    // 先seek到第一帧的地方 这里先写0了 实际应该是传进来的第一帧时间
    av_seek_frame(pFormatCtx, videoStream, 0, AVSEEK_FLAG_BACKWARD);

    for (idx; idx < len; idx++)
    {
        // 生成结构体 模拟生成指针 存到dataList
        int timeStamp = ((double)(frameData[idx]) / (double)1000)
        * pFormatCtx->streams[videoStream]->time_base.den / pFormatCtx->streams[videoStream]->time_base.num;
        dataList[idx] = *(getSpecificFrame(pNewCodecCtx, pFormatCtx,
        videoStream, timeStamp, frameInfo, Iframe, counts, id));
        // dataList[idx] = *imageData;
        // free
    }

    avcodec_close(pNewCodecCtx);
    av_free(pCodec);
    avcodec_close(pCodecCtx);

    avformat_close_input(&pFormatCtx);
    // dataList 已通过 transpostFrame 传递，这里返回 NULL
    return NULL;
}

// 获取视频的元数据
char *getMetaDataByKey(const char *key, const char *path) {
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    if (!pFormatCtx) {
        fprintf(stderr, "Could not allocate AVFormatContext.\n");
        return NULL;
    }

    AVCodec *pCodec = NULL;
    int videoStream = -1;
    AVCodecContext *pCodecCtx = initFileAndGetInfo(pFormatCtx, path, pCodec, &videoStream);

    if (!pCodecCtx) {
        fprintf(stderr, "Failed to initialize file and get codec context.\n");
        avformat_free_context(pFormatCtx);
        return NULL;
    }

    const char *def_value = "";
    const char *value = dump_metadata(NULL, pFormatCtx->metadata, key, def_value);
    printf("===>得到的value: %s\n", value);
    
    // 需要在释放 pFormatCtx 之前复制字符串
    char *result = NULL;
    if (value == def_value) {
        // 如果是默认值，复制一份
        result = strdup(def_value);
    } else {
        // 如果是 malloc 的内存，直接使用（但需要在释放 pFormatCtx 之前处理）
        result = strdup(value);
        free((void*)value);  // 释放 dump_metadata 返回的内存
    }
    
    avformat_free_context(pFormatCtx);

    if (!result) {
        fprintf(stderr, "Failed to allocate memory for metadata value.\n");
        return NULL;
    }

    return result;
}

// 检测视频是否存在音轨
int hasAudioTrack(const char *path) {
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    if (!pFormatCtx) {
        fprintf(stderr, "Could not allocate AVFormatContext.\n");
        return -1;
    }

    // 打开视频文件
    if (avformat_open_input(&pFormatCtx, path, NULL, NULL) < 0) {
        fprintf(stderr, "avformat_open_input failed\n");
        return -1;
    }

    // 获取流信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        fprintf(stderr, "avformat_find_stream_info failed\n");
        avformat_close_input(&pFormatCtx);
        return -1;
    }

    // 遍历所有流，查找音频流
    int hasAudio = 0;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            hasAudio = 1;
            printf("Found audio stream at index %d\n", i);
            break;
        }
    }

    avformat_close_input(&pFormatCtx);
    
    return hasAudio;
}
size_t get_string_length(int ptr) {
    if (ptr == 0) {
        printf("Received null pointer\n");
        return 0;  // 检查空指针
    }
    char* str = (char*) ptr;  // 将整数地址转换为字符指针
    printf("Received non-null pointer, pointing to: %s\n", str);  // 打印指针指向的字符串

    size_t length = strlen(str);  // 使用 strlen 获取字符串长度
    printf("String length: %zu\n", length);
    return length;
}
int main(int argc, char const *argv[])
{
    av_register_all();
    return 0;
}