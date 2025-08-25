//
//  SPPreviewUnitApple.hpp
//  SimplePlayer
//
//  Created by yyx on 2025/8/25.
//

#pragma once

#include "SPUnitBase.hpp"
#include "IPreviewManager.hpp"

namespace sp {



class SPPreviewUnitApple : public SPUnitBase {
public:
    SPPreviewUnitApple(std::shared_ptr<ISPGraphContext>context);
    virtual ~SPPreviewUnitApple();

    virtual SPResultChain init(bool isSync) override;
    virtual SPResultChain uninit(bool isSync) override;
    virtual SPResultChain start(bool isSync) override;
//    virtual SPResultChain stop(bool isSync) override;
//    virtual SPResultChain seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag) override;
//    virtual SPResultChain pause(bool isSync) override;

    virtual bool setParentViews(void *parents) { return _previewManager->setParentViews(parents); }
    
public:
    virtual const char *UNIT_NAME() const override { return "SPPreviewUnitApple"; }
    
    virtual bool _process(std::shared_ptr<sp::Pipeline> pipeline) override;

private:
    std::shared_ptr<IPreviewManager> _previewManager;

};



}
