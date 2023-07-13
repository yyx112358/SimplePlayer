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

class DecoderManager {
public:
    ~DecoderManager();
    
    bool init(const std::string &path);
    std::optional<Frame> getNextFrame(bool &eof);
    
protected:
    struct AVFormatContext *fmtCtx = nullptr;
    struct AVCodecContext *codecCtx[2] = {nullptr};
    struct AVPacket *packet = nullptr;
    struct AVFrame *frame = nullptr;
    struct SwsContext *swsCtx = nullptr;
    Frame buffer;
};


}
