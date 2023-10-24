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
    _avPacket = av_packet_alloc();
    _avFrame = av_frame_alloc();
    
    return true;
}
#undef RUN_INIT

bool DecoderManager::unInit()
{
    if (_processThread.joinable()) {
        stop(true);
    }
    _status = Status::UNINITAILIZED;

    if (_swsCtx != nullptr)
        sws_freeContext(_swsCtx);
    _swsCtx = nullptr;
    
    if (_avFrame != nullptr)
        av_frame_free(&_avFrame);
    _avFrame = nullptr;
    
    if (_avPacket != nullptr)
        av_packet_free(&_avPacket);
    _avPacket = nullptr;
    
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

std::shared_ptr<Pipeline> DecoderManager::getNextFrame(MediaType mediaType)
{
    std::shared_ptr<Pipeline> pipeline = nullptr;
    if (mediaType == MediaType::VIDEO && _videoQueue.empty() == false)
        pipeline = _videoQueue.deque();
    else if (mediaType == MediaType::AUDIO && _audioQueue.empty() == false)
        pipeline = _audioQueue.deque();

    return pipeline;
}

std::future<bool> DecoderManager::start(bool isSync)
{
    DecodeCommand cmd {.type = DecodeCommand::Type::start};
    cmd.type = DecodeCommand::Type::start;
    std::future<bool> f = cmd.result.get_future();
    
    if (_processThread.joinable() == false) {
        _processThread = std::thread([this]{_loop();});
    }
    _pushNextCommand(std::move(cmd));
    
    if (isSync)
        f.wait();
    return f;
}


std::future<bool> DecoderManager::stop(bool isSync) {
    DecodeCommand cmd {.type = DecodeCommand::Type::start};
    cmd.type = DecodeCommand::Type::stop;
    std::future<bool> f = cmd.result.get_future();
    
    if (_processThread.joinable() == false) { // 已经停止了
        cmd.result.set_value(true);
        return f;
    }
    _pushNextCommand(std::move(cmd));
    
    if (isSync) {
        // 释放所有的pipeline，避免解码线程因_videoQueue满而阻塞
        while (_videoQueue.empty() == false) {
            _videoQueue.deque();
        }
        while (_audioQueue.empty() == false) {
            _audioQueue.deque();
        }
        // 等待stop命令执行完毕
        f.wait();
        // 清空命令队列
        if (_cmdQueue.empty() == false) {
            std::unique_lock<std::shared_mutex> lock(_cmdLock);
            _cmdQueue.clear();
        }
        // 调用join以等待彻底结束
        if (_processThread.joinable())
            _processThread.join();
    }
    return f;
}

//std::future<bool> DecoderManager::startseek(bool isSync);
//std::future<bool> DecoderManager::startpause(bool isSync);
//std::future<bool> DecoderManager::startflush(bool isSync);

void DecoderManager::_loop()
{
    SPLOGD("Decode thread start");
    while(1) {
        // 获取下一个命令，调整状态
        if (auto nextCommand = _popNextCommand()) {
            if (nextCommand->type == DecodeCommand::Type::pause) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                _finishCommand(nextCommand);
                continue;
            } else if (nextCommand->type == DecodeCommand::Type::stop) {
                SPLOGD("Decode thread end");
                _finishCommand(nextCommand);
                return;
            } else if (nextCommand->type == DecodeCommand::Type::seek) {
                // TODO: 调整参数
                int ret = av_seek_frame(_fmtCtx, -1, nextCommand->seekPts, 0);
                _checkAVError(ret);
                // TODO: Flush缓冲区
                nextCommand->result.set_value(true);
                _finishCommand(nextCommand);
            } else {
                _finishCommand(nextCommand);
            }
        }
        
        std::shared_ptr<Pipeline> pipeline = std::make_shared<Pipeline>();
        
        // TODO: getNextFrame应能进行重试以确保输出一帧
        int ret = av_read_frame(_fmtCtx, _avPacket);
        if (pipeline->status = _checkAVError(ret, "av_read_frame(_fmtCtx, _packet)"); pipeline->status != Pipeline::EStatus::READY) {
            continue;
        }
        
        AVStream *stream = _getStream(MediaType::VIDEO);
        AVCodecContext *codecCtx = _getCodecCtx(MediaType::VIDEO);
        if (_avPacket->stream_index == 0) {
            
            auto videoFrame = std::make_shared<VideoFrame>();
            videoFrame->width = stream->codecpar->width;
            videoFrame->height = stream->codecpar->height;
            videoFrame->pixelFormat = static_cast<AVPixelFormat>(stream->codecpar->format);
            
            pipeline->videoFrame = videoFrame;
        } else if (_avPacket->stream_index == 1) {

            stream = _getStream(MediaType::AUDIO);
            codecCtx = _getCodecCtx(MediaType::AUDIO);
            auto audioFrame = std::make_shared<AudioFrame>();
            
            audioFrame->sampleRate = stream->codecpar->sample_rate;
            audioFrame->sampleFormat = static_cast<AVSampleFormat>(stream->codecpar->format);
            
            pipeline->audioFrame = audioFrame;
        }
        
        bool suc = _decodePacket(pipeline, codecCtx, _avPacket, _avFrame);
        
        if (suc == false) {
            pipeline->videoFrame = nullptr;
            pipeline->audioFrame = nullptr;
        }
        if (_avPacket->stream_index == 0)
            _videoQueue.enqueue(pipeline);
        else if (_avPacket->stream_index == 1)
            _audioQueue.enqueue(pipeline);
    }
}


bool DecoderManager::_pushNextCommand(DecodeCommand cmd)
{
    // TODO: 优化，例如stop会清空_cmdQueue，seek会调整到最近的seek时间点
    std::unique_lock<std::shared_mutex> lock(_cmdLock);
    _cmdQueue.push_back(std::move(cmd));
    return true;
}

std::optional<DecodeCommand> DecoderManager::_popNextCommand()
{
    std::unique_lock<std::shared_mutex> lock(_cmdLock);

    if (_cmdQueue.empty())
        return std::nullopt;
    else {
        std::optional<DecodeCommand> cmd = std::move(_cmdQueue.front());
        _cmdQueue.pop_front();
        return cmd;
    }
}

void DecoderManager::_finishCommand(std::optional<DecodeCommand> &cmd)
{
    if (cmd.has_value() == false)
        return;
    
    switch (cmd->type) {
        case DecodeCommand::Type::stop:
            _setStatus(Status::STOP);
            break;
            
        case DecodeCommand::Type::pause:
            _setStatus(Status::PAUSE);
            break;
            
        case DecodeCommand::Type::start:
        case DecodeCommand::Type::seek:
        default:
            _setStatus(Status::RUN);
            break;
    }
    
    // TODO: 失败情况下返回false
    cmd->result.set_value(true);
}

sp::DecoderManager::Status DecoderManager::_getStatus() const {
    std::shared_lock<std::shared_mutex> lock(_cmdLock);
    return _status;
}
void DecoderManager::_setStatus(Status newStatus) {
    std::unique_lock<std::shared_mutex> lock(_cmdLock);
    _status = newStatus;
}

bool DecoderManager::_decodePacket(std::shared_ptr<Pipeline> &pipeline, AVCodecContext * const codecCtx, AVPacket * const packet, AVFrame * const avFrame) const
{
    if (codecCtx == nullptr || packet == nullptr)
        return false;
    
    int ret = avcodec_send_packet(codecCtx, packet);
    if (pipeline->status = _checkAVError(ret, "avcodec_send_packet(codecCtx, packet)"); pipeline->status != Pipeline::EStatus::READY) {
        return false;
    }
    
    do {
        
        ret = avcodec_receive_frame(codecCtx, avFrame);
        if (pipeline->status = _checkAVError(ret, "avcodec_receive_frame(codecCtx, frame)"); pipeline->status != Pipeline::EStatus::READY) {
            av_frame_unref(avFrame);
            break;
        }
        pipeline->status = Pipeline::EStatus::READY;
        
        SPLOGD("pts:%d", avFrame->pts);

        if (auto videoFrame = pipeline->videoFrame) {

            videoFrame->data = std::shared_ptr<uint8_t[]>(new uint8_t[videoFrame->width * videoFrame->height * 4]);
            uint8_t *dstData[4] = {videoFrame->data.get()};
            int dstLineSizes[4] = {avFrame->width * 4};
            
            sws_scale(_swsCtx, avFrame->data, avFrame->linesize, 0, videoFrame->height, dstData, dstLineSizes);
            videoFrame->pixelFormat = AVPixelFormat::AV_PIX_FMT_RGBA;
            videoFrame->pts = avFrame->pts;
            
            static bool isSave = true;
            if (isSave) {
                writeBMP2File("decode.bmp", videoFrame->data.get(), avFrame->width, avFrame->height, 4);
                isSave = false;
            }
        }
        
        if (auto audioFrame = pipeline->audioFrame) {

            audioFrame->channels = avFrame->ch_layout.nb_channels;
            audioFrame->pts = avFrame->pts;
            audioFrame->sampleFormat = (enum AVSampleFormat)avFrame->format;
            audioFrame->dataSize = avFrame->nb_samples * av_get_bytes_per_sample(audioFrame->sampleFormat) * avFrame->ch_layout.nb_channels;
            audioFrame->data = std::shared_ptr<std::byte[]>(new std::byte[audioFrame->dataSize]);
            
            SPASSERTEX(audioFrame->sampleFormat == AV_SAMPLE_FMT_FLTP, "Only support AV_SAMPLE_FMT_FLTP now. Need write new convert code.");
            if (av_sample_fmt_is_planar(audioFrame->sampleFormat)) { // 非交错
                // 转换为交错数据。LLL  RRR -> LRLRLR
                const int sampleCnt = avFrame->nb_samples;
                const int sampleSize = av_get_bytes_per_sample(audioFrame->sampleFormat);
                const int srcStride = 1 * sampleSize;
                const int dstStride = avFrame->ch_layout.nb_channels * sampleSize;

                for (int channel = 0; channel < avFrame->ch_layout.nb_channels; channel++) {
                    uint8_t *psrc = avFrame->data[channel];
                    std::byte *pdst = audioFrame->data.get() + sampleSize * channel;
                    for (int i = 0; i < sampleCnt; i++) {
                        memcpy(pdst, psrc, sampleSize); // memcpy效率理应高于手写for循环
                        psrc += srcStride;
                        pdst += dstStride;
                    }
                }
            } else { // 交错
                memcpy(audioFrame->data.get(), avFrame->extended_data[0], audioFrame->dataSize);
            }
            
#if DEBUG
            SPASSERTEX(audioFrame->dataSize <= sizeof(audioFrame->debugData), "Size error. Modify AudioFrame::debugData[] length");
            memcpy(audioFrame->debugData, audioFrame->data.get(), audioFrame->dataSize);

            FILE *f = NULL;
            static bool first = true;
            if (first) {
                f = fopen("audio.pcm", "wb+");
                first = false;
            } else {
                // 本地播放命令行：ffplay -f f32le -ar 44100 -ac 1 -i /Users/yangyixuan/Library/Containers/com.yyx.SimplePlayer/Data/audio.pcm
                f = fopen("audio.pcm", "ab+");
            }
            fwrite(audioFrame->data.get(), 1, audioFrame->dataSize, f);
            fclose(f);
#endif
        }

        av_frame_unref(avFrame);
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
        SPASSERT(0);
        return Pipeline::EStatus::READY;
    }
}
