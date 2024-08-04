//
//  SPNSObjectHolder.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#include "SPNSobjectHolder.hpp"
#include "SPLog.h"

#include <Foundation/Foundation.h>

class SPNSObjectHolder::Impl {
public:
    Impl(void *ocPtr) {
        obj = (NSObject *)CFBridgingRelease(ocPtr);
        SPASSERT([obj isKindOfClass:[NSObject class]]); // SPNSObjectHolderImpl only hold one NSObject
    }
    ~Impl() {
        obj = nil;
    }
    
    NSObject *obj = nil;
};

// 必须在cpp文件中显式声明构造、析构函数，避免出现imcomplete type错误
// https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile
SPNSObjectHolder::SPNSObjectHolder() = default;
SPNSObjectHolder::~SPNSObjectHolder() = default;

SPNSObjectHolder::SPNSObjectHolder(void *ocPtr) : _pImpl(std::make_unique<Impl>(ocPtr)) {
    
}
SPNSObjectHolder::SPNSObjectHolder(SPNSObjectHolder &&other) : _pImpl(std::move(other._pImpl)) {
    
}

SPNSObjectHolder& SPNSObjectHolder::operator=(void *ocPtr) {
    _pImpl = std::make_unique<Impl>(ocPtr);
    return *this;
}

SPNSObjectHolder& SPNSObjectHolder::operator=(SPNSObjectHolder &&other) {
    _pImpl = std::move(other._pImpl);
    return *this;
}

void *SPNSObjectHolder::getStrongObject() {
    if (_pImpl != nullptr)
        return (void *)CFBridgingRetain(_pImpl->obj);
    else
        return nullptr;
}

void *SPNSObjectHolder::getWeakObject() {
    if (_pImpl != nullptr)
        return (__bridge void *)_pImpl->obj;
    else
        return nullptr;
}
