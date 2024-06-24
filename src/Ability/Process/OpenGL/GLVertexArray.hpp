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
#include <typeindex>

#include "IGLContext.hpp"

namespace sp {


class GLVertexArray {
public:
    /// 标准VertexBuffer类型，满足大多数场景
    typedef struct {
        std::array<GLfloat, 3>location;
        std::array<GLfloat, 2>texture;
    } VertexBuffer;
    
    /// 扩展VertexBuffer，支持其它场景
    class VertexBufferEx {
    public:
        std::unique_ptr<char []> buf;
        size_t vertexNum = 0;
        std::vector<std::pair<std::type_index, size_t>> descs;
        
        template<typename Tp>
        friend VertexBufferEx toBuffer(const std::vector<Tp> &src);
        
        static GLenum typeToGLenum(std::type_index typeIndex);
        static size_t typeToSize(std::type_index typeIndex);
        
        size_t attribSizeInByte(int index = -1) const;
        size_t vertexSizeInByte() const;
        size_t sizeInByte() const;
    };
    
    typedef std::array<GLuint, 3> ElementBuffer;

public:
    static void VERTEX_ARRAY_DELETER(GLuint p);
    static void VERTEX_BUFFER_DELETER(GLuint p);
    
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
    
    void UpdateVertexBufferEx(VertexBufferEx &&vboEx) {
        _vertexBufferEx = std::move(vboEx);
        _vertexBuffer.clear();
    }
    
    void UpdateElementBuffer(const std::vector<ElementBuffer> &ebo) {
        _elementBuffer = ebo;
    }
    
    virtual bool Activate();
    
    virtual bool Render();
    
protected:
    const std::shared_ptr<IGLContext> _context;
    GL_IdHolder _vertexArrayId = GL_IdHolder(VERTEX_ARRAY_DELETER);
    
    // Vertex Buffer Object(VBO)
    std::vector<VertexBuffer> _vertexBuffer = DEFAULT_RECT_VERTEX_BUFFER();
    std::optional<VertexBufferEx> _vertexBufferEx; // 优先使用Ex
    GL_IdHolder _vertexBufferId = GL_IdHolder(VERTEX_BUFFER_DELETER);
    std::optional<GLuint> _vertexBufferSize;
    
    // Element Buffer Object(EBO)
    std::vector<ElementBuffer> _elementBuffer = DEFAULT_RECT_ELEMENT_BUFFER();
    GL_IdHolder _elementBufferId = GL_IdHolder(VERTEX_BUFFER_DELETER);
    std::optional<GLuint> _elementBufferSize;
};

}
