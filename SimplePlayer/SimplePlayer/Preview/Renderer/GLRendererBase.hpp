//
//  GLRendererBase.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#ifndef GLRendererBase_hpp
#define GLRendererBase_hpp

#import <Foundation/Foundation.h>
#include <cstddef>
#include <string>
#include <optional>
#include <array>
#include <any>


#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#import "../../../../thirdParty/glm/glm/glm.hpp"
#include "../../../../thirdParty/glm/glm/gtc/matrix_transform.hpp"
#include "../../../../thirdParty/glm/glm/gtc/type_ptr.hpp"

#import "GLContext.hpp"

namespace sp {

class GLVertexArray {
public:
    static void VERTEX_ARRAY_DELETER(GLuint *p) {
        NSLog(@"Delete vertex array %d", *p);
        glDeleteTextures(1, p);
    }
    
    typedef struct {
        std::array<GLfloat, 2>location;
        std::array<GLfloat, 2>texture;
    } VertexBuffer;
    
    /// 默认矩形Vertex Buffer
    static const std::vector<VertexBuffer> &DEFAULT_RECT_VERTEX_BUFFER() {
        const static std::vector<VertexBuffer> buf = {
            {{-1.0, 1.0}, {0.0, 1.0},}, // 左上
            {{ 1.0, 1.0}, {1.0, 1.0},}, // 右上
            {{-1.0,-1.0}, {0.0, 0.0},}, // 左下
            {{ 1.0,-1.0}, {1.0, 0.0},}, // 右下
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
    GLVertexArray(std::shared_ptr<GLContext>context) : _context(context) {}
//    GLTexture(GLTexture &&other) : _context(other._context), _textureId(std::move(other._textureId)), _buffer(other._buffer), _textureWrapS(other._textureWrapS), _textureWrapT(other._textureWrapT), _textureMinFilter(other._textureMinFilter), _textureMagFilter(other._textureMagFilter) {}
    
    virtual ~GLVertexArray() {
        _context->switchContext();
        
        _vertexArrayId.reset();
        _vertexBuffer.clear();
        _elementBuffer.clear();
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
            NSLog(@"Create vertex array %d", *_vertexArrayId);
        }
        glBindVertexArray(*_vertexArrayId); // 绑定顶点Array
        
        // Vertex Buffer Object(VBO)
        if (_vertexBuffer.size() > 0) {
            GLuint vertexBufId;
            GLsizei vertexBufferSize = static_cast<GLsizei>(_vertexBuffer.size() * sizeof(_vertexBuffer[0]));
            
            glGenBuffers(1, &vertexBufId); // 生成 1 个顶点缓冲区对象，vertexBufId是绑定的唯一OpenGL标识
            glBindBuffer(GL_ARRAY_BUFFER, vertexBufId); // 绑定为GL_ARRAY_BUFFER
            glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, _vertexBuffer.data(), GL_STATIC_DRAW); // 传输数据

            glVertexAttribPointer(0, (GLsizei)_vertexBuffer[0].location.size(), GL_FLOAT, GL_FALSE, sizeof(_vertexBuffer[0]), (GLvoid *)(offsetof(VertexBuffer, location)));// 位置
            glEnableVertexAttribArray(0); // 启用VertexAttribArray
            glVertexAttribPointer(1, (GLsizei)_vertexBuffer[0].texture.size(), GL_FLOAT, GL_FALSE, sizeof(_vertexBuffer[0]), (GLvoid *)(offsetof(VertexBuffer, texture)));// 纹理
            glEnableVertexAttribArray(1);
            
            if (CheckError())
                return false;
            _vertexBufferId.emplace(vertexBufId);
            _vertexBufferSize.emplace(vertexBufferSize);
            _vertexBuffer.clear();
        }
        
        // Element Buffer Object(EBO)
        if (_elementBuffer.size() > 0) {
            GLuint elementBufId;
            GLsizei elementBufferSize = static_cast<GLsizei>(_elementBuffer.size() * sizeof(_elementBuffer[0]));
            
            glGenBuffers(1, &elementBufId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufId); // 绑定为GL_ELEMENT_ARRAY_BUFFER
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementBufferSize, _elementBuffer.data(), GL_STATIC_DRAW);
            
            if (CheckError())
                return false;
            _elementBufferId.emplace(elementBufId);
            _elementBufferSize.emplace(elementBufferSize);
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
    const std::shared_ptr<GLContext> _context;
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


class GLRendererBase {
public:
    static void VERTEX_ARRAY_DELETER(GLuint *p) {
        NSLog(@"Delete vertex array %d", *p);
        glDeleteVertexArrays(1, p);
    }
    static void FRAME_BUFFER_DELETER(GLuint *p) {
        NSLog(@"Delete frame buffer %d", *p);
        glDeleteFramebuffers(1, p);
    }
public:
    GLRendererBase(std::shared_ptr<GLContext> context) :_context(context), _program(new GLProgram(context)), _screenProgram(new GLProgram(context)) {}
    
    virtual ~GLRendererBase() {
        _context->switchContext();
        
        _program.reset();
//        _vertexArrayId.reset();
    }
    
    bool UpdateShader(const std::vector<std::string> &vertexShader, const std::vector<std::string> &fragmentShader) {
        _program->UpdateShader(vertexShader, fragmentShader);
        return true;
    }
    
    bool UpdateTexture(const std::vector<ImageBuffer> &buffers) {
        GLint MAX_TEXTURE_UNIT;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &MAX_TEXTURE_UNIT);
        assert(buffers.size() <= MAX_TEXTURE_UNIT);
        
        for (auto &buffer : buffers) {
            GLTexture texture(_context);
            texture.UploadBuffer(buffer);
            _textures.emplace_back(std::move(texture));
        }
        _needUpdate = true;
        return true;
    }
    
    void SetClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
        _clearColor[0] = r;
        _clearColor[1] = g;
        _clearColor[2] = b;
        _clearColor[3] = a;
    }
    
    void SetEnableBlend(bool enable) {
        if (enable)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
    }
    
    virtual bool Render() {
        // 获取当前OpenGL上下文
        if (_context == nullptr || _context->switchContext() == false)
            return false;
        // 更新内部参数
        if (_InternalUpdate() == false)
            return false;

        // 上屏绘制
        glBindFramebuffer(GL_FRAMEBUFFER, *_frameBufferId);
        glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        
//        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 取消注释后将启用线框模式
        _program->Activate(); // 启用Shader程序
        CheckError();
        
        _vertexArray.Activate();
        for (int i = 0; i < _textures.size(); i++) {
            assert(_textures[i].id().has_value());
            
            glActiveTexture(GL_TEXTURE0 + i); // 激活纹理单元1
            glBindTexture(GL_TEXTURE_2D, *_textures[i].id()); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
            
            char textureName[] = "texture00";
            snprintf(textureName, sizeof(textureName), "texture%d", i);
            _program->UpdateUniform(textureName, i); // 更新纹理uniform
        }
        
        CheckError();
        
        glm::mat4 trans(1.0f);
//        static int angle = 0;
//        trans = glm::translate(trans, glm::vec3(0.5f, 0.5f, 0.0f));
//        trans = glm::rotate(trans, glm::radians(float(angle++)), glm::vec3(0.0f, 0.0f, 1.0f));
//        trans = glm::scale(trans, glm::vec3(0.75f, 0.75f, 0.75f));
        _program->UpdateUniform("transform", trans);
        
        _program->FlushUniform();
        
        CheckError();
        _vertexArray.Render();
        
        CheckError();
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        _screenProgram->Activate();
        _screenVertexArray.Activate();
        glActiveTexture(GL_TEXTURE0 + 1); // 激活纹理单元1
        glBindTexture(GL_TEXTURE_2D, *_frameBufferTextureId); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
        _screenProgram->UpdateUniform("screenTexture", 1); // 更新纹理uniform
        _screenProgram->FlushUniform();

        _screenVertexArray.Render();
        
        if (CheckError())
            return false;
        
        return true;
    }
    
protected:
    virtual bool _InternalUpdate() {
        if (_needUpdate == false)
            return true;
        
        _screenProgram->UpdateShader({R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoords = aTexCoords;
}
)"}, {R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    FragColor = texture(screenTexture, TexCoords);
}
)"});
        _screenProgram->Activate();
        _screenVertexArray.Activate();
        
        _program->Activate();
        
        // 创建帧缓冲（Frame Buffer Object）
        GLuint frameBufferId;
        glGenFramebuffers(1, &frameBufferId);
        if (CheckError())
            return false;
        _frameBufferId.reset(new GLuint(frameBufferId));
        glBindFramebuffer(GL_FRAMEBUFFER, frameBufferId);
        
        // 创建一个FBO使用的空纹理
        GLuint frameBufferTextureId;
        glGenTextures(1, &frameBufferTextureId);
        if (CheckError())
            return false;
        _frameBufferTextureId.reset(new GLuint(frameBufferTextureId));
        glBindTexture(GL_TEXTURE_2D, frameBufferTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1920, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameBufferTextureId, 0); // Texture附着到FBO
//        glBindTexture(GL_FRAMEBUFFER, 0);
        if (GLenum ret = glCheckFramebufferStatus(GL_FRAMEBUFFER); ret != GL_FRAMEBUFFER_COMPLETE) {
            NSLog(@"Bind Frame Buffer %d failed, error:%d", frameBufferId, ret);
            return false;
        }
        
        CheckError();
        
        // 创建纹理
        for (auto &texture : _textures) {
            if (texture.Activate() == false)
                return false;
        }
        
        
        
//        // 这一行的作用是解除vertexBufId的激活状态，避免其它操作不小心改动到这里。不过这种情况很少见。
//        glBindVertexArray(0);
//        if (CheckError())
//            return false;



        _needUpdate = false;
        return true;
    }
    

    

    
protected:
    const std::shared_ptr<GLContext> _context;
    bool _needUpdate = true;
    
    std::vector<GLTexture> _textures;
    
    std::unique_ptr<GLProgram> _program;
    
    GLVertexArray _vertexArray = GLVertexArray(_context);
    GL_IdHolder _frameBufferId = GL_IdHolder(nullptr, FRAME_BUFFER_DELETER);
    
    
    std::unique_ptr<GLProgram> _screenProgram;
    GLVertexArray _screenVertexArray = GLVertexArray(_context);
    GL_IdHolder _frameBufferTextureId = GL_IdHolder(nullptr, FRAME_BUFFER_DELETER);
        
    std::array<GLfloat, 4> _clearColor;
};

class GLRendererPreview : public GLRendererBase {
public:
    GLRendererPreview(std::shared_ptr<GLContext> context):GLRendererBase(context) {}
    virtual ~GLRendererPreview() {}
    
protected:
    
    
};


}

#endif /* GLRendererBase_hpp */
