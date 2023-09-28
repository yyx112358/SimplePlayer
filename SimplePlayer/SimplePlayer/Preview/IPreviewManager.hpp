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

#include "VideoFrameBase.hpp"

class IPreviewManager {
public:
    static std::shared_ptr<IPreviewManager> createIPreviewManager();
    virtual ~IPreviewManager() {}
public:
    virtual bool setParentViews(void *parents) = 0;
    
    virtual bool render(std::optional<sp::VideoFrame> frame) = 0;
};

#endif /* IPreviewManager_hpp */
