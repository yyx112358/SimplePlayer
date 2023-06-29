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
#include "ImageWriterBmp.h"

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
            NSLog(@"Create vertex array %d", *_vertexArrayId);
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
            
            if (CheckError())
                return false;
            _vertexBufferId.emplace(vertexBufId);
            _vertexBufferSize.emplace(_vertexBuffer.size() * sizeof(_vertexBuffer[0]) / sizeof(float));
            _vertexBuffer.clear();
        }
        
        // Element Buffer Object(EBO)
        if (_elementBuffer.size() > 0) {
            GLuint elementBufId;
            
            glGenBuffers(1, &elementBufId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufId); // 绑定为GL_ELEMENT_ARRAY_BUFFER
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, _elementBuffer.size() * sizeof(_elementBuffer[0]), _elementBuffer.data(), GL_STATIC_DRAW);
            
            if (CheckError())
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

class GLFrameBuffer {
public:
    static void FRAME_BUFFER_DELETER(GLuint *p) {
        NSLog(@"Delete frame buffer %d", *p);
        glDeleteFramebuffers(1, p);
    }
public:
    GLFrameBuffer(std::shared_ptr<GLContext> context) :_context(context) {}
    virtual ~GLFrameBuffer() {
        _context->switchContext();
    }
    
    void UpdateAttachTextures(const std::vector<std::shared_ptr<GLTexture>> &textures) {
        _attachTextures = textures;
        
        // 纹理单元个数有限，需要检查
        GLint MAX_TEXTURE_UNIT;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &MAX_TEXTURE_UNIT);
        assert(_attachTextures.size() + _attachRenderBuffers.size() <= MAX_TEXTURE_UNIT);
    }
    
    const std::vector<std::shared_ptr<GLTexture>> &GetAttachTextures() const {
        return _attachTextures;
    }
    
    const std::shared_ptr<GLTexture> GetOutputTexture() const {
        return _attachTextures.size() > 0 ? _attachTextures[0] : nullptr;
    }
    
    void UpdateAttachRenderBuffers(const std::vector<std::shared_ptr<GLRenderBuffer>> &rbos) {
        _attachRenderBuffers = rbos;
    }
    
    const std::vector<std::shared_ptr<GLRenderBuffer>> &GetAttachRenderBuffers() const {
        return _attachRenderBuffers;
    }
    
    bool Activate() {
        _context->switchContext();
        
        if (_frameBufferId == nullptr) {
            // 创建帧缓冲（Frame Buffer Object）
            GLuint frameBufferId;
            glGenFramebuffers(1, &frameBufferId);
            auto holder = GL_IdHolder(new GLuint(frameBufferId), FRAME_BUFFER_DELETER);
            glBindFramebuffer(GL_FRAMEBUFFER, frameBufferId);
            if (CheckError())
                return false;
            
            assert(_attachTextures.size() > 0 || _attachRenderBuffers.size() > 0);
            assert(_attachTextures.size() + _attachRenderBuffers.size() <= 16);
            
            GLenum attachId = GL_COLOR_ATTACHMENT0;
            // Texture附着到FBO
            // 绝大多数情况下，只会用到_attachTextures[0]。除非是需要同时在多个Texture上绘制
            for (auto &tex : _attachTextures) {
                if (tex->Activate() == false)
                    return false;
                
                glActiveTexture(attachId); // 激活纹理单元
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachId, GL_TEXTURE_2D, *tex->id(), 0);
                
                if (CheckError())
                    return false;
                else
                    attachId++;
            }
            // RenderBuffer附着到FBO
            // 不可作为被采样纹理。一般用于Depth/Stencil Buffer
            for (auto &rbo : _attachRenderBuffers) {
                if (rbo->Activate() == false)
                    return false;
                
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachId, GL_RENDERBUFFER, *rbo->id());
                
                if (CheckError())
                    return false;
                else
                    attachId++;
            }
            
            
            if (GLenum ret = glCheckFramebufferStatus(GL_FRAMEBUFFER); ret != GL_FRAMEBUFFER_COMPLETE) {
                NSLog(@"Bind Frame Buffer %d failed, error:%d", frameBufferId, ret);
                return false;
            }
            
            if (CheckError())
                return false;
            
            _frameBufferId = std::move(holder);
        }
        
        return true;
    }
    
protected:
    const std::shared_ptr<GLContext> _context;
    GL_IdHolder _frameBufferId = GL_IdHolder(nullptr, FRAME_BUFFER_DELETER);
    
    std::vector<std::shared_ptr<GLTexture>> _attachTextures;
    std::vector<std::shared_ptr<GLRenderBuffer>> _attachRenderBuffers;
};

class GLRendererBase {
public:
    GLRendererBase(std::shared_ptr<GLContext> context) :_context(context), _program(new GLProgram(context)) { }
    
    virtual ~GLRendererBase() {
        _context->switchContext();
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
            auto texture = std::make_shared<GLTexture>(_context, buffer);
            _textures.emplace_back(std::move(texture));
        }
        _needUpdate = true;
        return true;
    }
    
    bool UpdateTexture(const std::vector<std::shared_ptr<GLTexture>> &textures) {
        GLint MAX_TEXTURE_UNIT;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &MAX_TEXTURE_UNIT);
        assert(textures.size() <= MAX_TEXTURE_UNIT);
        
        _textures = textures;
        _needUpdate = true;
        return true;
    }
    
    bool UpdateOutputTexture(std::shared_ptr<GLTexture> texture) {
        _outputTexture = texture;
        _needUpdate = true;
        return true;
    }
    
    std::shared_ptr<GLTexture> GetOutputTexture() {
        return _outputTexture;
    }
    
    bool UpdateTransform(const glm::mat4 &transform) {
        _transform = transform;
        return true;
    }
    
    const glm::mat4 &GetTransform() {
        return _transform;
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
    
    // 立即更新
    bool SyncUpdate() {
        // 获取当前OpenGL上下文
        if (_context == nullptr || _context->switchContext() == false)
            return false;
        // 更新内部参数
        return _InternalUpdate();
    }
    
    bool Render() {
        // 获取当前OpenGL上下文
        if (_context == nullptr || _context->switchContext() == false)
            return false;
        // 更新内部参数
        if (_InternalUpdate() == false)
            return false;
        
        return _InternalRender();
    }
protected:
    virtual bool _InternalUpdate() {
        if (_needUpdate == false)
            return true;
        
        _program->Activate();
        
        // 创建一个FBO使用的空纹理
        _frameBuffer->UpdateAttachTextures({_outputTexture});
        _frameBuffer->Activate();
        
        // 创建纹理
        for (auto &texture : _textures) {
            if (texture->Activate() == false)
                return false;
        }
        
        if (_outputTexture->Activate() == false)
            return false;
        
        //        // 这一行的作用是解除vertexBufId的激活状态，避免其它操作不小心改动到这里。不过这种情况很少见。
        //        glBindVertexArray(0);
        //        if (CheckError())
        //            return false;
        
        _needUpdate = false;
        return true;
    }
    
    virtual bool _InternalRender() {
        // 上屏绘制
        _frameBuffer->Activate();
        glViewport(0, 0, _outputTexture->width(), _outputTexture->height());
        glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        
        //        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 取消注释后将启用线框模式
        _program->Activate(); // 启用Shader程序
        CheckError();
        
        _vertexArray.Activate();
        for (int i = 0; i < _textures.size(); i++) {
            assert(_textures[i]->id().has_value());
            
            glActiveTexture(GL_TEXTURE0 + i); // 激活纹理单元1
            glBindTexture(GL_TEXTURE_2D, *_textures[i]->id()); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
            
            char textureName[] = "texture00";
            snprintf(textureName, sizeof(textureName), "texture%d", i);
            _program->UpdateUniform(textureName, i); // 更新纹理uniform
        }
        
        CheckError();
        
        _program->UpdateUniform("transform", _transform);
        
        _program->FlushUniform();
        
        _vertexArray.Render();
        CheckError();
        
        
        if (CheckError())
            return false;
        
        return true;
    }
    

    

    
protected:
    const std::shared_ptr<GLContext> _context;
    bool _needUpdate = true;
    
    /// 输入纹理，多输入
    std::vector<std::shared_ptr<GLTexture>> _textures;
    /// 输出纹理，单输出
    std::shared_ptr<GLTexture> _outputTexture;
    /// 变换矩阵
    glm::mat4 _transform = glm::mat4(1.0f);
    
    std::unique_ptr<GLProgram> _program;
    GLVertexArray _vertexArray = GLVertexArray(_context);
    std::shared_ptr<GLFrameBuffer> _frameBuffer = std::make_shared<GLFrameBuffer>(_context);
    
    std::array<GLfloat, 4> _clearColor;
};

class GLRendererPreview : public GLRendererBase {
public:
    GLRendererPreview(std::shared_ptr<GLContext> context):GLRendererBase(context) {
        
    }
    virtual ~GLRendererPreview() {}
    
    virtual bool UpdatePreviewSize(int width, int height) {
        _width = width;
        _height = height;
        return true;
    }
    
protected:
    bool _InternalUpdate() override {
        if (_needUpdate == false)
            return true;
        
        UpdateShader({R"(
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
        _program->Activate();
        _vertexArray.Activate();
        
        _needUpdate = false;
        return true;
    }
    
    bool _InternalRender() override {
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // 绑定到屏幕
        glViewport(0, 0, _width, _height);
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        
        _program->Activate();
        _vertexArray.Activate();
        glActiveTexture(GL_TEXTURE0 + 0); // 激活纹理单元1
        glBindTexture(GL_TEXTURE_2D, *_textures[0]->id()); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
        _program->UpdateUniform("screenTexture", 0); // 更新纹理uniform
        _program->FlushUniform();
        
        _vertexArray.Render();
        
        if (CheckError())
            return false;
        
        return true;
    }
    
protected:
    int _width = 0, _height = 0;
};


}

#endif /* GLRendererBase_hpp */
