//
//  AudioOutputManager.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#include "AudioOutputManager.hpp"
#include "SPLog.h"

using namespace sp;

bool AudioOutputManager::init() {
    
    return true;
}

bool AudioOutputManager::start(bool isSync) {
    SPASSERT(_inputQueue != nullptr);
    if (_inputQueue == nullptr)
        return false;
    
    if (_processThread.joinable() == false) {
        _processThread = std::thread([this]{ _loop(); });
    }
    _status = Status::RUN;
    
    return true;
}


void AudioOutputManager::_loop() {
    while(1) {
        // TODO: 采用cmdQueue
        if (_status == Status::STOP)
            break;
        
        std::shared_ptr<Pipeline> pipeline = _inputQueue->deque();
        
        // TODO: 处理和转换
        
    }
}
