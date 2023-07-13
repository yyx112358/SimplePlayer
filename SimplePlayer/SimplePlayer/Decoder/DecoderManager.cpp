//
//  DecoderManager.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/12.
//

#include "DecoderManager.hpp"
#include "SPLog.h"
#include "ImageWriterBmp.h"
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include <libavutil/imgutils.h>
}

using namespace sp;

struct AVFormatContext;

#define RUN(expr, then) \
    if (int ret = expr; ret < 0) { \
        SPLOGE("[ERROR] %s == %d | %s", #expr, ret, av_err2str(ret));\
        then;\
    }

class AVFormatContextHolder {
public:
    ~AVFormatContextHolder()
    {
        avformat_free_context(_ctx.get());
    }
    std::unique_ptr<struct AVFormatContext>_ctx = nullptr;
};


DecoderManager::~DecoderManager()
{
    if (swsCtx != nullptr)
        sws_freeContext(swsCtx);
    if (frame != nullptr)
        av_frame_free(&frame);
    if (packet != nullptr)
        av_packet_free(&packet);
    for (auto &ctx : codecCtx) {
        if (ctx != nullptr)
            avcodec_free_context(&ctx);
    }
    if (fmtCtx != nullptr)
        avformat_free_context(fmtCtx);
}

#define RUN_INIT(expr) RUN(expr, return false)
bool DecoderManager::init(const std::string &path)
{
    SPLOGV("%s", avformat_configuration()) ;
    
    // 打开文件
    const char *cpath = path.c_str();
    av_log_set_level(AV_LOG_DEBUG);
//    av_log_set_callback(my_log_callback);
    fmtCtx = nullptr;
    if (int ret = avformat_open_input(&fmtCtx, cpath, nullptr, nullptr); ret >= 0)
        SPLOGI("Open %s Successed!", cpath);
    else
        SPLOGE("Open %s Failed! Reason: %s", cpath, av_err2str(ret));
    
    // 探测流信息
    av_log_set_level(AV_LOG_DEBUG);
    RUN(avformat_find_stream_info(fmtCtx, nullptr), return false);
    av_dump_format(fmtCtx, 0, cpath, 0);
    // 寻找解码器
    for (int i = 0; i < fmtCtx->nb_streams; i++) {
        AVStream *stream = fmtCtx->streams[i];
        const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
        codecCtx[i] = avcodec_alloc_context3(codec);
        RUN_INIT(avcodec_parameters_to_context(codecCtx[i], stream->codecpar));
        RUN_INIT(avcodec_open2(codecCtx[i], codec, nullptr));
        
        if (codecCtx[i]->codec_type == AVMEDIA_TYPE_VIDEO) {
            buffer.width = stream->codecpar->width;
            buffer.height = stream->codecpar->height;
            buffer.pixelFormat = AVPixelFormat::AV_PIX_FMT_RGBA;
            buffer.data = std::shared_ptr<uint8_t[]>(new uint8_t[buffer.width * buffer.height * 4]);
        }
    }
    if (codecCtx[0]->codec_type != AVMEDIA_TYPE_VIDEO)
        std::swap(codecCtx[0], codecCtx[1]);
    swsCtx = sws_getContext(buffer.width, buffer.height, buffer.pixelFormat, 1920, 1080, AVPixelFormat::AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    
    return true;
}
#undef RUN_INIT

std::optional<Frame> DecoderManager::getNextFrame(bool &eof)
{
    std::optional<Frame> result;
    eof = false;
    
    if (packet == nullptr) {
        packet = av_packet_alloc();
//        av_init_packet(packet);
        packet->data = nullptr;
        packet->size = 0;
    }
    RUN(av_read_frame(fmtCtx, packet), return result);
    RUN(avcodec_send_packet(codecCtx[0], packet), return result);
    
    if (frame == nullptr)
        frame = av_frame_alloc();
    int ret = 0;
    while(ret >= 0) {
        
        ret = avcodec_receive_frame(codecCtx[0], frame);
        if (ret == AVERROR(EAGAIN)) { // 还需要多解几帧
            SPLOGE("avcodec_receive_frame error[%d] %s", ret, av_err2str(ret));
            break;
        } else if (ret == AVERROR_EOF) { // 文件结束
            eof = true;
            break;
        } else if (ret < 0) { // 错误
            SPLOGE("avcodec_receive_frame error[%d] %s", ret, av_err2str(ret));
            break;
        }
        
        SPLOGD("pts:%d", frame->pts, frame->width, frame->height, frame->format);
        
        uint8_t *dstData[4] = {buffer.data.get()};
        int dstLineSizes[4] = {frame->width * 4};
        
        sws_scale(swsCtx, frame->data, frame->linesize, 0, buffer.height, dstData, dstLineSizes);
        
        static bool isSave = true;
        if (isSave) {
            writeBMP2File("decode.bmp", buffer.data.get(), frame->width, frame->height, 4);
//            isSave = false;
        }
        result = buffer;
        av_frame_unref(frame);
    }
    av_packet_unref(packet);
    
    return result;
}
