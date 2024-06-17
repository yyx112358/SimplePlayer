//
//  SPUnitBase.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/3/5.
//

#include "SPUnitBase.hpp"
#include "SPLog.h"

using namespace sp;

SPUnitBase::SPUnitBase(std::shared_ptr<ISPGraphContext>context) : _context(context)
{
}

SPUnitBase::~SPUnitBase()
{
    uninit(true);
}


#pragma mark - ISPMediaControl
std::future<bool> __DefaultFuture()
{
    std::promise<bool> result;
    result.set_value(true);
    return result.get_future();
}

std::future<bool> SPUnitBase::init(bool isSync) 
{
    _isInited = true;
    SPLOGV("Unit [%s] init done", UNIT_NAME());
    return __DefaultFuture();
}

std::future<bool> SPUnitBase::uninit(bool isSync) 
{
    _isInited = false;
//    SPLOGV("Unit [%s] uninit done", UNIT_NAME());
    return __DefaultFuture();
}

std::future<bool> SPUnitBase::start(bool isSync) 
{
    return __DefaultFuture();
}

std::future<bool> SPUnitBase::stop(bool isSync) 
{
    return __DefaultFuture();
}

std::future<bool> SPUnitBase::seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag) 
{
    return __DefaultFuture();
}

std::future<bool> SPUnitBase::pause(bool isSync) 
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
