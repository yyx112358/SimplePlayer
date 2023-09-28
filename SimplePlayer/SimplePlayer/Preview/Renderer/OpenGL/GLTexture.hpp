//
//  GLTexture.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include <optional>

#include "IGLContext.hpp"
#include "VideoFrameBase.hpp"

namespace sp {


class GLTexture {
public:
    static void TEXTURE_DELETER(GLuint id);
    
public:
    GLTexture(std::shared_ptr<IGLContext>context) : _context(context) {}
    GLTexture(std::shared_ptr<IGLContext>context, VideoFrame buffer) : _context(context), _buffer(std::move(buffer)) {}
    GLTexture(std::shared_ptr<IGLContext>context, GLsizei width, GLsizei height) : _context(context), _buffer(VideoFrame{.width = width, .height = height, .pixelFormat = AV_PIX_FMT_RGBA}) {}
    
    virtual ~GLTexture() {
        _context->SwitchContext();
        
        _textureId.reset();
        _buffer.reset();
    }
    
    /// 上传Buffer，不阻塞。Activate时才真正上传
    void UploadBuffer(VideoFrame buffer);
    
    /// 下载Buffer，OpenGL ES不支持
    static std::optional<VideoFrame> DownloadTexture(std::shared_ptr<GLTexture>texture, std::optional<GLenum> pixelFormat = {});
    
    bool Activate();
    
    std::optional<GLuint> id() const;
    GLsizei width() const { return _buffer ? _buffer->width : -1; }
    GLsizei height() const { return _buffer ? _buffer->height : -1; }
    
    
    std::optional<const VideoFrame> getBuffer() const { return _buffer; }
    
protected:
    virtual bool _UploadBuffer();
    
protected:
    const std::shared_ptr<IGLContext> _context;
    GL_IdHolder _textureId = GL_IdHolder(TEXTURE_DELETER);
    bool _needUpdate = true;        // 需要执行更新
    bool _needUpdateAll = false;    // 需要重建Texture
    
    std::optional<VideoFrame> _buffer;
    GLenum _textureWrapS = GL_CLAMP_TO_EDGE, _textureWrapT = GL_CLAMP_TO_EDGE;
    GLenum _textureMinFilter = GL_NEAREST, _textureMagFilter = GL_LINEAR;
};


}
