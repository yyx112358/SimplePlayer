//
//  ISPGraphContext.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/1/4.
//

#pragma once
#include "SPMsg.hpp"


namespace sp {



class ISPGraphContext {
public:
    virtual bool addListener(SPMsgID, std::weak_ptr<void>) = 0;
    
};


}
