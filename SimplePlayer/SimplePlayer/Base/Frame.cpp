//
//  Frame.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/13.
//

#include "Frame.hpp"
#include <OpenGL/gl3.h>

using namespace sp;

bool VideoFrame::equalExceptData(const VideoFrame &other) const {
    return width == other.width && height == other.height
        && type == other.type && pixelFormat == other.pixelFormat;
}

uint32_t sp::VideoFrame::glFormat() const {
    switch (this->pixelFormat) {
        case AV_PIX_FMT_RGBA: return GL_RGBA;
        default: assert(0); return GL_RGBA;
    }
}
