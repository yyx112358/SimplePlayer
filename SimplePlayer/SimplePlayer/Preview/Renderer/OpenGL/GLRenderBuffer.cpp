//
//  GLRenderBuffer.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "GLRenderBuffer.hpp"

#include "SPLog.h"

using namespace sp;


void GLRenderBuffer::RENDER_BUFFER_DELETER(GLuint p)
{
    SPLOGD("Delete render buffer %d", p);
    glDeleteRenderbuffers(1, &p);
}

bool GLRenderBuffer::Activate()
{
    _context->SwitchContext();
    
    if (_renderBufferId.has_value() == false) {
        if (_width.has_value() == false || _height.has_value() == false)
            return false;
        
        GLuint renderBufferId;
        glGenRenderbuffers(1, &renderBufferId);
        auto holder = GL_IdHolder(renderBufferId, RENDER_BUFFER_DELETER);
        SPLOGD("Create render buffer %d", renderBufferId);
        
        glBindRenderbuffer(GL_RENDERBUFFER, renderBufferId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, *_width, *_height);
        
        if (GLCheckError())
            return false;
        
        _renderBufferId = std::move(holder);
    }
    glBindRenderbuffer(GL_RENDERBUFFER, *_renderBufferId);
    
    if (GLCheckError())
        return false;
    
    // glBindRenderbuffer( GL_RENDERBUFFER, 0 );
    return true;
}
