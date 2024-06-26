//
//  GLRenderBuffer.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include "IGLContext.hpp"
#include <optional>

namespace sp {


/// Render Buffer Object (RBO)
/// 仅可写入的缓冲区，为离屏渲染到FBO优化
class GLRenderBuffer {
public:
    static void RENDER_BUFFER_DELETER(GLuint p);
    
public:
    GLRenderBuffer(std::shared_ptr<IGLContext>context) : _context(context) {}
    GLRenderBuffer(std::shared_ptr<IGLContext>context, GLsizei width, GLsizei height) : _context(context), _width(width), _height(height) {}
    virtual ~GLRenderBuffer() {
        _context->SwitchContext();
    }
    
    void setSize(GLsizei width, GLsizei height) {
        _width = width;
        _height = height;
    }
    
    bool Activate();
    
    std::optional<GLuint> id() const {
        return _renderBufferId.id();
    }
    
protected:
    const std::shared_ptr<IGLContext> _context;
    GL_IdHolder _renderBufferId = GL_IdHolder(RENDER_BUFFER_DELETER);
    
    std::optional<GLsizei> _width, _height;
};


}
