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
#include "ISPMediaControl.hpp"
#include "ISPGraphContext.hpp"

namespace sp {

class AudioSpeaker_Mac;

class AudioOutputManager : public ISPGraphContextListener, public std::enable_shared_from_this<AudioOutputManager> {
public:
    enum class Status {
        UNINITAILIZED,
        RUN,
        STOP,
        PAUSE,
    };
    
public:
    AudioOutputManager(std::shared_ptr<ISPGraphContext> context);
    ~AudioOutputManager();
    
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
    
public:
    virtual void processMessage(SPMsg msg) {}
    
protected:
    void _loop();
    void _postAudioClock();
    
protected:
    
    std::weak_ptr<ISPGraphContext> _context;
    
    Status _status = Status::UNINITAILIZED;
    
    std::unique_ptr<sp::AudioSpeaker_Mac> _speaker;
    
    std::thread _processThread;
    
    std::weak_ptr<sp::SPPipelineQueue> _inputQueue;
};

}
