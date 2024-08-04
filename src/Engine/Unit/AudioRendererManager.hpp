//
//  AudioRendererManager.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#pragma once

#include <memory>
#include <thread>

#include "Pipeline.hpp"


namespace sp {

class AudioRendererManager {
public:
    enum class Status {
        UNINITAILIZED,
        RUN,
        STOP,
        PAUSE,
    };
    
public:
    AudioRendererManager();
    ~AudioRendererManager();
    
    AudioRendererManager(const AudioRendererManager &) = delete;
    AudioRendererManager &operator = (const AudioRendererManager &) = delete;
    
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
    const std::shared_ptr<sp::SPPipelineQueue> getOutputQueue() { return _outputQueue; }
    
protected:
    void _loop();
    void _enqueueStopPipeline();
    
protected:
    
    constexpr static size_t OUTPUT_QUEUE_MAX_SIZE = 1;
    
    Status _status = Status::UNINITAILIZED;
    
    std::thread _processThread;
    
    std::weak_ptr<sp::SPPipelineQueue> _inputQueue;
    const std::shared_ptr<sp::SPPipelineQueue> _outputQueue = std::make_shared<sp::SPPipelineQueue>(OUTPUT_QUEUE_MAX_SIZE);
};

}
