//
//  GLFrameBuffer.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "GLFrameBuffer.hpp"
#include "SPLog.h"

using namespace sp;


void GLFrameBuffer::FRAME_BUFFER_DELETER(GLuint *p)
{
    SPLOGD("Delete frame buffer %d", *p);
    glDeleteFramebuffers(1, p);
}

bool GLFrameBuffer::Activate()
{
    _context->SwitchContext();
    
    if (_frameBufferId == nullptr) {
        // 创建帧缓冲（Frame Buffer Object）
        GLuint frameBufferId;
        glGenFramebuffers(1, &frameBufferId);
        auto holder = GL_IdHolder(new GLuint(frameBufferId), FRAME_BUFFER_DELETER);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBufferId);
        if (GLCheckError())
            return false;
        
        assert(_attachTextures.size() > 0 || _attachRenderBuffers.size() > 0);
        assert(_attachTextures.size() + _attachRenderBuffers.size() <= 16);
        
        GLenum attachId = GL_COLOR_ATTACHMENT0;
        // Texture附着到FBO
        // 绝大多数情况下，只会用到_attachTextures[0]。除非是需要同时在多个Texture上绘制
        for (auto &tex : _attachTextures) {
            if (tex->Activate() == false)
                return false;
            
            glActiveTexture(attachId); // 激活纹理单元
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachId, GL_TEXTURE_2D, *tex->id(), 0);
            
            if (GLCheckError())
                return false;
            else
                attachId++;
        }
        // RenderBuffer附着到FBO
        // 不可作为被采样纹理。一般用于Depth/Stencil Buffer
        for (auto &rbo : _attachRenderBuffers) {
            if (rbo->Activate() == false)
                return false;
            
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachId, GL_RENDERBUFFER, *rbo->id());
            
            if (GLCheckError())
                return false;
            else
                attachId++;
        }
        
        
        if (GLenum ret = glCheckFramebufferStatus(GL_FRAMEBUFFER); ret != GL_FRAMEBUFFER_COMPLETE) {
            SPLOGE("Bind Frame Buffer %d failed, error:%d", frameBufferId, ret);
            return false;
        }
        
        if (GLCheckError())
            return false;
        
        _frameBufferId = std::move(holder);
    } else
        glBindFramebuffer(GL_FRAMEBUFFER, *_frameBufferId);
    
    return true;
}


std::optional<Frame> GLFrameBuffer::DownloadFrameBuffer(std::optional<GLenum> pixelFormat) const {
    std::optional<Frame> buffer;
    if (_attachTextures.size() == 0 || _attachTextures[0]->id().has_value() == false)
        return buffer;
    _context->SwitchContext();
    
    buffer = _attachTextures[0]->getBuffer();
    buffer->data = std::shared_ptr<uint8_t[]>(new uint8_t[buffer->width * buffer->height * 4]);
    
    // 从FrameBuffer下载数据
    glReadPixels(0, 0, (GLsizei)buffer->width, (GLsizei)buffer->height, pixelFormat.has_value() ? *pixelFormat : buffer->glFormat(), GL_UNSIGNED_BYTE, buffer->data.get());
    
    if (GLCheckError())
        return std::optional<Frame>();
    else
        return buffer;
}
