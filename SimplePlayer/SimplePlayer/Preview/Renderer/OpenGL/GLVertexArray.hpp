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
#include "SPLog.h"

namespace sp {


class GLVertexArray {
public:
    static void VERTEX_ARRAY_DELETER(GLuint *p) {
        SPLOGD("Delete vertex array %d", *p);
        glDeleteTextures(1, p);
    }
    
    typedef struct {
        std::array<GLfloat, 2>location;
        std::array<GLfloat, 2>texture;
    } VertexBuffer;
    
    /// 默认矩形Vertex Buffer
    static const std::vector<VertexBuffer> &DEFAULT_RECT_VERTEX_BUFFER() {
        const static std::vector<VertexBuffer> buf = {
            {{-0.95, 0.95}, {0.0, 1.0},}, // 左上
            {{ 0.95, 0.95}, {1.0, 1.0},}, // 右上
            {{-0.95,-0.95}, {0.0, 0.0},}, // 左下
            {{ 0.95,-0.95}, {1.0, 0.0},}, // 右下
        };
        return buf;
    }
    
    typedef std::array<GLuint, 3> ElementBuffer;
    /// 默认矩形Element Buffer
    static const std::vector<ElementBuffer> &DEFAULT_RECT_ELEMENT_BUFFER() {
        const static std::vector<ElementBuffer> buf = {
            {0, 1, 2}, // 第一个三角形
            {1, 3, 2}, // 第二个三角形
        };
        return buf;
    }
    
public:
    GLVertexArray(std::shared_ptr<IGLContext>context) : _context(context) {}
    virtual ~GLVertexArray() {
        _context->switchContext();
    }
    
    void UpdateVertexBuffer(const std::vector<VertexBuffer> &vbo) {
        _vertexBuffer = vbo;
    }
    
    void UpdateElementBuffer(const std::vector<ElementBuffer> &ebo) {
        _elementBuffer = ebo;
    }
    
    virtual bool Activate() {
        // 创建Vertex Array Object(VAO)。后续所有顶点操作都会储存到VAO中。OpenGL core模式下VAO必须要有。
        if (_vertexArrayId == nullptr) {
            GLuint vertexArrayId;
            glGenVertexArrays(1, &vertexArrayId); // 生成顶点Array对象。【必须在创建Buffer前】
            _vertexArrayId.reset(new GLuint(vertexArrayId));
            SPLOGD("Create vertex array %d", *_vertexArrayId);
        }
        glBindVertexArray(*_vertexArrayId); // 绑定顶点Array
        
        // Vertex Buffer Object(VBO)
        if (_vertexBuffer.size() > 0) {
            GLuint vertexBufId;
            
            glGenBuffers(1, &vertexBufId); // 生成 1 个顶点缓冲区对象，vertexBufId是绑定的唯一OpenGL标识
            glBindBuffer(GL_ARRAY_BUFFER, vertexBufId); // 绑定为GL_ARRAY_BUFFER
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(_vertexBuffer.size() * sizeof(_vertexBuffer[0])), _vertexBuffer.data(), GL_STATIC_DRAW); // 传输数据
            
            glVertexAttribPointer(0, (GLsizei)_vertexBuffer[0].location.size(), GL_FLOAT, GL_FALSE, sizeof(_vertexBuffer[0]), (GLvoid *)(offsetof(VertexBuffer, location)));// 位置
            glEnableVertexAttribArray(0); // 启用VertexAttribArray
            glVertexAttribPointer(1, (GLsizei)_vertexBuffer[0].texture.size(), GL_FLOAT, GL_FALSE, sizeof(_vertexBuffer[0]), (GLvoid *)(offsetof(VertexBuffer, texture)));// 纹理
            glEnableVertexAttribArray(1);
            
            if (GLCheckError())
                return false;
            _vertexBufferId.emplace(vertexBufId);
            _vertexBufferSize.emplace(_vertexBuffer.size());
            _vertexBuffer.clear();
        }
        
        // Element Buffer Object(EBO)
        if (_elementBuffer.size() > 0) {
            GLuint elementBufId;
            
            glGenBuffers(1, &elementBufId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufId); // 绑定为GL_ELEMENT_ARRAY_BUFFER
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, _elementBuffer.size() * sizeof(_elementBuffer[0]), _elementBuffer.data(), GL_STATIC_DRAW);
            
            if (GLCheckError())
                return false;
            _elementBufferId.emplace(elementBufId);
            _elementBufferSize.emplace(_elementBuffer.size() * _elementBuffer[0].size());
            _elementBuffer.clear();
        }
        
        return true;
    }
    
    virtual bool Render() {
        if (_elementBufferSize.has_value()) // 使用Element绘制三角形
            glDrawElements(GL_TRIANGLES, *_elementBufferSize, GL_UNSIGNED_INT, 0);
        else if (_vertexBufferSize.has_value()) // 使用Arrays绘制三角形
            glDrawArrays(GL_TRIANGLES, 0, *_vertexBufferSize);
        else
            return false;
        return true;
    }
    
protected:
    const std::shared_ptr<IGLContext> _context;
    GL_IdHolder _vertexArrayId = GL_IdHolder(nullptr, VERTEX_ARRAY_DELETER);
    
    // Vertex Buffer Object(VBO)
    std::vector<VertexBuffer> _vertexBuffer = DEFAULT_RECT_VERTEX_BUFFER();
    std::optional<GLuint> _vertexBufferId;
    std::optional<GLuint> _vertexBufferSize;
    
    // Element Buffer Object(EBO)
    std::vector<ElementBuffer> _elementBuffer = DEFAULT_RECT_ELEMENT_BUFFER();
    std::optional<GLuint> _elementBufferId;
    std::optional<GLuint> _elementBufferSize;
};

}
