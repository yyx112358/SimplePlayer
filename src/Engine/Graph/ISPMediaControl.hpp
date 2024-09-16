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

#include "SPParam.hpp"

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
    virtual std::future<SPParam> init(bool isSync) = 0;
    virtual std::future<SPParam> uninit(bool isSync) = 0;
    
    virtual bool isInited() const = 0;
    
public:
    virtual std::future<SPParam> start(bool isSync) = 0;
    virtual std::future<SPParam> stop(bool isSync) = 0;
    virtual std::future<SPParam> seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag) = 0;
    virtual std::future<SPParam> pause(bool isSync) = 0;
//    virtual std::future<SPParam> flush(bool isSync) = 0;
//    virtual std::future<SPParam> reset(bool isSync) = 0;
};

}
