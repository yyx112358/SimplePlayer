//
//  SPTaskQueueApple.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/3/24.
//

#if __APPLE__

#include "SPTaskQueueApple.hpp"

#import <Foundation/Foundation.h>

using namespace sp;

@interface TaskObj : NSObject
{
    SPTask task;
}
- (instancetype)init:(SPTask &&)task;
- (SPTask &)getTask;

@end

@implementation TaskObj

- (instancetype)init:(SPTask &&)task {
    if (self = [super init]) {
        self->task = std::move(task);
    }
    return self;
}

- (SPTask &)getTask {
    return task;
}

@end


SPTaskQueueApple::SPTaskQueueApple(std::string name) : ISPTaskQueue(name)
{
    _queue = dispatch_queue_create(_name.c_str(), DISPATCH_QUEUE_SERIAL);
    dispatch_queue_set_specific(_queue, this, this, NULL);
}

SPTaskQueueApple::~SPTaskQueueApple()
{
    
}

std::future<SPParam> SPTaskQueueApple::runSync(SPTask task)
{
    task.isAsync = false;
    std::future<SPParam> f = task.msg.result.get_future();
    {
        _mtx.lock();
        _tasks.push_back(std::move(task));
        _mtx.unlock();
    }
    
    if (_isRunning == false)
        _run();
    
    return f;
}

std::future<SPParam> SPTaskQueueApple::runAsync(SPTask task)
{
    task.isAsync = true;
    std::future<SPParam> f = task.msg.result.get_future();
    {
        _mtx.lock();
        _tasks.push_back(std::move(task));
        _mtx.unlock();
    }
    
    if (_isRunning == false)
        _run();
    
    return f;
}

void SPTaskQueueApple::_run()
{
    _isRunning = true;
    _dispatchSync(^{
        SPTask task;
        {
            std::lock_guard g(_mtx);
            if (_tasks.size() == 0) {
                _isRunning = false;
                return;
            }
            task = std::move(_tasks.front());
            _tasks.pop_front();
        }
        TaskObj *taskObj = [[TaskObj alloc] init:std::move(task)];
        dispatch_block_t blk = ^{
            taskObj.getTask.msg.result.set_value(taskObj.getTask.msg.callback());
            _run();
        };
        if (task.isAsync) {
            dispatch_async(_queue, blk);
        } else {
            _dispatchSync(blk);
        }
    });
    

}

void SPTaskQueueApple::_dispatchSync(dispatch_block_t blk) {
    // 在当前线程执行dispatch_sync将导致死锁，此时需要直接执行blk()
    // https://stackoverflow.com/questions/10984732/why-cant-we-use-a-dispatch-sync-on-the-current-queue
    void * key = dispatch_get_specific(this);
    if (key)
        blk();
    else
        dispatch_sync(_queue, blk);
}

#endif // __APPLE__
