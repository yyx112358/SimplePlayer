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


class SPUnitBase : public ISPUnit {
public:
    SPUnitBase(std::shared_ptr<ISPGraphContext>context);
    virtual ~SPUnitBase();
    
// ISPMediaControl
public:
    virtual std::future<bool> init(bool isSync) override;
    virtual std::future<bool> uninit(bool isSync) override;
    
    virtual bool isInited() const override { return _isInited; }
    
    virtual std::future<bool> start(bool isSync) override;
    virtual std::future<bool> stop(bool isSync) override;
    virtual std::future<bool> seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag) override;
    virtual std::future<bool> pause(bool isSync) override;
    
// ISPGraphListener
public:
    virtual void processMessage(SPMsg msg) override {}
    
// ISPUnit
public:
    virtual bool connect(std::shared_ptr<ISPUnit> unit) override;
    virtual bool disconnect(std::shared_ptr<ISPUnit> unit) override;
    virtual bool disconnectAll() override;
    
    virtual bool _process(std::shared_ptr<sp::Pipeline> pipeline) override;
    
protected:
    std::weak_ptr<ISPGraphContext> _context;
    
    std::list<std::weak_ptr<ISPUnit>> _inUnits;
    std::list<std::weak_ptr<ISPUnit>> _outUnits;
    
    bool _isInited = false;
    
};


}

