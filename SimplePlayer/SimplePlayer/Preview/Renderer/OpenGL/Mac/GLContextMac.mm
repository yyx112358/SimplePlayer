//
//  GLContextMac.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "GLContextMac.hpp"
#include "SPLog.h"

using namespace sp;

extern std::shared_ptr<sp::IGLContext> CreateGLContextMac()
{
    return std::make_shared<GLContextMac>();
}

GLContextMac::~GLContextMac()
{
    std::lock_guard lock(_mutex);
    _context = nil;
}

bool GLContextMac::Init()
{
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

/// 切换到本Context
bool GLContextMac::SwitchContext()
{
    if (_context == nil && Init() == false)
        return false;
    std::lock_guard lock(_mutex);
    
    [_context makeCurrentContext];
    return true;
}

// 在OpenGL绘制完成后，调用flush方法将绘制的结果显示到窗口上
// 应在最后一个GL操作完成时调用
bool GLContextMac::Flush()
{
    if (_context == nil && Init() == false)
        return false;
    std::lock_guard lock(_mutex);
    
    [_context flushBuffer];
    return true;
}

NSOpenGLContext *GLContextMac::context()
{
    return _context;
}
