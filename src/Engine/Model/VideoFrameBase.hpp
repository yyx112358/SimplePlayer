//
//  VideoFrameBase.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/9/28.
//

#pragma once

#include <memory>

extern "C" {
#include "libavutil/pixfmt.h"
}

namespace sp {

struct VideoFrame {
    
    int width = 0, height = 0;
    int64_t pts = -1;
    int64_t dts = -1;
    enum AVPixelFormat pixelFormat = AV_PIX_FMT_RGBA;

    std::shared_ptr<uint8_t[]> data = nullptr;
    
    bool operator == (const VideoFrame & other) const = default;
    bool equalExceptData(const VideoFrame & other) const;
    
    uint32_t glFormat() const;
};

}
