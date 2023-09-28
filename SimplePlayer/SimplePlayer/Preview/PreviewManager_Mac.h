//
//  PreviewManager_Mac.h
//  SimplePlayer
//
//  Created by YangYixuan on 2023/5/29.
//

#pragma once

#include "IPreviewManager.hpp"

class PreviewManager_Mac : public IPreviewManager {
public:
    virtual ~PreviewManager_Mac();
public:
    bool setParentViews(void *parents) override;
    
    bool render(std::shared_ptr<sp::Pipeline> pipeline) override;
    
};

