//
//  ISPTaskQueue.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/3/24.
//

#pragma once

#include <string>
#include <future>
#include <functional>

#include "SPMsg.hpp"

namespace sp {


struct SPTask {
public:
    SPTask() = default;
    SPTask(SPTask &&) = default;
    SPTask &operator=(SPTask &&task) = default;
    
    SPTask &operator=(const SPTask &) = delete;

    void setResult(SPParam &&result) {
        _promise.set_value(result);
    }
    
    std::future<SPParam> getFuture() {
        return _promise.get_future();
    }
    
public:
    std::function<SPParam(SPTask&)> work;
    bool isAsync = false;
    bool forceExecute = false;
    int timeout = -1;
    int priority = 0;
    
    std::vector<SPParam> params;
    
protected:
    std::promise<SPParam> _promise;
};


// 任务队列
class ISPTaskQueue {
public:
    ISPTaskQueue(std::string name) : _name(name) {}
    ISPTaskQueue(const ISPTaskQueue &) = delete;
    ISPTaskQueue &operator=(const ISPTaskQueue &) = delete;
    virtual ~ISPTaskQueue() {}
    
    virtual std::future<SPParam> run(SPTask task) = 0;
    virtual std::future<SPParam> runSync(SPTask task) = 0;
    virtual std::future<SPParam> runAsync(SPTask task) = 0;
    
    static std::shared_ptr<ISPTaskQueue> Create(std::string name = "");
    
protected:
    std::string _name;
};


}

