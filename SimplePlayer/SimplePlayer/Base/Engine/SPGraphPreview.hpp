//
//  SPGraphPreview.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/1/13.
//

#pragma once
#include "ISPGraph.hpp"
#include "SPDecodeReaderUnit.hpp"
#include "SPDecodeReaderFF.hpp"
#include "AudioRendererManager.hpp"
#include "AudioOutputManager.hpp"
#include "IPreviewManager.hpp"

namespace sp {

class SPGraphPreview : public ISPGraph {
public:
    SPGraphPreview() = default;
    virtual std::future<bool> init(bool isSync) override;
    virtual std::future<bool> uninit(bool isSync) override;
    
    virtual bool isInited() const override {return false;}
    
    virtual std::future<bool> updateModel(const SPMediaModel& model, bool isSync) override;
    
    void *_parentPlayerView = nullptr; // TODO: 临时处理方案，后续需重构Preview
    
// ISPMediaControl
public:
    virtual std::future<bool> start(bool isSync) override;
    virtual std::future<bool> stop(bool isSync) override;
    virtual std::future<bool> seek(std::chrono::time_point<std::chrono::steady_clock>pts, bool isSync, SeekFlag flag) override;
    virtual std::future<bool> pause(bool isSync) override;
//    virtual std::future<bool> flush(bool isSync) override;
//    virtual std::future<bool> reset(bool isSync) override;
    
// ISPGraphContext
public:
    virtual bool addListener(SPMsgID, std::shared_ptr<ISPGraphContextListener>, SPMsgConnectType connectType = SPMsgConnectType::AUTO) override;
    
    virtual void postMessage(SPMsg msg, int type) override {}
    
    
    
protected:
    std::unique_ptr<SPMediaModel> _model;
    std::unique_ptr<SPMediaModel> _modelShadow;
    
    std::shared_ptr<IPreviewManager> preview;
    std::shared_ptr<sp::AudioRendererManager> audioRenderer;
    std::shared_ptr<sp::AudioOutputManager> audioOutput;
    std::shared_ptr<sp::SPDecodeReaderFF> decoder;
    
    std::map<std::string, std::shared_ptr<ISPUnit>> _units;
    std::weak_ptr<ISPUnit> _sourceVideoUnit, _sourceAudioUnit;
};

}
