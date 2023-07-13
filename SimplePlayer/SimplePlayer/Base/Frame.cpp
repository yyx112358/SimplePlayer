//
//  Frame.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/13.
//

#include "Frame.hpp"
#import <OpenGL/gl3.h>

uint32_t sp::Frame::glFormat() const {
    switch (this->pixelFormat) {
        case AV_PIX_FMT_RGBA: return GL_RGBA;
        default: assert(0); return GL_RGBA;
    }
}
