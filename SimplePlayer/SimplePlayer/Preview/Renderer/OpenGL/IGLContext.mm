//
//  IGLContext.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#include "IGLContext.hpp"

#include <mutex>
#include <typeindex>

#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#include "GLContextMac.hpp"
#include "SPLog.h"

using namespace sp;


#pragma mark -GLContext

std::shared_ptr<IGLContext> IGLContext::CreateGLContext() {
    return std::make_shared<GLContextMac>();
}

bool IGLContext::CheckGLError(const char *function, int line) {
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
        SPLog(ESPLogLevel::SP_LOG_DEBUG, function, line, error);
    }
    return errorCode != 0;
}


