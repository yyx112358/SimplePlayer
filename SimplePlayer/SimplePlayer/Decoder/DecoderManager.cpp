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
#define ALLOC(expr, then) \
    expr; \
    if (expr == nullptr) { \
        SPLOGE("[ERROR] %s Failed", #expr);\
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
    unInit();
}

#define RUN_INIT(expr) RUN(expr, return false)
bool DecoderManager::init(const std::string &path)
{
    SPLOGV("%s", avformat_configuration()) ;
    unInit();
    
    // 打开文件
    const char *cpath = path.c_str();
    av_log_set_level(AV_LOG_DEBUG);
//    av_log_set_callback(my_log_callback);
    _fmtCtx = nullptr;
    RUN_INIT(avformat_open_input(&_fmtCtx, cpath, nullptr, nullptr));
    SPLOGI("Open %s Successed!", cpath);
    
    // 探测流信息
    av_log_set_level(AV_LOG_DEBUG);
    RUN(avformat_find_stream_info(_fmtCtx, nullptr), return false);
    av_dump_format(_fmtCtx, 0, cpath, 0);
    
    // 视频解码器
    int videoStreamId = av_find_best_stream(_fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoStreamId < 0) {
        SPLOGE("Find video stream failed");
    } else {
        AVStream *stream = _fmtCtx->streams[videoStreamId];
        const AVCodec *codec = ALLOC(avcodec_find_decoder(stream->codecpar->codec_id), return false);
        AVCodecContext *codecCtx = ALLOC(avcodec_alloc_context3(codec), return false);
        RUN_INIT(avcodec_parameters_to_context(codecCtx, stream->codecpar));
        RUN_INIT(avcodec_open2(codecCtx, codec, nullptr));
        
        _codecCtx[0] = codecCtx;// 下标0固定为视频轨
        
        _rgbaBuffer.width = stream->codecpar->width;
        _rgbaBuffer.height = stream->codecpar->height;
        _rgbaBuffer.pixelFormat = AVPixelFormat::AV_PIX_FMT_RGBA;
        _rgbaBuffer.data = std::shared_ptr<uint8_t[]>(new uint8_t[_rgbaBuffer.width * _rgbaBuffer.height * 4]);
        
        // 颜色空间转换上下文
        AVPixelFormat srcPixelFormat = (AVPixelFormat)stream->codecpar->format;
        _swsCtx = sws_getContext(_rgbaBuffer.width, _rgbaBuffer.height, srcPixelFormat, 1920, 1080, AVPixelFormat::AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    }
    
    // TODO: 音频解码
    
    
    // 初始化Packet和Frame
    _packet = av_packet_alloc();
    _frame = av_frame_alloc();
    
    return true;
}
#undef RUN_INIT

bool DecoderManager::unInit() {
    if (_swsCtx != nullptr)
        sws_freeContext(_swsCtx);
    _swsCtx = nullptr;
    
    if (_frame != nullptr)
        av_frame_free(&_frame);
    _frame = nullptr;
    
    if (_packet != nullptr)
        av_packet_free(&_packet);
    _packet = nullptr;
    
    for (auto &ctx : _codecCtx) {
        if (ctx != nullptr)
            avcodec_free_context(&ctx);
        ctx = nullptr;
    }
    
    if (_fmtCtx != nullptr)
        avformat_free_context(_fmtCtx);
    _fmtCtx = nullptr;
    
    return true;
}

std::optional<Frame> DecoderManager::getNextFrame()
{
    std::optional<Frame> result;
    
    RUN(av_read_frame(_fmtCtx, _packet), av_packet_unref(_packet);return result;);
    if (_packet->stream_index != 0) { // 音频轨，退出
        av_packet_unref(_packet);
        return result;
    }
    result = _decodePacket(MediaType::VIDEO);
        
    av_packet_unref(_packet);
    
    return result;
}

std::optional<Frame> DecoderManager::_decodePacket(MediaType mediaType)
{
    std::optional<Frame> result;
    AVCodecContext *codecCtx = _getCodecCtx(mediaType);
    RUN(avcodec_send_packet(codecCtx, _packet), return result);
    
    int ret = 0;
    while(ret >= 0) {
        
        ret = avcodec_receive_frame(codecCtx, _frame);
        if (ret == AVERROR(EAGAIN)) { // 还需要多解几帧
            SPLOGV("Need Decode more", ret, av_err2str(ret));
            break;
        } else if (ret == AVERROR_EOF) { // 文件结束
            SPLOGI("EOF frame", ret, av_err2str(ret));
            break;
        } else if (ret < 0) { // 错误
            SPLOGE("avcodec_receive_frame error[%d] %s", ret, av_err2str(ret));
            break;
        }
        
        SPLOGD("pts:%d", _frame->pts);
        
        uint8_t *dstData[4] = {_rgbaBuffer.data.get()};
        int dstLineSizes[4] = {_frame->width * 4};
        
        sws_scale(_swsCtx, _frame->data, _frame->linesize, 0, _rgbaBuffer.height, dstData, dstLineSizes);
        _rgbaBuffer.pixelFormat = AVPixelFormat::AV_PIX_FMT_RGBA;
        static bool isSave = true;
        if (isSave) {
            writeBMP2File("decode.bmp", _rgbaBuffer.data.get(), _frame->width, _frame->height, 4);
            isSave = false;
        }
        result = _rgbaBuffer;
        av_frame_unref(_frame);
    }
    
    return result;
}


AVCodecContext *DecoderManager::_getCodecCtx(MediaType mediaType) {
    switch (mediaType) {
        case MediaType::VIDEO:          return _codecCtx[0];
        case MediaType::AUDIO:          return _codecCtx[1];
            
        default:    return nullptr;
    }
}
