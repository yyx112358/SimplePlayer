//
//  GLTexture.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "GLTexture.hpp"
#include "SPLog.h"

using namespace sp;

void GLTexture::TEXTURE_DELETER(GLuint *p)
{
    SPLOGD("Delete texture %d", *p);
    glDeleteTextures(1, p);
}
   
void GLTexture::UploadBuffer(Frame buffer)
{
    _needUpdateAll = _buffer.has_value() == false || _buffer->equalExceptData(buffer);
    _needUpdate = true;
    _buffer = buffer;
}

std::optional<Frame> GLTexture::DownloadBuffer(std::optional<GLenum> pixelFormat) const
{
    std::optional<Frame> buffer;
    if (_textureId == nullptr)
        return buffer;
    
    _context->SwitchContext();
    buffer = *_buffer;
    buffer->data = std::shared_ptr<uint8_t[]>(new uint8_t[_buffer->width * _buffer->height * 4]);
    glReadPixels(0, 0, (GLsizei)_buffer->width, (GLsizei)_buffer->height, pixelFormat.has_value() ? *pixelFormat : _buffer->glFormat(), GL_UNSIGNED_BYTE, buffer->data.get());
    if (GLCheckError())
        return std::optional<Frame>();
    else
        return buffer;
}

bool GLTexture::Activate()
{
    _context->SwitchContext();
    
    if (_UploadBuffer() == false)
        return false;
    glBindTexture(GL_TEXTURE_2D, *_textureId);
    return true;
}

std::optional<GLuint> GLTexture::id() const
{
    return _textureId != nullptr ? std::make_optional<GLuint>(*_textureId) : std::make_optional<GLuint>();
}

bool GLTexture::_UploadBuffer()
{
    if (_needUpdate == false)
        return true;
    
    // 清除texture
    if (_buffer.has_value() == false) {
        _textureId.reset();
        return false;
    }
    
    // 需要全部更新
    if (_needUpdateAll == true)
        _textureId.reset();
    
    // 创建Texture
    if (_textureId == nullptr) {
        GLuint textureId;
        glGenTextures(1, &textureId);
        auto holder = GL_IdHolder(new GLuint(textureId), TEXTURE_DELETER);
        SPLOGD("Create texture %d", textureId);
        
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        // warp参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _textureWrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _textureWrapT);
        // 插值filter参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _textureMinFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _textureMagFilter);
        
        if (GLCheckError())
            return false;
        
        _textureId = std::move(holder);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, _buffer->glFormat(), _buffer->width, _buffer->height, 0, _buffer->glFormat(), GL_UNSIGNED_BYTE, _buffer->data.get()); // 上传纹理。如果_buffer->data为空，则生成空纹理
    // glGenerateMipmap(GL_TEXTURE_2D); // 如果需要生成mipmap的话
    _buffer->data.reset();// 释放内存
    
    if (GLCheckError())
        return false;
    
//    glBindTexture(GL_TEXTURE_2D, 0);
    _needUpdate = false;
    return true;
}
