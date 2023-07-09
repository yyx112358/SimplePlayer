//
//  GLFrameBuffer.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include <vector>

#include "IGLContext.hpp"
#include "GLTexture.hpp"
#include "GLRenderBuffer.hpp"

#include "SPLog.h"

namespace sp {


class GLFrameBuffer {
public:
    static void FRAME_BUFFER_DELETER(GLuint *p) {
        SPLOGD("Delete frame buffer %d", *p);
        glDeleteFramebuffers(1, p);
    }
public:
    GLFrameBuffer(std::shared_ptr<IGLContext> context) :_context(context) {}
    virtual ~GLFrameBuffer() {
        _context->switchContext();
    }
    
    void UpdateAttachTextures(const std::vector<std::shared_ptr<GLTexture>> &textures) {
        _attachTextures = textures;
        
        // 纹理单元个数有限，需要检查
        GLint MAX_TEXTURE_UNIT;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &MAX_TEXTURE_UNIT);
        assert(_attachTextures.size() + _attachRenderBuffers.size() <= MAX_TEXTURE_UNIT);
    }
    
    const std::vector<std::shared_ptr<GLTexture>> &GetAttachTextures() const {
        return _attachTextures;
    }
    
    const std::shared_ptr<GLTexture> GetOutputTexture() const {
        return _attachTextures.size() > 0 ? _attachTextures[0] : nullptr;
    }
    
    void UpdateAttachRenderBuffers(const std::vector<std::shared_ptr<GLRenderBuffer>> &rbos) {
        _attachRenderBuffers = rbos;
    }
    
    const std::vector<std::shared_ptr<GLRenderBuffer>> &GetAttachRenderBuffers() const {
        return _attachRenderBuffers;
    }
    
    bool Activate() {
        _context->switchContext();
        
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
        }
        
        return true;
    }
    
protected:
    const std::shared_ptr<IGLContext> _context;
    GL_IdHolder _frameBufferId = GL_IdHolder(nullptr, FRAME_BUFFER_DELETER);
    
    std::vector<std::shared_ptr<GLTexture>> _attachTextures;
    std::vector<std::shared_ptr<GLRenderBuffer>> _attachRenderBuffers;
};

}
