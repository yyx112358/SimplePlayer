//
//  SPUnitBase.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/3/5.
//

#pragma once
#include "ISPUnit.hpp"

#include <list>
#include <memory>

namespace sp {

class ISPTaskQueue;

class SPUnitBase : public ISPUnit {
public:
    SPUnitBase(std::shared_ptr<ISPGraphContext>context);
    virtual ~SPUnitBase();
    
// ISPMediaControl
public:
    virtual std::future<SPParam> init(bool isSync) override;
    virtual std::future<SPParam> uninit(bool isSync) override;
    
    virtual bool isInited() const override { return _isInited; }
    
    virtual std::future<SPParam> start(bool isSync) override;
    virtual std::future<SPParam> stop(bool isSync) override;
    virtual std::future<SPParam> seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag) override;
    virtual std::future<SPParam> pause(bool isSync) override;
    
// ISPGraphListener
public:
    virtual void processMessage(SPMsg msg) override {}
    
// ISPUnit
public:
    virtual bool connect(std::shared_ptr<ISPUnit> unit) override;
    virtual bool disconnect(std::shared_ptr<ISPUnit> unit) override;
    virtual bool disconnectAll() override;
    
    virtual bool _process(std::shared_ptr<sp::Pipeline> pipeline) override;
    
    virtual void setProcessThread(std::shared_ptr<ISPTaskQueue> queue) { _processThread = queue; }
protected:
    virtual std::future<SPParam> _runTask(bool isSync, std::function<SPParam(SPUnitBase * const sthis)>callback);
    
protected:
    std::weak_ptr<ISPGraphContext> _context;
    std::shared_ptr<ISPTaskQueue> _processThread;
    
    std::list<std::weak_ptr<ISPUnit>> _inUnits;
    std::list<std::weak_ptr<ISPUnit>> _outUnits;
    
    bool _isInited = false;
    
};


}

