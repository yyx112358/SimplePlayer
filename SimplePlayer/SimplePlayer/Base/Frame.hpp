//
//  Frame.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/13.
//

#pragma once

#include <memory>
#include "libavutil/pixfmt.h"


namespace sp {

struct Frame {
    int width = 0, height = 0;
    int type; // TODO
    enum AVPixelFormat pixelFormat = AV_PIX_FMT_RGBA;
    std::shared_ptr<uint8_t[]> data = nullptr;
    
    uint32_t glFormat() const;
};



}
