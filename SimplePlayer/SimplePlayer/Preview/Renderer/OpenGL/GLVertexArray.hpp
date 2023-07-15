//
//  GLVertexArray.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include <vector>
#include <array>
#include <optional>

#include "IGLContext.hpp"

namespace sp {


class GLVertexArray {
public:
    typedef struct {
        // TODO: 灵活性太差，考虑别的设计
        std::array<GLfloat, 2>location;
        std::array<GLfloat, 2>texture;
    } VertexBuffer;
    
    typedef std::array<GLuint, 3> ElementBuffer;

public:
    static void VERTEX_ARRAY_DELETER(GLuint *p);
    static void VERTEX_BUFFER_DELETER(GLuint *p);
    
    /// 默认矩形Vertex Buffer
    static const std::vector<VertexBuffer> &DEFAULT_RECT_VERTEX_BUFFER();
    
    /// 默认矩形Element Buffer
    static const std::vector<ElementBuffer> &DEFAULT_RECT_ELEMENT_BUFFER();
    
public:
    GLVertexArray(std::shared_ptr<IGLContext>context) : _context(context) {}
    virtual ~GLVertexArray() {
        _context->SwitchContext();
    }
    
    void UpdateVertexBuffer(const std::vector<VertexBuffer> &vbo) {
        _vertexBuffer = vbo;
    }
    
    void UpdateElementBuffer(const std::vector<ElementBuffer> &ebo) {
        _elementBuffer = ebo;
    }
    
    virtual bool Activate();
    
    virtual bool Render();
    
protected:
    const std::shared_ptr<IGLContext> _context;
    GL_IdHolder _vertexArrayId = GL_IdHolder(nullptr, VERTEX_ARRAY_DELETER);
    
    // Vertex Buffer Object(VBO)
    std::vector<VertexBuffer> _vertexBuffer = DEFAULT_RECT_VERTEX_BUFFER();
    GL_IdHolder _vertexBufferId = GL_IdHolder(nullptr, VERTEX_BUFFER_DELETER);
    std::optional<GLuint> _vertexBufferSize;
    
    // Element Buffer Object(EBO)
    std::vector<ElementBuffer> _elementBuffer = DEFAULT_RECT_ELEMENT_BUFFER();
    GL_IdHolder _elementBufferId = GL_IdHolder(nullptr, VERTEX_BUFFER_DELETER);
    std::optional<GLuint> _elementBufferSize;
};

}
