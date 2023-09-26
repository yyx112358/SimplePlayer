//
//  GLTexture.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "GLTexture.hpp"
#include "SPLog.h"

using namespace sp;

void GLTexture::TEXTURE_DELETER(GLuint id)
{
    SPLOGD("Delete texture %d", id);
    glDeleteTextures(1, &id);
}
   
void GLTexture::UploadBuffer(Frame buffer)
{
    _needUpdateAll = _buffer.has_value() == false || _buffer->equalExceptData(buffer) == false;
    _needUpdate = true;
    _buffer = buffer;
}

std::optional<Frame> GLTexture::DownloadTexture(std::optional<GLenum> pixelFormat) const
{
    // OpenGL ES不支持
    std::optional<Frame> buffer;
    if (_textureId.has_value() == false)
        return buffer;
    
    _context->SwitchContext();
    buffer = *_buffer;
    buffer->data = std::shared_ptr<uint8_t[]>(new uint8_t[buffer->width * buffer->height * 4]);
    
    // 用于从Texture下载数据，OpenGL ES不支持
    glGetTexImage(GL_TEXTURE_2D, 0, pixelFormat.has_value() ? *pixelFormat : buffer->glFormat(), GL_UNSIGNED_BYTE, buffer->data.get());
    
//    GLuint fbo;
//    glGenFramebuffers(1, &fbo);
//    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, texture, 0);
//    uint8_t *buffer = new uint8_t[w * h * 4];
//    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
//    int ret = TER_FAIL;
//    if (resolveFormat) {
//        ret = TEUtils::writeBMP2File2(pName, buffer, w, h, 4);
//    } else {
//        ret = TEUtils::writeBMP2File(pName, buffer, w, h, 4);
//    }
//    delete[] buffer;
//    glDeleteFramebuffers(1, &fbo);
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
    return _textureId.id();
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
    if (_textureId.has_value() == false) {
        GLuint textureId;
        glGenTextures(1, &textureId);
        auto holder = GL_IdHolder(textureId, TEXTURE_DELETER);
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
    
    glBindTexture(GL_TEXTURE_2D, *_textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, _buffer->glFormat(), _buffer->width, _buffer->height, 0, _buffer->glFormat(), GL_UNSIGNED_BYTE, _buffer->data.get()); // 上传纹理。如果_buffer->data为空，则生成空纹理
    // glGenerateMipmap(GL_TEXTURE_2D); // 如果需要生成mipmap的话

    _buffer->data.reset();// 释放内存
    
    if (GLCheckError())
        return false;
    
//    glBindTexture(GL_TEXTURE_2D, 0);
    _needUpdate = false;
    return true;
}
