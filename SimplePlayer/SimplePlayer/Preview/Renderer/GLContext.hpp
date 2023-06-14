//
//  GLContext.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#ifndef GLContext_hpp
#define GLContext_hpp

#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#include <mutex>
#include <any>

namespace sp {

/**
 * GL上下文
 * TODO: 抽出公共跨平台部分
 */
class GLContext {
public:
    virtual ~GLContext();
    
    virtual bool init();
    
    virtual NSOpenGLContext *context();
    
    /// 切换到本Context
    virtual bool switchContext();
    
    // 在OpenGL绘制完成后，调用flush方法将绘制的结果显示到窗口上
    // 应在最后一个GL操作完成时调用
    virtual bool flush();
    
    // 检查错误
    static bool CheckGLError(const char *file, int line);
    
protected:
    NSOpenGLContext *_context;
    std::mutex _mutex;
};

#define CheckError() GLContext::CheckGLError(__FILE__, __LINE__)


class Parameter {
public:
    std::any parameter;
    std::any shadowParameter;
    
    void update(const std::any&input) {
        shadowParameter = input;
    }
    std::any& get() {
        return shadowParameter.has_value() ? shadowParameter : parameter;
    }
    std::any getReal() {
        return parameter;
    }
    std::any& getAndUpdate() {
        parameter.swap(shadowParameter);
        shadowParameter.reset();
        return get();
    }
    bool isUpdated() {
        return shadowParameter.has_value() == false;
    }
};

}

#endif /* GLContext_hpp */
