//
//  IGLContext.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#include "IGLContext.hpp"

#include "SPLog.h"


#if __APPLE__ && !defined(TARGET_OS_IPHONE)

#define GL_SILENCE_DEPRECATION
#import <OpenGL/gl3.h>
#include "GLContextMac.hpp"

#endif // __APPLE__ && !TARGET_OS_IPHONE

using namespace sp;


#pragma mark -GLContext

std::shared_ptr<IGLContext> IGLContext::CreateGLContext()
{
    return CreateGLContextMac();
}

bool IGLContext::CheckGLError(const char *function, int line)
{
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
            default: SPASSERT_NOT_IMPL;break;
        }
        SPLog(ESPLogLevel::SP_LOG_DEBUG, function, line, error);
    }
    return errorCode != 0;
}

GLint IGLContext::GetMaxTextureUnits() {
    GLint value = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &value);
    return value;
}

GLint IGLContext::GetMaxVertexAttribs() {
    GLint value = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &value);
    return value;
}

