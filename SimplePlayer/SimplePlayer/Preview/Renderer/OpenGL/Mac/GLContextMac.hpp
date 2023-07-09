//
//  GLContextMac.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include "IGLContext.hpp"

#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#include <mutex>

namespace sp {

class GLContextMac : public IGLContext
{
public:
    virtual ~GLContextMac();

    bool Init() override;
    bool SwitchContext() override;
    bool Flush() override;
    
    NSOpenGLContext *context();

protected:
    NSOpenGLContext *_context;
    std::mutex _mutex;
};


}
