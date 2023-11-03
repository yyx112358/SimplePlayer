//
//  Pipeline.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/9/28.
//

#pragma once

#include <memory>
#include <deque>

#include "VideoFrameBase.hpp"
#include "AudioFrameBase.hpp"
#include "SPSemaphore.hpp"

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



/// Pipeline的带锁生产者-消费者队列
class SPPipelineQueue {
    
public:
    SPPipelineQueue() = delete;
    SPPipelineQueue(size_t capacity):_capacity(capacity), _producerSem(static_cast<int>(_capacity)), _consumerSem(0) {}
    SPPipelineQueue(std::deque<std::shared_ptr<Pipeline>> &&inputs):_capacity(inputs.size()), _producerSem(static_cast<int>(_capacity)), _consumerSem(0), _buffer(inputs) {}
    ~SPPipelineQueue() {}
    
    /// 入队
    void enqueue(std::shared_ptr<Pipeline> value) {
        _producerSem.acquire();
        
        // 因为有锁，所以无需判定full()
        _buffer.push_back(value);
        
        _consumerSem.release();
    }
    
    /// 出队
    std::shared_ptr<Pipeline> deque() {
        _consumerSem.acquire();
        
        // 因为有锁，所以无需判定empty()
        std::shared_ptr<Pipeline> out = _buffer.front();
        _buffer.pop_front();
        
        _producerSem.release();
        return out;
    }
    
    inline std::shared_ptr<Pipeline>& front() {
        return _buffer.front();
    }
    
    inline std::shared_ptr<Pipeline>& back() {
        return _buffer.back();
    }
    
    inline size_t capacity() const {
        return _capacity;
    }
    
    inline size_t size() const {
        return _buffer.size();
    }
    
    inline bool full() const {
        return size() >= capacity();
    }
    
    inline bool empty() const {
        return size() == 0;
    }
    
private:
    std::deque<std::shared_ptr<Pipeline>> _buffer;
    
    const size_t _capacity;
    SPSemaphore _producerSem, _consumerSem;
};

}
