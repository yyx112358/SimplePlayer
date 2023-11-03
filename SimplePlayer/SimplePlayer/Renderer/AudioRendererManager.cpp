//
//  AudioRendererManager.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#include "AudioRendererManager.hpp"
#include "SPLog.h"

using namespace sp;

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
    while(_inputQueue && _inputQueue->size() > 0) {
        _inputQueue->deque();
    }
    while(_outputQueue && _outputQueue->size() > 0) {
        _outputQueue->deque();
    }
    _processThread = std::thread();
    _status = Status::UNINITAILIZED;
    
    return true;
}

bool AudioRendererManager::start(bool isSync) {
    SPASSERT(_inputQueue != nullptr);
    if (_inputQueue == nullptr)
        return false;
    
    if (_processThread.joinable() == false) {
        _processThread = std::thread([this]{ _loop(); });
    }
    _status = Status::RUN;
    
    return true;
}

bool AudioRendererManager::stop(bool isSync) {
    _status = Status::STOP;
    
    if (_processThread.joinable())
        _processThread.join();
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
    while(1) {
        // TODO: 采用cmdQueue
        if (_status == Status::STOP)
            break;
        
        std::shared_ptr<Pipeline> pipeline = _inputQueue->deque();
        
        // TODO: 处理和转换
        
        _outputQueue->enqueue(pipeline);
    }
}
