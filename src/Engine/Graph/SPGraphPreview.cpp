//
//  SPGraphPreview.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/1/13.
//

#include "SPGraphPreview.hpp"
#include "SPLog.h"


using namespace sp;

std::future<bool> SPGraphPreview::init(bool isSync) {
    std::promise<bool> result;
    if (isSync == false)
        SPASSERT_NOT_IMPL;
    if (_model == nullptr) {
        result.set_value(false);
        return result.get_future();
    }
    
    decoder = std::make_shared<sp::DecoderManager>();
    decoder->init(_model->videoTracks.front().front());
    
    audioRenderer = std::make_shared<sp::AudioRendererManager>();
    audioRenderer->init();
    audioRenderer->setInputQueue(decoder->_audioQueue);
    
    audioOutput = std::make_shared<sp::AudioOutputManager>();
    audioOutput->init();
    audioOutput->setInputQueue(audioRenderer->getOutputQueue());
    
    preview = IPreviewManager::createIPreviewManager();
    preview->setParentViews(_parentPlayerView);
    preview->setPipelineQueue(decoder->_videoQueue);
    
    decoder->start(false);
    audioRenderer->start(false);
    audioOutput->start(false);
    preview->start(false);
    
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
    
    if (decoder != nullptr)
        decoder->start(false);
    if (audioOutput != nullptr)
        audioOutput->start(false);
    
    
    result.set_value(true);
    return result.get_future();
}

std::future<bool> SPGraphPreview::stop(bool isSync) {
    std::promise<bool> result;
    
    result.set_value(true);
    return result.get_future();
}

//std::future<bool> SPGraphPreview::seek(bool isSync) {
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
