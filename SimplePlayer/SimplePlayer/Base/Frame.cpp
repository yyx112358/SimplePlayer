//
//  Frame.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/13.
//

#include "Frame.hpp"
#import <OpenGL/gl3.h>

using namespace sp;

bool Frame::operator == (const Frame & other) const {
    return width == other.width && height == other.height
        && type == other.type && pixelFormat == other.pixelFormat
        && data == other.data;
}

bool Frame::equalExceptData(const Frame &other) const {
    return width == other.width && height == other.height
        && type == other.type && pixelFormat == other.pixelFormat;
}

uint32_t sp::Frame::glFormat() const {
    switch (this->pixelFormat) {
        case AV_PIX_FMT_RGBA: return GL_RGBA;
        default: assert(0); return GL_RGBA;
    }
}
