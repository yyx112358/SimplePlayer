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
    UNINIT,
    INFO_AUDIO_CLOCK
};

enum class SPMsgConnectType {
    AUTO,
    DIRECT,
    QUEUE,
};

enum class SPMsgPriority {
    NORMAL,
    HIGH,
};


class SPMsg {
public:
    SPMsgID id;
    std::vector<SPParam> params;
    std::promise<SPParam> result;
    std::function<SPParam()> callback;
};
}
