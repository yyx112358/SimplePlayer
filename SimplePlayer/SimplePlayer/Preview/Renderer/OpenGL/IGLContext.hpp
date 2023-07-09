//
//  IGLContext.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#pragma once

#include "GLBaseDefine.h"

namespace sp {


/**
 * GL上下文
 */
class IGLContext {
public:
    /// 创建Context
    static std::shared_ptr<IGLContext> CreateGLContext();
    
    /// 检查错误
    static bool CheckGLError(const char *function, int line);

public:
    virtual ~IGLContext() {}
    
    /// 初始化
    virtual bool Init() = 0;
    
    /// 切换到本Context
    virtual bool SwitchContext() = 0;
    
    // 在OpenGL绘制完成后，调用flush方法将绘制的结果显示到窗口上
    // 应在最后一个GL操作完成时调用
    virtual bool Flush() = 0;
    
};

#define GLCheckError() IGLContext::CheckGLError(__FUNCTION__, __LINE__)


}

