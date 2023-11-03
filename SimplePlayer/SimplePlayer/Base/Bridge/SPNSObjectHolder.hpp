//
//  SPNSObjectHolder.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#include <memory>

class SPNSObjectHolderImpl;

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
    SPNSObjectHolder() = default;
    SPNSObjectHolder(void *ocPtr);
    SPNSObjectHolder& operator=(void *ocPtr);
    
    virtual ~SPNSObjectHolder() = default;
    
    void *getStrongObject();
    void *getWeakObject();

protected:
    std::shared_ptr<SPNSObjectHolderImpl> impl = nullptr;
};
