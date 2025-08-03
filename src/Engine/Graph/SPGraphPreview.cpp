//
//  SPGraphPreview.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/1/13.
//

#include "SPGraphPreview.hpp"
#include "SPLog.h"

// TODO: 改成工厂模式构造
#include "SPDecodeReaderUnit.hpp"
#include "ISPTaskQueue.hpp"


using namespace sp;

constexpr char SPUNIT_NAME_VIDEO_DECODE_READER[] = "video decode reader";

std::future<bool> SPGraphPreview::init(bool isSync) {
    std::promise<bool> result;
    if (isSync == false)
        SPASSERT_NOT_IMPL;
    if (_model == nullptr) {
        result.set_value(false);
        return result.get_future();
    }
    
    std::shared_ptr<ISPTaskQueue> _decodeThread = ISPTaskQueue::Create();
    {
        auto unit = std::make_shared<SPDecodeReaderUnit>(shared_from_this());
        unit->setProcessThread(_decodeThread);
        unit->__SetVideoPath__(_model->videoTracks.front().front());
        _units.emplace(SPUNIT_NAME_VIDEO_DECODE_READER, unit);
        _sourceVideoUnit = unit;
    }
    
//    decoder = std::make_shared<sp::SPDecodeReaderFF>();
//    decoder->init(_model->videoTracks.front().front());
//    
//    audioRenderer = std::make_shared<sp::AudioRendererManager>();
//    audioRenderer->init();
//    audioRenderer->setInputQueue(decoder->_audioQueue);
//    
//    audioOutput = std::make_shared<sp::AudioOutputManager>(shared_from_this());
//    audioOutput->init();
//    audioOutput->setInputQueue(audioRenderer->getOutputQueue());
//    
//    preview = IPreviewManager::createIPreviewManager();
//    preview->setParentViews(_parentPlayerView);
//    preview->setPipelineQueue(decoder->_videoQueue);
//    
//    decoder->start(false);
//    audioRenderer->start(false);
//    audioOutput->start(false);
//    preview->start(false);
    
    result.set_value(true);
    return result.get_future();
}

std::future<bool> SPGraphPreview::uninit(bool isSync) {
    std::promise<bool> result;
    if (isSync == false)
        SPASSERT_NOT_IMPL;
    if (_model == nullptr) {
        result.set_value(false);
        return result.get_future();
    }
    
    std::future<bool> futureDecoder = decoder->stop(false);
    audioRenderer->stop(false);
    audioOutput->stop(false);

    // futureDecoder.wait();
    decoder->unInit();
    decoder = nullptr;

    audioRenderer->uninit();
    audioRenderer = nullptr;

    audioOutput->uninit();
    audioOutput = nullptr;

    preview = nullptr;
    
    result.set_value(true);
    return result.get_future();
}

std::future<bool> SPGraphPreview::updateModel(const SPMediaModel &model, bool isSync) {
    std::promise<bool> result;
    _model = std::make_unique<SPMediaModel>(model);
    
    result.set_value(true);
    return result.get_future();
}


std::future<bool> SPGraphPreview::start(bool isSync) {
    std::promise<bool> result;
    
//    if (decoder != nullptr)
//        decoder->start(false);
//    if (audioOutput != nullptr)
//        audioOutput->start(false);
    std::vector<std::future<bool>> futures;
    if (auto unit = _sourceVideoUnit.lock())
        futures.emplace_back(unit->start(isSync));
    
    if (isSync)
        result.set_value(true);
    else {
        auto block = [result = std::move(result), futures = std::move(futures)]() {
//            bool flag = true;
//            for (auto &f : futures) {
//                flag &= f.get();
//            }
//            result = flag;
        };
        // TODO: 在线程队列里执行
        block();
    }
    return result.get_future();
}

std::future<bool> SPGraphPreview::stop(bool isSync) {
    std::promise<bool> result;
    
    result.set_value(true);
    return result.get_future();
}

std::future<bool> SPGraphPreview::seek(std::chrono::time_point<std::chrono::steady_clock>pts, bool isSync, SeekFlag flag) {
    std::promise<bool> result;
    
//    if (decoder != nullptr)
//        decoder->start(false);
//    if (audioOutput != nullptr)
//        audioOutput->start(false);    
    
    result.set_value(true);
    return result.get_future();
}

std::future<bool> SPGraphPreview::pause(bool isSync) {
    std::promise<bool> result;
    
    if (decoder != nullptr)
        decoder->pause(false);
    if (audioOutput != nullptr)
        audioOutput->pause(false);
    
    
    result.set_value(true);
    return result.get_future();
}
//std::future<bool> SPGraphPreview::start(bool isSync) {
//    std::promise<bool> result;
//    
//    if (decoder != nullptr)
//        decoder->start(false);
//    if (audioOutput != nullptr)
//        audioOutput->start(false);
//    
//    
//    result.set_value(true);
//    return result.get_future();
//}
//std::future<bool> SPGraphPreview::start(bool isSync) {
//    std::promise<bool> result;
//    
//    if (decoder != nullptr)
//        decoder->start(false);
//    if (audioOutput != nullptr)
//        audioOutput->start(false);
//    
//    
//    result.set_value(true);
//    return result.get_future();
//}

bool SPGraphPreview::addListener(SPMsgID msgID, std::shared_ptr<ISPGraphContextListener> listener, SPMsgConnectType connectType) {
    
    return true;
}


