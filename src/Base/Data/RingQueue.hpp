//
//  RingBuffer.h
//  SimplePlayer
//
//  Created by YangYixuan on 2023/9/29.
//

#pragma once

#include <atomic>
#include <functional>
#include <array>

#include "SPLog.h"
#include "SPSemaphore.hpp"

namespace sp {

template <typename Tp, size_t Capacity>
/// 环形队列，内部元素复用。
class RingQueue {
    
public:
    RingQueue():_producerSem(Capacity), _consumerSem(0) {}
    RingQueue(std::array<Tp, Capacity> &&inputs):_producerSem(Capacity), _consumerSem(0), _buffer(inputs) {}
    ~RingQueue() {}
    
    /// 入队
    void enqueue(Tp &value) {
        _producerSem.acquire();
        
        // 因为有锁，所以无需判定full()
        _buffer[_head % Capacity] = value;
        _head++;
        
        _consumerSem.release();
    }
    
    /// 入队，并对队首元素执行block
    void enqueueWithBlock(std::function<void(Tp &)> block) {
        _producerSem.acquire();
        
        block(_buffer[_head % Capacity]);
        _head++;
        
        _consumerSem.release();
    }
    
    /// 出队
    Tp deque() {
        _consumerSem.acquire();
        
        Tp out = std::move(_buffer[_tail % Capacity]);
        _tail++;
        
        _producerSem.release();
        return out;
    }
    
    Tp& front() {
        SPASSERT(empty() == false);
        return _buffer[(_head - 1) % Capacity];
    }
    
    Tp& back() {
        SPASSERT(empty() == false);
        return _buffer[_tail % Capacity];
    }
    
    constexpr size_t capacity() {
        return Capacity;
    }
    
    size_t size() const {
        return _head - _tail;
    }
    
    bool full() const {
        return _head - _tail >= Capacity;
    }
    
    bool empty() const {
        return _head == _tail;
    }
    
private:
    std::array<Tp, Capacity> _buffer;
    std::atomic_size_t _head = 0; /// 头指针，指向队首元素的下一个节点
    std::atomic_size_t _tail = 0; /// 尾指针，指向队尾元素
    
    sp::SPSemaphore _producerSem, _consumerSem;
};

}
