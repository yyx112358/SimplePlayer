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
//    static std::shared_ptr<IPreviewManager> createIPreviewManager();
public:
    bool setParentViews(void *parents) override;
    
    bool render(void *data, int width, int height) override;
};

