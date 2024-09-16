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

using namespace std;
using namespace sp;

SPDecodeReaderUnit::SPDecodeReaderUnit(std::shared_ptr<ISPGraphContext>context)
    : SPUnitBase(context)
{
    
}

SPDecodeReaderUnit::~SPDecodeReaderUnit()
{
}

std::future<SPParam> SPDecodeReaderUnit::init(bool isSync) 
{
    return _runTask(isSync, [path = _videoPath] (SPUnitBase * const sthis) -> SPParam 
    {
        auto self = static_cast<SPDecodeReaderUnit * const>(sthis);
        
        unique_ptr<SPDecodeReaderFF> decoder = std::make_unique<SPDecodeReaderFF>();
//        decoder->_processThread = self->_processThread;
        if (decoder->init(path)) {
            self->_decoders[0] = std::move(decoder);
            return true;
        } else {
            return false;
        }
    });
}

std::future<SPParam> SPDecodeReaderUnit::uninit(bool isSync)
{
    return _runTask(isSync, [] (SPUnitBase * const sthis) -> SPParam
    {
        auto self = static_cast<SPDecodeReaderUnit * const>(sthis);

        bool result = true;
        for (auto &decoder : self->_decoders)
            result &= decoder.second->unInit();
        return result;
    });
}

std::future<SPParam> SPDecodeReaderUnit::start(bool isSync)
{
    SPTask task;
    SPASSERT0(isSync || _processThread);
    task.isAsync = isSync == false && _processThread != nullptr;
    task.work = [wthis = weak_from_this(), path = _videoPath] (SPTask &) -> SPParam {
        auto sthis = dynamic_pointer_cast<SPDecodeReaderUnit>(wthis.lock());
        if (sthis == nullptr)
            return false;

        for (auto it = sthis->_decoders.begin(); it != sthis->_decoders.end(); ++it) {
            it->second->start(true);
        }

        return true;
    };
    return _processThread->run(std::move(task));
}
