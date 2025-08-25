//
//  SPUnitBase.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/3/5.
//

#include "SPUnitBase.hpp"
#include "SPLog.h"

using namespace std;
using namespace sp;

SPUnitBase::SPUnitBase(std::shared_ptr<ISPGraphContext>context) : _context(context)
{
}

SPUnitBase::~SPUnitBase()
{
    uninit(true);
}


#pragma mark - ISPMediaControl

SPResultChain SPUnitBase::init(bool isSync)
{
    auto result = SP_RESULT_CHAIN();
    _isInited = true;
    SPLOGV("Unit [%s] init done", UNIT_NAME());
    
    // TODO: 链式初始化渲染链上所有Unit
    result.wait();
    result.finish();
    return result;
}

SPResultChain SPUnitBase::uninit(bool isSync)
{
    auto result = SP_RESULT_CHAIN();
    _isInited = false;
//    SPLOGV("Unit [%s] uninit done", UNIT_NAME());

    result.wait();
    result.finish();
    return result;
}

SPResultChain SPUnitBase::start(bool isSync)
{
    auto result = SP_RESULT_CHAIN();
    result.wait();
    result.finish();
    return result;
}

SPResultChain SPUnitBase::stop(bool isSync)
{
    auto result = SP_RESULT_CHAIN();
    result.wait();
    result.finish();
    return result;
}

SPResultChain SPUnitBase::seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag)
{
    auto result = SP_RESULT_CHAIN();
    result.wait();
    result.finish();
    return result;
}

SPResultChain SPUnitBase::pause(bool isSync)
{
    auto result = SP_RESULT_CHAIN();
    result.wait();
    result.finish();
    return result;
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

bool SPUnitBase::_process(std::shared_ptr<sp::Pipeline> pipeline)
{
    for (auto wunit : _outUnits) {
        if (auto unit = dynamic_pointer_cast<SPUnitBase>(wunit.lock())) {
            if (unit->_process(pipeline) == false)
                return false;
        }
    }
    
    return true;
}
