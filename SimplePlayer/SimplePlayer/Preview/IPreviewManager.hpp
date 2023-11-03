//
//  IPreviewManager.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/5/29.
//

#ifndef IPreviewManager_hpp
#define IPreviewManager_hpp

#include <memory>
#include <optional>

#include "Pipeline.hpp"

class IPreviewManager {
public:
    static std::shared_ptr<IPreviewManager> createIPreviewManager();
    virtual ~IPreviewManager() {}
public:
    virtual bool start(bool isSync) = 0;
    virtual bool stop(bool isSync) = 0;
    
    virtual bool setParentViews(void *parents) = 0;
    
    virtual bool setPipelineQueue(std::shared_ptr<sp::SPPipelineQueue> videoQueue, std::shared_ptr<sp::SPPipelineQueue> audioQueue) = 0;
    virtual bool addPipeline(std::shared_ptr<sp::Pipeline> pipeline) = 0;
    
protected:
    virtual bool _render() = 0;
};

#endif /* IPreviewManager_hpp */
