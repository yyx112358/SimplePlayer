//
//  SPDecodeReaderUnit.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/3/11.
//

#pragma once

#include <map>
#include <thread>

#include "SPUnitBase.hpp"

namespace sp {

class SPDecodeReaderFF;
class ISPTaskQueue;

class SPDecodeReaderUnit : public SPUnitBase {
public:

public:
    SPDecodeReaderUnit(std::shared_ptr<ISPGraphContext>context);
    virtual ~SPDecodeReaderUnit();
    
    virtual std::future<bool> init(bool isSync) override;
//    virtual std::future<bool> uninit(bool isSync) override;
    
//    virtual std::future<bool> start(bool isSync) override;
//    virtual std::future<bool> stop(bool isSync) override;
//    virtual std::future<bool> seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag) override;
//    virtual std::future<bool> pause(bool isSync) override;
    
    virtual void setProcessThread(std::shared_ptr<ISPTaskQueue> queue) { _processThread = queue; }
public:
    const char *UNIT_NAME() const override { return "SPDecodeReaderUnit"; }
    
    void __SetVideoPath__(const std::string &path); // TODO: 使用Timeline
    
protected:
    std::map<int, std::unique_ptr<SPDecodeReaderFF>> _decoders;
    std::shared_ptr<ISPTaskQueue> _processThread;
};

}
 
