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
    
public:
    /// 获取最大支持的纹理单元数，最少32
    static GLint GetMaxTextureUnits();
    /// 获取支持的最大顶点属性个数，最少16
    static GLint GetMaxVertexAttribs();
    
};

#define GLCheckError() IGLContext::CheckGLError(__FUNCTION__, __LINE__)


}

