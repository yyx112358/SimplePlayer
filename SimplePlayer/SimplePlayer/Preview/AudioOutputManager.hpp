//
//  AudioOutputManager.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#pragma once

#include <memory>
#include <thread>

#include "Pipeline.hpp"


namespace sp {

class AudioOutputManager {
public:
    enum class Status {
        UNINITAILIZED,
        RUN,
        STOP,
        PAUSE,
    };
    
public:
    AudioOutputManager() {}
    ~AudioOutputManager() {}
    
    AudioOutputManager(const AudioOutputManager &) = delete;
    AudioOutputManager &operator = (const AudioOutputManager &) = delete;
    
// Control
public:
    bool init();
    bool uninit();
    
    bool start(bool isSync);
    bool stop(bool isSync);
    bool pause(bool isSync);
    bool seek(bool isSync);
    bool flush(bool isSync);
    bool reset(bool isSync);
    
// Property
public:
    bool setInputQueue(std::shared_ptr<sp::SPPipelineQueue> inputQueue) { _inputQueue = inputQueue; return true; }
    
protected:
    void _loop();
    
protected:
    
    Status _status = Status::UNINITAILIZED;
    
    std::thread _processThread;
    
    std::shared_ptr<sp::SPPipelineQueue> _inputQueue;
};

}
