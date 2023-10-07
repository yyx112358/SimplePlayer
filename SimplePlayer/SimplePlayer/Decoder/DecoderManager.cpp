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
    unInit();
}

#define RUN_INIT(expr) RUN(expr, return false)
bool DecoderManager::init(const std::string &path)
{
    SPLOGV("%s", avformat_configuration()) ;
    unInit();
    
    // 打开文件
    const char *cpath = path.c_str();
//    av_log_set_level(AV_LOG_DEBUG);
//    av_log_set_callback(my_log_callback);
    _fmtCtx = nullptr;
    RUN_INIT(avformat_open_input(&_fmtCtx, cpath, nullptr, nullptr));
    SPLOGI("Open %s Successed!", cpath);
    
    // 探测流信息
//    av_log_set_level(AV_LOG_DEBUG);
    RUN(avformat_find_stream_info(_fmtCtx, nullptr), return false);
    av_dump_format(_fmtCtx, 0, cpath, 0);
    
    // 视频解码器
    int videoStreamId = av_find_best_stream(_fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoStreamId < 0) {
        SPLOGE("Video stream NOT FOUND!");
    } else {
        AVStream *stream = _fmtCtx->streams[videoStreamId];
        const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (codec == nullptr) {
            SPLOGE("[ERROR] %s Failed", "avcodec_find_decoder(stream->codecpar->codec_id)");
            return false;
        }
        AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
        if (codecCtx == nullptr) {
            SPLOGE("[ERROR] %s Failed", "avcodec_alloc_context3(codec)");
            return false;
        }
        
        RUN_INIT(avcodec_parameters_to_context(codecCtx, stream->codecpar));
        RUN_INIT(avcodec_open2(codecCtx, codec, nullptr));
        
        _codecCtx[0] = codecCtx;// 下标0固定为视频轨
        _stream[0] = stream;
        
        // 颜色空间转换上下文
        AVPixelFormat srcPixelFormat = (AVPixelFormat)stream->codecpar->format;
        _swsCtx = sws_getContext(stream->codecpar->width, stream->codecpar->height, srcPixelFormat, 1920, 1080, AVPixelFormat::AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    }
    

    if (int audioStreamId = av_find_best_stream(_fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0); audioStreamId >= 0) {
        AVStream *stream = _fmtCtx->streams[audioStreamId];
        const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (codec == nullptr) {
            SPLOGE("[ERROR] %s Failed", "avcodec_find_decoder(stream->codecpar->codec_id)");
            return false;
        }
        AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
        if (codecCtx == nullptr) {
            SPLOGE("[ERROR] %s Failed", "avcodec_alloc_context3(codec)");
            return false;
        }
        
        RUN_INIT(avcodec_parameters_to_context(codecCtx, stream->codecpar));
        RUN_INIT(avcodec_open2(codecCtx, codec, nullptr));
        
        _codecCtx[1] = codecCtx;// 下标1固定为音频轨
        _stream[1] = stream;
    } else {
        SPLOGE("Audio stream NOT FOUND!");
    }
    
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
        avformat_close_input(&_fmtCtx); // 用avformat_free_context会内存泄漏
    _fmtCtx = nullptr;
    
    return true;
}

std::shared_ptr<Pipeline> DecoderManager::getNextFrame()
{
    std::shared_ptr<Pipeline> pipeline = std::make_shared<Pipeline>();
    
    // TODO: getNextFrame应能进行重试以确保输出一帧
    int ret = av_read_frame(_fmtCtx, _packet);
    if (pipeline->status = _checkAVError(ret, "av_read_frame(_fmtCtx, _packet)"); pipeline->status != Pipeline::EStatus::READY) {
        return pipeline;
    }
    
    AVStream *stream = _getStream(MediaType::VIDEO);
    AVCodecContext *codecCtx = _getCodecCtx(MediaType::VIDEO);
    if (_packet->stream_index == 0) {
        
        auto videoFrame = std::make_shared<VideoFrame>();
        videoFrame->width = stream->codecpar->width;
        videoFrame->height = stream->codecpar->height;
        videoFrame->pixelFormat = static_cast<AVPixelFormat>(stream->codecpar->format);
        
        pipeline->videoFrame = videoFrame;
    } else if (_packet->stream_index == 1) {

        stream = _getStream(MediaType::AUDIO);
        codecCtx = _getCodecCtx(MediaType::AUDIO);
        auto audioFrame = std::make_shared<AudioFrame>();
        
        audioFrame->sampleRate = stream->codecpar->sample_rate;
        audioFrame->sampleFormat = static_cast<AVSampleFormat>(stream->codecpar->format);
        
        pipeline->audioFrame = audioFrame;
    }
    
    bool suc = _decodePacket(pipeline, codecCtx, _packet, _frame);
    
    if (suc == false) {
        pipeline->videoFrame = nullptr;
        pipeline->audioFrame = nullptr;
    }
    
    return pipeline;
}

bool DecoderManager::_decodePacket(std::shared_ptr<Pipeline> &pipeline, AVCodecContext * const codecCtx, AVPacket * const packet, AVFrame * const frame) const
{
    if (codecCtx == nullptr || packet == nullptr)
        return false;
    
    int ret = avcodec_send_packet(codecCtx, packet);
    if (pipeline->status = _checkAVError(ret, "avcodec_send_packet(codecCtx, packet)"); pipeline->status != Pipeline::EStatus::READY) {
        return false;
    }
    
    do {
        
        ret = avcodec_receive_frame(codecCtx, frame);
        if (pipeline->status = _checkAVError(ret, "avcodec_receive_frame(codecCtx, frame)"); pipeline->status != Pipeline::EStatus::READY) {
            av_frame_unref(frame);
            break;
        }
        pipeline->status = Pipeline::EStatus::READY;
        
        SPLOGD("pts:%d", frame->pts);

        if (auto videoFrame = pipeline->videoFrame) {

            videoFrame->data = std::shared_ptr<uint8_t[]>(new uint8_t[videoFrame->width * videoFrame->height * 4]);
            uint8_t *dstData[4] = {videoFrame->data.get()};
            int dstLineSizes[4] = {frame->width * 4};
            
            sws_scale(_swsCtx, frame->data, frame->linesize, 0, videoFrame->height, dstData, dstLineSizes);
            videoFrame->pixelFormat = AVPixelFormat::AV_PIX_FMT_RGBA;
            videoFrame->pts = frame->pts;
            
            static bool isSave = true;
            if (isSave) {
                writeBMP2File("decode.bmp", videoFrame->data.get(), frame->width, frame->height, 4);
                isSave = false;
            }
        }
        
        if (auto audioFrame = pipeline->audioFrame) {

            audioFrame->sampleFormat = (enum AVSampleFormat)frame->format;
            audioFrame->dataSize = frame->nb_samples * av_get_bytes_per_sample(audioFrame->sampleFormat);
            audioFrame->data = std::shared_ptr<uint8_t[]>(new uint8_t[audioFrame->dataSize]);
            memcpy(audioFrame->data.get(), frame->extended_data[0], audioFrame->dataSize);
#if DEBUG
            memcpy(audioFrame->debugData, frame->extended_data[0], audioFrame->dataSize);
#endif
            
            FILE *f = NULL;
            static bool first = true;
            if (first) {
                f = fopen("audio.pcm", "wb+");
                first = false;
            } else {
                // 本地播放命令行：ffplay -f f32le -ar 44100 -ac 1 -i /Users/yangyixuan/Library/Containers/com.yyx.SimplePlayer/Data/audio.pcm
                f = fopen("audio.pcm", "ab+");
            }
            fwrite(frame->extended_data[0], 1, audioFrame->dataSize, f);
            fclose(f);
        }

        av_frame_unref(frame);
    } while(ret > 0);
    
    av_packet_unref(packet);
    
    return pipeline->status == Pipeline::EStatus::READY;
}


AVCodecContext * DecoderManager::_getCodecCtx(MediaType mediaType) const {
    switch (mediaType) {
        case MediaType::VIDEO:          return _codecCtx[0];
        case MediaType::AUDIO:          return _codecCtx[1];
            
        default:    return nullptr;
    }
}

AVStream * DecoderManager::_getStream(MediaType mediaType) const {
    switch (mediaType) {
        case MediaType::VIDEO:          return _stream[0];
        case MediaType::AUDIO:          return _stream[1];
            
        default:    return nullptr;
    }
}

Pipeline::EStatus DecoderManager::_checkAVError(int code, const char *msg /*= nullptr*/) {
    if (code == 0) { // 其它
        return Pipeline::EStatus::READY;
    } else if (code == AVERROR_EOF) { // 文件结束
        SPLOGI("%s | [%d] %s", msg ? msg : "", code, av_err2str(code));
        return Pipeline::EStatus::END_OF_FILE;
    }  else if (code == AVERROR(EAGAIN)) { // 还需要多解几帧
        SPLOGI("%s | [%d] %s", msg ? msg : "", code, av_err2str(code));
        return Pipeline::EStatus::TEMPORARILY_UNAVALIABLE;
    } else if (code < 0) { // 错误
        SPLOGE("%s | [%d] %s", msg ? msg : "", code, av_err2str(code));
        return Pipeline::EStatus::ERROR;
    } else { // 其它
        SPLOGV("%s | [%d] %s", msg ? msg : "", code, av_err2str(code));
        assert(0);
        return Pipeline::EStatus::READY;
    }
}
