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
    
    virtual SPResultChain init(bool isSync) override;
    virtual SPResultChain uninit(bool isSync) override;
    
    virtual SPResultChain start(bool isSync) override;
//    virtual SPResultChain stop(bool isSync) override;
//    virtual SPResultChain seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag) override;
//    virtual SPResultChain pause(bool isSync) override;
    
    virtual void setProcessThread(std::shared_ptr<ISPTaskQueue> queue) { _processThread = queue; }
public:
    const char *UNIT_NAME() const override { return "SPDecodeReaderUnit"; }
    
    void __SetVideoPath__(const std::string &path); // TODO: 使用Timeline
    std::string _path;
    
protected:
    virtual bool _process(std::shared_ptr<sp::Pipeline> pipeline) override;
    
protected:
    std::map<int, std::unique_ptr<SPDecodeReaderFF>> _decoders;
    std::shared_ptr<ISPTaskQueue> _processThread;
    std::function<SPParam()> _processBlk;
};

}
 
