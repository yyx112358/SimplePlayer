//
//  SPSemaphore.h
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/2.
//

#pragma once

#include <condition_variable>
#include <mutex>
#include <atomic>

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

}
