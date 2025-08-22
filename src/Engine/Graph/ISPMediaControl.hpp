//
//  ISPMediaControl.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/1/13.
//

#pragma once

#include <chrono>
#include <bitset>
#include <future>

#include "SPResultChain.h"

namespace sp {

class ISPMediaControl
{
public:
    enum class ESeekFlag {
        Last,
        OnGoing,
    };
    typedef std::bitset<8> SeekFlag;
    
public:
    virtual SPResultChain init(bool isSync) = 0;
    virtual SPResultChain uninit(bool isSync) = 0;
    
    virtual bool isInited() const = 0;
    
public:
    virtual SPResultChain start(bool isSync) = 0;
    virtual SPResultChain stop(bool isSync) = 0;
    virtual SPResultChain seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag) = 0;
    virtual SPResultChain pause(bool isSync) = 0;
//    virtual SPResultChain flush(bool isSync) = 0;
//    virtual SPResultChain reset(bool isSync) = 0;
};

}
