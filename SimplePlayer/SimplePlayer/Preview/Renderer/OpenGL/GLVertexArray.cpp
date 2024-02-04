//
//  GLVertexArray.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "GLVertexArray.hpp"
#include "SPLog.h"
#include "IGLContext.hpp"

#include <unordered_map>

using namespace sp;

void GLVertexArray::VERTEX_ARRAY_DELETER(GLuint p)
{
    SPLOGD("Delete vertex array %d", p);
    glDeleteVertexArrays(1, &p);
}

void GLVertexArray::VERTEX_BUFFER_DELETER(GLuint p) {
    SPLOGD("Delete vertex buffer %d", p);
    glDeleteBuffers(1, &p);
}

/// 默认矩形Vertex Buffer
const std::vector<GLVertexArray::VertexBuffer> &GLVertexArray::DEFAULT_RECT_VERTEX_BUFFER()
{
    const static std::vector<GLVertexArray::VertexBuffer> buf = {
        {{-1.0, 1.0}, {0.0, 0.0},}, // 左上
        {{ 1.0, 1.0}, {1.0, 0.0},}, // 右上
        {{-1.0,-1.0}, {0.0, 1.0},}, // 左下
        {{ 1.0,-1.0}, {1.0, 1.0},}, // 右下
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
    if (_vertexArrayId.has_value() == false
        || _vertexBuffer.size() > 0 || _elementBuffer.size() > 0) {
        GLuint vertexArrayId;
        glGenVertexArrays(1, &vertexArrayId); // 生成顶点Array对象。【必须在创建Buffer前】
        _vertexArrayId.reset(vertexArrayId);
        SPLOGD("Create vertex array %d", *_vertexArrayId);
    }
    glBindVertexArray(*_vertexArrayId); // 绑定顶点Array
    
    // Vertex Buffer Object(VBO)
    if (_vertexBufferEx.has_value()) {
        SPASSERT1(_vertexBufferEx.has_value() && _vertexBuffer.size() == 0, "_vertexBuffer和_vertexBufferEx同时非空，检查是否需要重置");
        SPASSERT0(_vertexBufferEx->buf != nullptr);
        SPASSERT0(_vertexBufferEx->vertexNum > 0);
        SPASSERT0(0 < _vertexBufferEx->descs.size() && _vertexBufferEx->descs.size() <= IGLContext::GetMaxVertexAttribs());

        // 创建VBO并传输数据
        GLuint vertexBufId;
        glGenBuffers(1, &vertexBufId); // 生成 1 个顶点缓冲区对象，vertexBufId是绑定的唯一OpenGL标识
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufId); // 绑定为GL_ARRAY_BUFFER
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(_vertexBufferEx->sizeInByte()), _vertexBufferEx->buf.get(), GL_STATIC_DRAW); // 传输数据
        if (GLCheckError())
            return false;
        
        // 配置Attribute属性
        GLsizei offset = 0;
        for (int i = 0; i < _vertexBufferEx->descs.size(); i++) {
            std::type_index typeIndex = _vertexBufferEx->descs[i].first;
            GLint num = static_cast<GLint>(_vertexBufferEx->descs[i].second);
            
            GLint size = static_cast<GLint>(_vertexBufferEx->attribSizeInByte(i));
            GLenum type = _vertexBufferEx->typeToGLenum(typeIndex);
            GLsizei stride = static_cast<GLsizei>(_vertexBufferEx->vertexSizeInByte());
            const GLvoid *pointer = reinterpret_cast<GLvoid *>(offset);
            
            glVertexAttribPointer(i, num, type, GL_FALSE, stride, pointer);
            glEnableVertexAttribArray(i); // 启用VertexAttribArray
            
            offset += size;
            
            if (GLCheckError())
                return false;
        }
        
        // 记录参数
        _vertexBufferId.reset(vertexBufId);
        _vertexBufferSize.emplace(_vertexBufferEx->vertexNum);
        _vertexBufferEx.reset();
    } else if (_vertexBuffer.size() > 0) {
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
        _vertexBufferId.reset(vertexBufId);
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
        _elementBufferId.reset(elementBufId);
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

GLenum GLVertexArray::VertexBufferEx::typeToGLenum(std::type_index typeIndex) {
    static std::unordered_map<std::type_index, GLenum> tbl = {
        {std::type_index(typeid(GLfloat)), GL_FLOAT},
        {std::type_index(typeid(GLint)), GL_INT},
        {std::type_index(typeid(GLbyte)), GL_BYTE},
    };
    return tbl.at(typeIndex);
}

template<typename Tp>
std::pair<std::type_index, size_t> _SizeHelper() {
    return {std::type_index(typeid(Tp)), sizeof(Tp)};
}

size_t GLVertexArray::VertexBufferEx::typeToSize(std::type_index typeIndex) {
    static std::unordered_map<std::type_index, size_t> tbl = {
        _SizeHelper<GLfloat>(),
        _SizeHelper<GLint>(),
        _SizeHelper<GLbyte>(),
    };
    return tbl.at(typeIndex);
}


size_t GLVertexArray::VertexBufferEx::attribSizeInByte(int index) const {
    if (0 <= index && index < descs.size()) {
        return typeToSize((descs[index].first)) * descs[index].second;
    } else {
        size_t num = 0;
        for (auto i = 0; i < descs.size(); i++)
            num += typeToSize(descs[i].first) * descs[i].second;
        return num;
    }
}

size_t GLVertexArray::VertexBufferEx::vertexSizeInByte() const {
    return attribSizeInByte(-1);
}

size_t GLVertexArray::VertexBufferEx::sizeInByte() const {
    return vertexNum * vertexSizeInByte();
}


