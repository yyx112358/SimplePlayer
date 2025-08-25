//
//  SPPreviewUnitApple.cpp
//  SimplePlayer
//
//  Created by yyx on 2025/8/25.
//

#include "SPPreviewUnitApple.hpp"
#include "PreviewManager_Mac.h"


namespace sp {


SPPreviewUnitApple::SPPreviewUnitApple(std::shared_ptr<ISPGraphContext>context)
    : SPUnitBase(context)  {
    _previewManager = IPreviewManager::createIPreviewManager();
}

SPPreviewUnitApple::~SPPreviewUnitApple() {
    _previewManager = nullptr;
}

SPResultChain SPPreviewUnitApple::init(bool isSync) {
    return SPUnitBase::init(isSync);
}

SPResultChain SPPreviewUnitApple::uninit(bool isSync) {
    return SPUnitBase::uninit(isSync);
}

SPResultChain SPPreviewUnitApple::start(bool isSync) {
    auto result = SP_RESULT_CHAIN();
    _previewManager->start(isSync);
    
    result.finish();
    return result;
}


bool SPPreviewUnitApple::_process(std::shared_ptr<sp::Pipeline> pipeline) {
    return _previewManager->setPipelineQueue(pipeline);
}

}
