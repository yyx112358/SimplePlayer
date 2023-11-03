//
//  SPNSObjectHolder.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#include "SPNSobjectHolder.hpp"
#include "SPLog.h"

#include <Foundation/Foundation.h>

class SPNSObjectHolderImpl {
public:
    SPNSObjectHolderImpl(void *ocPtr) {
        obj = (NSObject *)CFBridgingRelease(ocPtr);
        SPASSERT([obj isKindOfClass:[NSObject class]]); // SPNSObjectHolderImpl only hold one NSObject
    }
    
    NSObject *obj = nil;
};

SPNSObjectHolder::SPNSObjectHolder(void *ocPtr) {
    impl = std::make_shared<SPNSObjectHolderImpl>(ocPtr);
}

SPNSObjectHolder& SPNSObjectHolder::operator=(void *ocPtr) {
    impl = std::make_shared<SPNSObjectHolderImpl>(ocPtr);
    return *this;
}

void *SPNSObjectHolder::getStrongObject() {
    if (impl != nullptr)
        return (void *)CFBridgingRetain(impl->obj);
    else
        return nullptr;
}

void *SPNSObjectHolder::getWeakObject() {
    if (impl != nullptr)
        return (__bridge void *)impl->obj;
    else
        return nullptr;
}
