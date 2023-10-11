//
//  RingBuffer.h
//  SimplePlayer
//
//  Created by YangYixuan on 2023/9/29.
//

#pragma once

#include <condition_variable>
#include <mutex>
#include <semaphore.h>
#include <atomic>
#include <functional>

#include "SPLog.h"


namespace sp {

// 信号量
// C++20 后可使用std::counting_semaphore重写
class SPSemaphore {
public:
    SPSemaphore(int maxSize):_cnt(maxSize) {}
    ~SPSemaphore() {_cv.notify_all();}
    
    SPSemaphore(const SPSemaphore &) = delete;
    SPSemaphore(const SPSemaphore &&) = delete;
    SPSemaphore & operator=(SPSemaphore &) = delete;
    SPSemaphore & operator=(SPSemaphore &&) = delete;
    
    bool release() {
        
        std::unique_lock lk(_m);
        ++_cnt;
        _cv.notify_one();
        
        return true;
    }
    
    bool acquire() {
        
        std::unique_lock lk(_m);
        _cv.wait(lk, [this]{return _cnt > 0;});
        --_cnt;
        
        return true;
    }
    
private:
    std::mutex _m;
    std::condition_variable _cv;
    std::atomic_int _cnt;
};


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
