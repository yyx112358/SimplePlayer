//
//  SPTaskQueueApple.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/3/24.
//

#pragma once

#include <dispatch/dispatch.h>
#include <list>
#include <mutex>
#include <atomic>

#include "ISPTaskQueue.hpp"

namespace sp {
class SPTaskQueueApple : public ISPTaskQueue {
public:
    SPTaskQueueApple(std::string name);
    virtual ~SPTaskQueueApple();
    
public:
    std::future<SPParam> runSync(SPTask task) override;
    std::future<SPParam> runAsync(SPTask task) override;
    
protected:
    void _run();
    void _dispatchSync(dispatch_block_t blk);
    
protected:
    dispatch_queue_t _queue;
    std::list<SPTask> _tasks;
    std::mutex _mtx;
    std::atomic_bool _isRunning = false;
    
};
}
