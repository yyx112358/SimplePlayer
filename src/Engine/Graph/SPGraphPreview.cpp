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

SPResultChain SPGraphPreview::init(bool isSync) {
    auto result = SP_RESULT_CHAIN();
    if (_model == nullptr) {
        result.finish(SPResultChain::RESULT_CODE::FAIL);
        return result;
    }
    
    std::shared_ptr<ISPTaskQueue> _decodeThread = ISPTaskQueue::Create();
    {
        auto unit = std::make_shared<SPDecodeReaderUnit>(shared_from_this());
        unit->setProcessThread(_decodeThread);
        unit->__SetVideoPath__(_model->videoTracks.front().front());
        _units.emplace(SPUNIT_NAME_VIDEO_DECODE_READER, unit);
        _sourceVideoUnit = unit;
        result.after(unit->init(isSync));
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
    
    if (auto code = result.wait(); code != SPResultChain::RESULT_CODE::OK) {
        result.finish(code);
    } else {
        result.finish(SPResultChain::RESULT_CODE::OK);
    }

    return result;
}

SPResultChain SPGraphPreview::uninit(bool isSync) {
    auto result = SP_RESULT_CHAIN();
    if (_model == nullptr) {
        result.finish(SPResultChain::RESULT_CODE::FAIL);
        return result;
    }
    
    if (auto unit = _sourceVideoUnit.lock()) {
        result.after(unit->stop(isSync));
    }

    audioRenderer->stop(false);
    audioOutput->stop(false);


    audioRenderer->uninit();
    audioRenderer = nullptr;

    audioOutput->uninit();
    audioOutput = nullptr;

    preview = nullptr;
    
    if (auto code = result.wait(); code != SPResultChain::RESULT_CODE::OK) {
        result.finish(code);
    } else {
        result.finish(SPResultChain::RESULT_CODE::OK);
    }

    return result;
}

SPResultChain SPGraphPreview::updateModel(const SPMediaModel &model, bool isSync) {
    auto result = SP_RESULT_CHAIN();
    _model = std::make_unique<SPMediaModel>(model);
    
    if (auto code = result.wait(); code != SPResultChain::RESULT_CODE::OK) {
        result.finish(code);
    } else {
        result.finish(SPResultChain::RESULT_CODE::OK);
    }
    
    return result;
}


SPResultChain SPGraphPreview::start(bool isSync) {
    auto result = SP_RESULT_CHAIN();
    std::vector<SPResultChain> futures;

    if (auto code = result.wait(); code != SPResultChain::RESULT_CODE::OK) {
        result.finish(code);
    } else {
        result.finish(SPResultChain::RESULT_CODE::OK);
    }
    return result;
}

SPResultChain SPGraphPreview::stop(bool isSync) {
    auto result = SP_RESULT_CHAIN();
    if (auto code = result.wait(); code != SPResultChain::RESULT_CODE::OK) {
        result.finish(code);
    } else {
        result.finish(SPResultChain::RESULT_CODE::OK);
    }
    return result;
}

SPResultChain SPGraphPreview::seek(std::chrono::time_point<std::chrono::steady_clock>pts, bool isSync, SeekFlag flag) {
    auto result = SP_RESULT_CHAIN();
    if (auto code = result.wait(); code != SPResultChain::RESULT_CODE::OK) {
        result.finish(code);
    } else {
        result.finish(SPResultChain::RESULT_CODE::OK);
    }
    return result;
}

SPResultChain SPGraphPreview::pause(bool isSync) {
    auto result = SP_RESULT_CHAIN();
    if (auto code = result.wait(); code != SPResultChain::RESULT_CODE::OK) {
        result.finish(code);
    } else {
        result.finish(SPResultChain::RESULT_CODE::OK);
    }
    return result;
}

//SPResultChain SPGraphPreview::start(bool isSync) {
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
//SPResultChain SPGraphPreview::start(bool isSync) {
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


