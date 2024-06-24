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
    virtual std::future<bool> init(bool isSync) = 0;
    virtual std::future<bool> uninit(bool isSync) = 0;
    
    virtual bool isInited() const = 0;
    
public:
    virtual std::future<bool> start(bool isSync) = 0;
    virtual std::future<bool> stop(bool isSync) = 0;
    virtual std::future<bool> seek(std::chrono::time_point<std::chrono::steady_clock> pts, bool isSync, SeekFlag flag) = 0;
    virtual std::future<bool> pause(bool isSync) = 0;
//    virtual std::future<bool> flush(bool isSync) = 0;
//    virtual std::future<bool> reset(bool isSync) = 0;
};

}
