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

std::future<bool> SPDecodeReaderUnit::init(bool isSync) 
{
    
    return SPUnitBase::init(isSync);
}

void SPDecodeReaderUnit::__SetVideoPath__(const std::string &path) {
    SPTask task;
    task.msg.params.push_back(path);
    task.isAsync = false;
    task.msg.callback = [wthis = weak_from_this(), path]() -> SPParam {
        auto sthis = dynamic_pointer_cast<SPDecodeReaderUnit>(wthis.lock());
        if (sthis == nullptr)
            return false;
        
        std::unique_ptr<SPDecodeReaderFF> decoder = std::make_unique<SPDecodeReaderFF>();
        decoder->init(path);
        sthis->_decoders[0] = std::move(decoder);
        
        return true;
    };
    _processThread->runSync(std::move(task));
}
