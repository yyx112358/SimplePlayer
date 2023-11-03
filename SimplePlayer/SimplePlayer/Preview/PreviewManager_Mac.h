//
//  PreviewManager_Mac.h
//  SimplePlayer
//
//  Created by YangYixuan on 2023/5/29.
//

#pragma once

#include "IPreviewManager.hpp"
#include <thread>

class PreviewManager_Mac : public IPreviewManager {
public:
    virtual ~PreviewManager_Mac();
public:
    bool setParentViews(void *parents) override;
    
    bool start(bool isSync) override;
    bool stop(bool isSync) override;
    
    bool addPipeline(std::shared_ptr<sp::Pipeline> pipeline) override;
    
protected:
    bool _render() override {return true;}
    
private:
    static CVReturn _displayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext);

    CVDisplayLinkRef _displayLink = NULL;
};

