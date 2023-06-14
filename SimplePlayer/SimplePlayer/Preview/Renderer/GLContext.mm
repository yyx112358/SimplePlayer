//
//  GLContext.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#include "GLContext.hpp"

#import <OpenGL/gl3.h>

using namespace sp;

GLContext::~GLContext() {
    std::lock_guard lock(_mutex);
    _context = nil;
}

bool GLContext::init() {
    std::lock_guard lock(_mutex);

    NSOpenGLPixelFormatAttribute attrs[] =
    {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,  // 【声明使用OpenGL3.2】，不配置则默认OpenGL 2
        0
    };
    
    NSOpenGLPixelFormat * pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    _context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
    
    return _context != nil;
}

NSOpenGLContext *GLContext::context() {
    return _context;
}

/// 切换到本Context
bool GLContext::switchContext() {
    if (_context == nil && init() == false)
        return false;
    std::lock_guard lock(_mutex);
    
    [_context makeCurrentContext];
    return true;
}

// 在OpenGL绘制完成后，调用flush方法将绘制的结果显示到窗口上
// 应在最后一个GL操作完成时调用
bool GLContext::flush() {
    if (_context == nil && init() == false)
        return false;
    std::lock_guard lock(_mutex);
    
    [_context flushBuffer];
    return true;
}

bool GLContext::CheckGLError(const char *file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        const char *error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
//            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
//            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
            default: assert(0);break;
        }
        NSLog(@"%s | %s[%d]", error, file, line);
    }
    return errorCode != 0;
}
