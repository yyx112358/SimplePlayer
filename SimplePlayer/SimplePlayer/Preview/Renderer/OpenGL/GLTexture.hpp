//
//  GLTexture.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include <optional>

#include "IGLContext.hpp"
#include "Frame.hpp"

namespace sp {


class GLTexture {
public:
    static void TEXTURE_DELETER(GLuint *p);
    
public:
    GLTexture(std::shared_ptr<IGLContext>context) : _context(context) {}
    GLTexture(std::shared_ptr<IGLContext>context, Frame buffer) : _context(context), _buffer(std::move(buffer)) {}
    GLTexture(std::shared_ptr<IGLContext>context, GLsizei width, GLsizei height) : _context(context), _buffer(Frame{.width = width, .height = height, .pixelFormat = AV_PIX_FMT_RGBA}) {}
    
    virtual ~GLTexture() {
        _context->SwitchContext();
        
        _textureId.reset();
        _buffer.reset();
    }
    
    /// 上传Buffer，不阻塞。Activate时才真正上传
    void UploadBuffer(Frame buffer);
    
    /// 下载Buffer
    std::optional<Frame> DownloadBuffer(std::optional<GLenum> pixelFormat = {}) const;
    
    bool Activate();
    
    std::optional<GLuint> id() const;
    GLsizei width() const { return _buffer ? _buffer->width : -1; }
    GLsizei height() const { return _buffer ? _buffer->height : -1; }
    
protected:
    virtual bool _UploadBuffer();
    
protected:
    const std::shared_ptr<IGLContext> _context;
    GL_IdHolder _textureId = GL_IdHolder(nullptr, TEXTURE_DELETER);
    bool _needUpdate = true;
    
    std::optional<Frame> _buffer;
    GLenum _textureWrapS = GL_CLAMP_TO_EDGE, _textureWrapT = GL_CLAMP_TO_EDGE;
    GLenum _textureMinFilter = GL_NEAREST, _textureMagFilter = GL_LINEAR;
};


}
