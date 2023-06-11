//
//  ImageReader.h
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/11.
//

#ifndef ImageReader_h
#define ImageReader_h

#include <string>
#include <memory>

namespace sp {


struct ImageBuffer {
    size_t width, height;
    int type, pixelFormat; // TODO
    std::shared_ptr<uint8_t> data;
};


}

#endif /* ImageReader_h */
