//
//  AudioRendererManager.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#include "AudioRendererManager.hpp"
#include "SPLog.h"

using namespace sp;
using std::chrono_literals::operator""ms;

AudioRendererManager::AudioRendererManager() {
    
}

AudioRendererManager::~AudioRendererManager() {
    uninit();
}

bool AudioRendererManager::init() {
    uninit();
    
    return true;
}

bool AudioRendererManager::uninit() {
    if (_processThread.joinable()) {
        stop(true);
    }
    _inputQueue.reset();
    
    if (_outputQueue != nullptr)
        _outputQueue->clear();
    _processThread = std::thread();
    _status = Status::UNINITAILIZED;
    
    return true;
}

bool AudioRendererManager::start(bool isSync) {
    SPASSERT(_inputQueue.lock() != nullptr);
    if (_inputQueue.lock() == nullptr)
        return false;
    
    // stop的_setStatus()到线程完全结束之间还有一小段时间，需要等待完全结束
    if (_processThread.joinable() == true && _status == Status::STOP)
        _processThread.join();
    if (_processThread.joinable() == false) {
        _processThread = std::thread([this]{ _loop(); });
    }
    _status = Status::RUN;
    
    return true;
}

bool AudioRendererManager::stop(bool isSync) {
    
    if (auto inputQueue = _inputQueue.lock()) {
        inputQueue->clear();
        inputQueue->enqueue(Pipeline::CreateStopPipeline());
    }
    _outputQueue->clear();
//    _outputQueue->enqueue(Pipeline::CreateStopPipeline());
    
    if (isSync) {
        if (_processThread.joinable())
            _processThread.join();
    }

    return true;
}

bool AudioRendererManager::pause(bool isSync) {
    SPASSERT_NOT_IMPL;
    return true;
}

bool AudioRendererManager::seek(bool isSync) {
    SPASSERT_NOT_IMPL;
    return true;
}

bool AudioRendererManager::flush(bool isSync) {
    SPASSERT_NOT_IMPL;
    return true;
}

bool AudioRendererManager::reset(bool isSync) {
    SPASSERT_NOT_IMPL;
    return true;
}

void AudioRendererManager::_loop() {
    SPLOGD("Audio render thread start");
    while(1) {
        auto inpueQueue = _inputQueue.lock();
        if (inpueQueue == nullptr) {
            _status = Status::STOP;
            _enqueueStopPipeline();
            break;
        }
        std::shared_ptr<Pipeline> pipeline = inpueQueue->deque();
        
        if (pipeline->status == Pipeline::EStatus::STOP) {
            _status = Status::STOP;
            _enqueueStopPipeline();
            break;
        }
//        std::this_thread::sleep_for(1000ms);
        
        // TODO: 处理和转换
        
        _outputQueue->enqueue(pipeline);
    }
    SPLOGD("Audio render thread end");
}

void AudioRendererManager::_enqueueStopPipeline() {
    _outputQueue->clear();
    _outputQueue->enqueue(Pipeline::CreateStopPipeline());
}
