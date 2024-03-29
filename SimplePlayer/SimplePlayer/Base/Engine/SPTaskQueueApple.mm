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
}

SPTaskQueueApple::~SPTaskQueueApple()
{
    
}

std::future<SPParam> SPTaskQueueApple::runSync(SPTask task)
{
    task.isAsync = false;
    {
        _mtx.lock();
        _tasks.push_back(std::move(task));
        _mtx.unlock();
    }
    
    if (_isRunning == false)
        _run();
    
    return task.msg.result.get_future();
}

std::future<SPParam> SPTaskQueueApple::runAsync(SPTask task)
{
    task.isAsync = true;
    {
        _mtx.lock();
        _tasks.push_back(std::move(task));
        _mtx.unlock();
    }
    
    if (_isRunning == false)
        _run();
    
    return task.msg.result.get_future();
}

void SPTaskQueueApple::_run()
{
    _isRunning = true;
    dispatch_sync(_queue, ^{
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
            dispatch_sync(_queue, blk);
        } else {
            dispatch_async(_queue, blk);
        }
    });
    

}

#endif // __APPLE__
