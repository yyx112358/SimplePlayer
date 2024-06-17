//
//  ISPGraphContext.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/1/4.
//

#pragma once
#include "SPMsg.hpp"


namespace sp {


class ISPGraphContextListener;

class ISPGraphContext {
public:
    virtual bool addListener(SPMsgID, std::shared_ptr<ISPGraphContextListener>, SPMsgConnectType connectType = SPMsgConnectType::AUTO) = 0;
    
    virtual void postMessage(SPMsg msg, int type) = 0;
};

class ISPGraphContextListener {
public:
    
    virtual void processMessage(SPMsg msg) = 0;
};


}
