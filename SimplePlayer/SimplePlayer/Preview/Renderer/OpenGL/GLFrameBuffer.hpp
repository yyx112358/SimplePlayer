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


namespace sp {


class GLFrameBuffer {
public:
    static void FRAME_BUFFER_DELETER(GLuint p);

public:
    GLFrameBuffer(std::shared_ptr<IGLContext> context) :_context(context) {}
    virtual ~GLFrameBuffer() {
        _context->SwitchContext();
    }
    
    void UpdateAttachTextures(const std::vector<std::shared_ptr<GLTexture>> &textures) {
        _attachTextures = textures;
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
    
    bool Activate();
    
    /// 下载FrameBuffer。Render后执行，否则结果未知
    std::optional<Frame> DownloadFrameBuffer(std::optional<GLenum> pixelFormat = {}) const;
    
protected:
    const std::shared_ptr<IGLContext> _context;
    GL_IdHolder _frameBufferId = GL_IdHolder(FRAME_BUFFER_DELETER);
    
    std::vector<std::shared_ptr<GLTexture>> _attachTextures;
    std::vector<std::shared_ptr<GLRenderBuffer>> _attachRenderBuffers;
};

}
