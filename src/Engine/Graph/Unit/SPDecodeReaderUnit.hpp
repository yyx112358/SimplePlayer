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

class SPDecodeReaderUnit : public SPUnitBase {
public:

public:
    SPDecodeReaderUnit(std::shared_ptr<ISPGraphContext>context);
    virtual ~SPDecodeReaderUnit();
    
    virtual std::future<SPParam> init(bool isSync) override;
    virtual std::future<SPParam> uninit(bool isSync) override;
    
    virtual std::future<SPParam> start(bool isSync) override;
//    virtual std::future<SPParam> stop(bool isSync) override;
//    virtual std::future<SPParam> seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag) override;
//    virtual std::future<SPParam> pause(bool isSync) override;
    

public:
    const char *UNIT_NAME() const override { return "SPDecodeReaderUnit"; }
    
    std::string _videoPath; // TODO: 这是临时方案，未来使用Timeline更新
    
protected:
    std::map<int, std::unique_ptr<SPDecodeReaderFF>> _decoders;
};

}
 
