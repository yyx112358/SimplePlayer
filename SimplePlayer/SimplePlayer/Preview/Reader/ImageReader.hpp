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

namespace sp {


struct ImageBuffer {
    GLsizei width = 0, height = 0;
    int type; // TODO
    GLuint pixelFormat = GL_RGBA;
    std::shared_ptr<uint8_t> data = nullptr;
};


}

#endif /* ImageReader_h */
