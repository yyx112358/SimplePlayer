//
//  SPUnitBase.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/3/5.
//

#include "SPUnitBase.hpp"
#include "SPLog.h"
#include "ISPTaskQueue.hpp"

using namespace sp;

SPUnitBase::SPUnitBase(std::shared_ptr<ISPGraphContext>context) : _context(context)
{
}

SPUnitBase::~SPUnitBase()
{
    uninit(true);
}


#pragma mark - ISPMediaControl
std::future<SPParam> __DefaultFuture()
{
    std::promise<SPParam> result;
    result.set_value(true);
    return result.get_future();
}

std::future<SPParam> SPUnitBase::_runTask(bool isSync, std::function<SPParam(SPUnitBase * const sthis)>callback) {
    
    SPASSERT0(isSync || _processThread);
    
    // 没有_processThread，则直接执行
    if (_processThread == nullptr) {
        std::promise<SPParam> result;
        result.set_value(callback(this));
        return result.get_future();
    }
    
    // 否则在_processThread执行
    SPTask task;
    task.isAsync = !isSync;
    task.work = [wthis = weak_from_this(), call = std::move(callback)] (SPTask &) mutable -> SPParam {
        auto sthis = std::static_pointer_cast<SPUnitBase>(wthis.lock());
        if (sthis == nullptr)
            return SPParam();
        
        return call(sthis.get());
    };
    return _processThread->run(std::move(task));
}

std::future<SPParam> SPUnitBase::init(bool isSync)
{
    _isInited = true;
    SPLOGV("Unit [%s] init done", UNIT_NAME());
    return __DefaultFuture();
}

std::future<SPParam> SPUnitBase::uninit(bool isSync)
{
    _isInited = false;
//    SPLOGV("Unit [%s] uninit done", UNIT_NAME());
    return __DefaultFuture();
}

std::future<SPParam> SPUnitBase::start(bool isSync)
{
    return __DefaultFuture();
}

std::future<SPParam> SPUnitBase::stop(bool isSync)
{
    return __DefaultFuture();
}

std::future<SPParam> SPUnitBase::seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag)
{
    return __DefaultFuture();
}

std::future<SPParam> SPUnitBase::pause(bool isSync)
{
    return __DefaultFuture();
}

#pragma mark - ISPGraphListener


#pragma mark - ISPUnit

bool SPUnitBase::connect(std::shared_ptr<ISPUnit> unit)
{
    if (unit == nullptr)
        return false;
    
    _outUnits.push_back(unit);
    static_pointer_cast<SPUnitBase>(unit)->_inUnits.push_back(shared_from_this());
    return true;
}
bool SPUnitBase::disconnect(std::shared_ptr<ISPUnit> unit)
{
    if (unit == nullptr) {
        _inUnits.clear();
        _outUnits.clear();
        return true;
    }
    
    for (auto it = _outUnits.begin(); it != _outUnits.end();) {
        auto outUnit = it->lock();
        if (outUnit == nullptr || outUnit == unit)  // 删除指定节点和空节点
            it = _outUnits.erase(it);
        else
            ++it;
    }
    
    for (auto it = _inUnits.begin(); it != _inUnits.end();) {
        auto inUnit = it->lock();
        if (inUnit == nullptr || inUnit == shared_from_this())
            it = _outUnits.erase(it);
        else
            ++it;
    }
    
    return true;
}

bool SPUnitBase::disconnectAll()
{
    return disconnect(nullptr);
}

bool SPUnitBase::_process(std::shared_ptr<sp::Pipeline>)
{
    const char * const *p = static_cast<const char* const*>(nullptr);
    return true;
}
