//
//  DecoderManager.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/12.
//

#pragma once

#include <string>
#include <optional>
#include <memory>
#include <deque>
#include <shared_mutex>
#include <thread>
#include <future>

#include "Pipeline.hpp"
#include "SPUnitBase.hpp"

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVPacket;
struct AVFrame;
struct SwsContext;

namespace sp {

struct DecodeCommand {
    enum class Type {
        start,
        stop,
        seek,
        pause,
    } type;
    
    int64_t seekPts;
    std::promise<bool> result;
};

class SPDecodeReaderFF {
public:
    /// 同AVMediaType
    enum class MediaType {
        UNKNOWN = -1,  ///< Usually treated as DATA
        VIDEO,
        AUDIO,
        DATA,          ///< Opaque data information usually continuous
        SUBTITLE,
        ATTACHMENT,    ///< Opaque data information usually sparse
        NB
    };
    
    enum class Status {
        UNINITAILIZED,
        RUN,
        STOP,
        PAUSE,
    };
    
public:
    ~SPDecodeReaderFF();
    
    bool init(const std::string &path);
    bool unInit();
    
    std::future<bool> start(bool isSync);
    std::future<bool> stop(bool isSync);
//    std::future<bool> seek(bool isSync);
    std::future<bool> pause(bool isSync);
//    std::future<bool> flush(bool isSync);
//    std::future<bool> reset(bool isSync);
    
protected:
    void _loop();
    
    bool _pushNextCommand(DecodeCommand cmd);
    std::optional<DecodeCommand> _popNextCommand();
    void _finishCommand(std::optional<DecodeCommand> &cmd);
    void _enqueueStopPipeline();
    
    Status _getStatus() const;
    void _setStatus(Status newStatus);
    
    bool _decodePacket(std::shared_ptr<Pipeline> &pipeline, AVCodecContext * const codecCtx, AVPacket * const packet, AVFrame * const avFrame) const;
    
    
    AVCodecContext *_getCodecCtx(MediaType mediaType) const;
    AVStream *_getStream(MediaType mediaType) const;
    
    static Pipeline::EStatus _checkAVError(int code, const char *msg = nullptr);
    
// stop时不改变，uninit()时会清空
protected:
    struct AVFormatContext *_fmtCtx = nullptr;
    struct AVCodecContext *_codecCtx[2] = {nullptr};
    struct AVStream *_stream[2] = {nullptr};
    struct AVPacket *_avPacket = nullptr;
    struct AVFrame *_avFrame = nullptr;
    struct SwsContext *_swsCtx = nullptr;
    
// stop时会被清空的数据
protected:
    std::thread _processThread;
    
    Status _status = Status::UNINITAILIZED;
    std::deque<DecodeCommand> _cmdQueue;
    mutable std::shared_mutex _cmdLock;
    
public: // FIXME: 后续使用connnect()完成
    const std::shared_ptr<sp::SPPipelineQueue> _videoQueue = std::make_shared<sp::SPPipelineQueue>(8);
    const std::shared_ptr<sp::SPPipelineQueue> _audioQueue = std::make_shared<sp::SPPipelineQueue>(3);
};


}
