//
//  DecoderManager.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/12.
//

#pragma once

#include <string>
#include <optional>

#include "Pipeline.hpp"

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVPacket;
struct AVFrame;
struct SwsContext;

namespace sp {

class FrameQueue {
public:
    
};

class DecoderManager {
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
    
public:
    ~DecoderManager();
    
    bool init(const std::string &path);
    bool unInit();
    std::shared_ptr<Pipeline> getNextFrame();
    
protected:
    bool _decodePacket(std::shared_ptr<Pipeline> &pipeline, AVCodecContext * const codecCtx, AVPacket * const packet, AVFrame * const frame) const;
    
    
    AVCodecContext *_getCodecCtx(MediaType mediaType) const;
    AVStream *_getStream(MediaType mediaType) const;
    
    static Pipeline::EStatus _checkAVError(int code, const char *msg = nullptr);
    
protected:
    struct AVFormatContext *_fmtCtx = nullptr;
    struct AVCodecContext *_codecCtx[2] = {nullptr};
    struct AVStream *_stream[2] = {nullptr};
    struct AVPacket *_packet = nullptr;
    struct AVFrame *_frame = nullptr;
    struct SwsContext *_swsCtx = nullptr;
};


}
