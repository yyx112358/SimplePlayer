//
//  SPGraphPreview.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/1/13.
//

#pragma once
#include "ISPGraph.hpp"
#include "DecoderManager.hpp"
#include "AudioRendererManager.hpp"
#include "AudioOutputManager.hpp"
#include "IPreviewManager.hpp"

namespace sp {

class SPGraphPreview : public ISPGraph {
public:
    SPGraphPreview() = default;
    virtual std::future<bool> init(bool isSync) override;
    virtual std::future<bool> uninit(bool isSync) override;
    
    virtual std::future<bool> updateModel(const SPMediaModel& model, bool isSync) override;
    
    void *_parentPlayerView = nullptr; // TODO: 临时处理方案，后续需重构Preview
    
// ISPMediaControl
public:
    virtual std::future<bool> start(bool isSync) override;
    virtual std::future<bool> stop(bool isSync) override;
//   virtual  std::future<bool> seek(bool isSync) override;
    virtual std::future<bool> pause(bool isSync) override;
//    virtual std::future<bool> flush(bool isSync) override;
//    virtual std::future<bool> reset(bool isSync) override;
    
// ISPGraphContext
public:
    virtual bool addListener(SPMsgID, std::weak_ptr<void>) override { return true; }
    
protected:
    std::unique_ptr<SPMediaModel> _model;
    std::unique_ptr<SPMediaModel> _modelShadow;
    
    std::shared_ptr<IPreviewManager> preview;
    std::shared_ptr<sp::AudioRendererManager> audioRenderer;
    std::shared_ptr<sp::AudioOutputManager> audioOutput;
    std::shared_ptr<sp::DecoderManager> decoder;
};

}
