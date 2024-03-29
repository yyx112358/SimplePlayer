//
//  ISPTaskQueue.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/3/24.
//

#include "ISPTaskQueue.hpp"

#if __APPLE__
#include "SPTaskQueueApple.hpp"
#endif

using namespace sp;

std::shared_ptr<ISPTaskQueue> ISPTaskQueue::Create(std::string name) {
    if (name.size() == 0) {
        static int cnt = 0;
        name = std::string("TaskQueue_") + std::to_string(cnt);
    }
    
#if __APPLE__
    return std::shared_ptr<ISPTaskQueue>(new SPTaskQueueApple(name));
#else
    return nullptr;
#endif
    
}
