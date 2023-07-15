//
//  DecoderManager.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/12.
//

#pragma once

#include <string>
#include <optional>

#include "Frame.hpp"

struct AVFormatContext;
struct AVCodecContext;
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
    std::optional<Frame> getNextFrame();
    
protected:
    std::optional<Frame> _decodePacket(MediaType mediaType);
    AVCodecContext *_getCodecCtx(MediaType mediaType);
    
protected:
    struct AVFormatContext *_fmtCtx = nullptr;
    struct AVCodecContext *_codecCtx[2] = {nullptr};
    struct AVPacket *_packet = nullptr;
    struct AVFrame *_frame = nullptr;
    struct SwsContext *_swsCtx = nullptr;
    Frame _rgbaBuffer;
};


}
