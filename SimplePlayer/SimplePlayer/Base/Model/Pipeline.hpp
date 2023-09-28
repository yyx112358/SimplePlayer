//
//  Pipeline.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/9/28.
//

#pragma once

#include <memory>

#include "VideoFrameBase.hpp"
#include "AudioFrameBase.hpp"


namespace sp {

class Pipeline {
public:
    enum class EStatus {
        UNINITILIZED,
        READY,
        END_OF_FILE,
    };
    
public:
    
    
public:
    std::shared_ptr<VideoFrame> videoFrame;
    std::shared_ptr<AudioFrame> audioFrame;
    EStatus status = EStatus::UNINITILIZED;
};

}
