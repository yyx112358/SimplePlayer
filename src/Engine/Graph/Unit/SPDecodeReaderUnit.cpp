//
//  SPDecodeReaderUnitBase.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/3/11.
//

#include "SPDecodeReaderUnit.hpp"
#include "SPLog.h"
#include "ImageWriterUIImage.h"

#include "SPDecodeReaderFF.hpp"
#include "ISPTaskQueue.hpp"

using namespace sp;


using namespace sp;

SPDecodeReaderUnit::SPDecodeReaderUnit(std::shared_ptr<ISPGraphContext>context)
    : SPUnitBase(context)
{
    
}

SPDecodeReaderUnit::~SPDecodeReaderUnit()
{
}

SPResultChain SPDecodeReaderUnit::init(bool isSync) 
{
    auto result = SP_RESULT_CHAIN();
    SPTask task;
    task.isAsync = false;
    task.msg.callback = [wthis = weak_from_this(), result]() mutable {
        auto sthis = dynamic_pointer_cast<SPDecodeReaderUnit>(wthis.lock());
        if (sthis == nullptr)
            return false;
        
        std::unique_ptr<SPDecodeReaderFF> decoder = std::make_unique<SPDecodeReaderFF>();
        decoder->init(sthis->_path);
        sthis->_decoders[0] = std::move(decoder);
        
        result.finish();
        return true;
    };
    
    if (_processThread && isSync == false)
        _processThread->runAsync(std::move(task));
    else
        task.msg.callback();
    return result;
}

void SPDecodeReaderUnit::__SetVideoPath__(const std::string &path) {
    _path = path;
//    SPTask task;
//    task.msg.params.push_back(path);
//    task.isAsync = false;
//    task.msg.callback = [wthis = weak_from_this(), path]() -> SPParam {
//        auto sthis = dynamic_pointer_cast<SPDecodeReaderUnit>(wthis.lock());
//        if (sthis == nullptr)
//            return false;
//        
//        std::unique_ptr<SPDecodeReaderFF> decoder = std::make_unique<SPDecodeReaderFF>();
//        decoder->init(path);
//        sthis->_decoders[0] = std::move(decoder);
//        
//        return true;
//    };
//    auto f = _processThread->runAsync(std::move(task));
//    auto p = f.get();
//    SPLOGI("%d", std::get<bool>(p));
}
