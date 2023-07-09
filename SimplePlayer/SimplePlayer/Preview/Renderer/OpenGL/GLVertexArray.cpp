//
//  GLVertexArray.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "GLVertexArray.hpp"
#include "SPLog.h"

using namespace sp;

void GLVertexArray::VERTEX_ARRAY_DELETER(GLuint *p)
{
    SPLOGD("Delete vertex array %d", *p);
    glDeleteTextures(1, p);
}

/// 默认矩形Vertex Buffer
const std::vector<GLVertexArray::VertexBuffer> &GLVertexArray::DEFAULT_RECT_VERTEX_BUFFER()
{
    const static std::vector<GLVertexArray::VertexBuffer> buf = {
        {{-1.0, 1.0}, {0.0, 1.0},}, // 左上
        {{ 1.0, 1.0}, {1.0, 1.0},}, // 右上
        {{-1.0,-1.0}, {0.0, 0.0},}, // 左下
        {{ 1.0,-1.0}, {1.0, 0.0},}, // 右下
    };
    return buf;
}

/// 默认矩形Element Buffer
const std::vector<GLVertexArray::ElementBuffer> &GLVertexArray::DEFAULT_RECT_ELEMENT_BUFFER()
{
    const static std::vector<GLVertexArray::ElementBuffer> buf = {
        {0, 1, 2}, // 第一个三角形
        {1, 3, 2}, // 第二个三角形
    };
    return buf;
}


bool GLVertexArray::Activate()
{
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

bool GLVertexArray::Render()
{
    if (_elementBufferSize.has_value()) // 使用Element绘制三角形
        glDrawElements(GL_TRIANGLES, *_elementBufferSize, GL_UNSIGNED_INT, 0);
    else if (_vertexBufferSize.has_value()) // 使用Arrays绘制三角形
        glDrawArrays(GL_TRIANGLES, 0, *_vertexBufferSize);
    else
        return false;
    return true;
}
