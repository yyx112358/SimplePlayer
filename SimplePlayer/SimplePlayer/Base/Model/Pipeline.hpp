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
        UNINITILIZED,               // 未初始化
        READY,                      // 准备完毕
        TEMPORARILY_UNAVALIABLE,
        END_OF_FILE,                // 文件结束
        ERROR,                      // 出错
    };
    
public:
    
    
public:
    std::shared_ptr<VideoFrame> videoFrame;
    std::shared_ptr<AudioFrame> audioFrame;
    EStatus status = EStatus::UNINITILIZED;
};

}
