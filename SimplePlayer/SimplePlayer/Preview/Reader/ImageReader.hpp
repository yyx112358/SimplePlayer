//
//  ImageReader.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/11.
//

#ifndef ImageReader_h
#define ImageReader_h

#include <string>
#include <memory>
#import <OpenGL/gl3.h>

#include "libavutil/pixfmt.h"

namespace sp {


struct ImageBuffer {
    GLsizei width = 0, height = 0;
    int type; // TODO
    enum AVPixelFormat pixelFormat = AV_PIX_FMT_RGBA;
    std::shared_ptr<uint8_t> data = nullptr;
};

inline GLuint AVPixelFormat2GLFormat(enum AVPixelFormat avFormat) {
    switch (avFormat) {
        case AV_PIX_FMT_RGBA: return GL_RGBA;
        default: assert(0); return GL_RGBA;
    }
}


}

#endif /* ImageReader_h */
