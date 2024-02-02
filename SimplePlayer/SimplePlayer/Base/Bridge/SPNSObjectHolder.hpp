//
//  SPNSObjectHolder.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#include <memory>

/**
 * Apple端NSObject包装器，使用RAII和ARC自动管理内存
 * 读取时务必区分强持有和弱持有
 * 用法示例：
 *  构造：
 *      NSString *string = [NSString stringWithString:@"test"];
 *      SPNSObjectHolder holder((void *)CFBridgingRetain(s));
 *  读取：
 *      NSString *s = (NSMutableString *)CFBridgingRelease(holder.getStrongObject()); // 强持有
 *      NSAssert(holder.getWeakObject() != nil, ""); // 弱持有。
 */
class SPNSObjectHolder {
public:
    SPNSObjectHolder();
    SPNSObjectHolder(void *ocPtr);
    SPNSObjectHolder(SPNSObjectHolder &&other);
    
    SPNSObjectHolder& operator=(void *ocPtr);
    SPNSObjectHolder& operator=(SPNSObjectHolder &&other);
    
    ~SPNSObjectHolder();
    
    void *getStrongObject();
    void *getWeakObject();

protected:
    class Impl;
    std::unique_ptr<Impl> _pImpl;
};
