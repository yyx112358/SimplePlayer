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
    task.isAsync = isSync;
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

SPResultChain SPDecodeReaderUnit::uninit(bool isSync) {
    auto result = SP_RESULT_CHAIN();
    SPTask task;
    task.isAsync = isSync;
    task.msg.callback = [wthis = weak_from_this(), result]() mutable {
        auto sthis = dynamic_pointer_cast<SPDecodeReaderUnit>(wthis.lock());
        if (sthis == nullptr)
            return false;

        for (auto &decoder : sthis->_decoders) {
            decoder.second->stop(true);
            decoder.second = nullptr;
        }
        sthis->_decoders.clear();
        
        result.finish();
        return true;
    };
    
    if (_processThread && isSync == false)
        _processThread->runAsync(std::move(task));
    else
        task.msg.callback();
    return result;
}

SPResultChain SPDecodeReaderUnit::start(bool isSync) {
    auto result = SP_RESULT_CHAIN();
    SPTask task;
    task.isAsync = isSync;
    task.msg.callback = [wthis = weak_from_this(), result]() mutable {
        auto sthis = dynamic_pointer_cast<SPDecodeReaderUnit>(wthis.lock());
        if (sthis == nullptr)
            return false;

        for (auto &decoder : sthis->_decoders) {
            decoder.second->start(true);
        }
        result.finish();
        return true;
    };
    
    if (_processThread && isSync == false)
        _processThread->runAsync(std::move(task));
    else
        task.msg.callback();
    
    _processBlk = [wthis = weak_from_this()]() {
        auto sthis = dynamic_pointer_cast<SPDecodeReaderUnit>(wthis.lock());
        if (sthis == nullptr)
            return false;
        
        auto pipeline = std::make_shared<sp::Pipeline>();
        static int PIPELINE_ID = 0;
        pipeline->id = PIPELINE_ID++;
        
        if (sthis->_process(pipeline)) {
            SPTask task2;
            task2.isAsync = false;
            task2.msg.callback = sthis->_processBlk;
            sthis->_processThread->runAsync(std::move(task2));
        }
        
        return true;
    };
    
    SPTask task2;
    task2.isAsync = false;
    task2.msg.callback = _processBlk;
    if (_processThread)
        _processThread->runAsync(std::move(task2));
    
    return result;
}


void SPDecodeReaderUnit::__SetVideoPath__(const std::string &path) {
    _path = path;
}

bool SPDecodeReaderUnit::_process(std::shared_ptr<sp::Pipeline> pipeline) {
    for (auto &decoder : _decoders) {
        pipeline = decoder.second->_videoQueue->deque();
        if (SPUnitBase::_process(pipeline) == false)
            return false;
        if (decoder.second->_audioQueue->size() > 0)
            decoder.second->_audioQueue->deque();
    }
    return true;
}
