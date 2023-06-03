//
//  IPreviewManager.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/5/29.
//

#ifndef IPreviewManager_hpp
#define IPreviewManager_hpp

#include <memory>

class IPreviewManager {
public:
    static std::shared_ptr<IPreviewManager> createIPreviewManager();
public:
    virtual bool setParentViews(void *parents) = 0;
    
    virtual bool render(void *data, int width, int height) = 0;
};

#endif /* IPreviewManager_hpp */
