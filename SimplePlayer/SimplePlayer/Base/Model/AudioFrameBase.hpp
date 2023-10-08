//
//  AudioFrameBase.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/9/28.
//

#pragma once

#include <memory>

extern "C" {
#include "libavutil/samplefmt.h"
}

namespace sp {

struct AudioFrame {
    
    int64_t pts = -1;
    int64_t sampleRate = -1;
    int channels = 2;
    enum AVSampleFormat sampleFormat = AV_SAMPLE_FMT_NONE;
    
    int64_t dataSize = -1;
    std::shared_ptr<uint8_t[]> data = nullptr;
#if DEBUG
    float debugData[1024] = {0};
#endif
};


}
