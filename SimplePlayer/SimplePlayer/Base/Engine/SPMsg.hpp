//
//  SPMsg.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/1/4.
//

#pragma once

#include "SPParam.hpp"
#include <vector>
#include <future>

namespace sp {
enum class SPMsgID {
    UNKNOWN,
    INFO_AUDIO_CLOCK
};



class SPMsg {
public:
    SPMsgID id;
    std::vector<SPParam> params;
    std::future<SPParam> result;
    std::function<SPParam()> callback;
};
}
