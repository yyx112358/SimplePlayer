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
//        assert(buffers.size() <= MAX_TEXTURE_UNIT);
        
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
    
    bool UpdateUniform(const std::string &name, GLUniform uniform) {
        return _program->UpdateUniform(name, uniform);
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

class GLRendererCharPainting : public GLRendererBase {
public:
    GLRendererCharPainting(std::shared_ptr<GLContext> context):GLRendererBase(context) {
        
    }
    virtual ~GLRendererCharPainting() {}
    
    void SetCharSize(int width, int height) {
        _charWidth = width;
        _charHeight = height;
        _needUpdate = true;
    }
    
protected:
    
    void _AddVertexByDrawArray() {
        int texWidth = _textures[0]->width(), texHeight = _textures[0]->height();
        int charWidth = _charWidth, charHeight = _charHeight;
        std::vector<GLVertexArray::VertexBuffer> bufs;
        for (int y = 0; y < texHeight; y += charHeight) {
            for (int x = 0; x < texWidth; x += charWidth) {
                { // 左上
                    float posX = x, posY = y;
                    GLVertexArray::VertexBuffer vtx;
                    vtx.location[0] = -1 + (posX / texWidth) * 2;
                    vtx.location[1] = -1 + (1 - posY / texHeight) * 2;
                    vtx.texture[0]  =  0 + (float(x) / texWidth) / 1;
                    vtx.texture[1]  =  0 + (1 - float(y) / texHeight) / 1;
                    bufs.push_back(vtx);
                }
                { // 右上
                    float posX = x + charWidth, posY = y;
                    GLVertexArray::VertexBuffer vtx;
                    vtx.location[0] = -1 + (posX / texWidth) * 2;
                    vtx.location[1] = -1 + (1 - posY / texHeight) * 2;
                    vtx.texture[0]  =  0 + (float(x) / texWidth) / 1;
                    vtx.texture[1]  =  0 + (1 - float(y) / texHeight) / 1;
                    bufs.push_back(vtx);
                }
                { // 左下
                    float posX = x, posY = y + charHeight;
                    GLVertexArray::VertexBuffer vtx;
                    vtx.location[0] = -1 + (posX / texWidth) * 2;
                    vtx.location[1] = -1 + (1 - posY / texHeight) * 2;
                    vtx.texture[0]  =  0 + (float(x) / texWidth) / 1;
                    vtx.texture[1]  =  0 + (1 - float(y) / texHeight) / 1;
                    bufs.push_back(vtx);
                }
                { // 左下
                    float posX = x, posY = y + charHeight;
                    GLVertexArray::VertexBuffer vtx;
                    vtx.location[0] = -1 + (posX / texWidth) * 2;
                    vtx.location[1] = -1 + (1 - posY / texHeight) * 2;
                    vtx.texture[0]  =  0 + (float(x) / texWidth) / 1;
                    vtx.texture[1]  =  0 + (1 - float(y) / texHeight) / 1;
                    bufs.push_back(vtx);
                }
                { // 右上
                    float posX = x + charWidth, posY = y;
                    GLVertexArray::VertexBuffer vtx;
                    vtx.location[0] = -1 + (posX / texWidth) * 2;
                    vtx.location[1] = -1 + (1 - posY / texHeight) * 2;
                    vtx.texture[0]  =  0 + (float(x) / texWidth) / 1;
                    vtx.texture[1]  =  0 + (1 - float(y) / texHeight) / 1;
                    bufs.push_back(vtx);
                }
                { // 右下
                    float posX = x + charWidth, posY = y + charHeight;
                    GLVertexArray::VertexBuffer vtx;
                    vtx.location[0] = -1 + (posX / texWidth) * 2;
                    vtx.location[1] = -1 + (1 - posY / texHeight) * 2;
                    vtx.texture[0]  =  0 + (float(x) / texWidth) / 1;
                    vtx.texture[1]  =  0 + (1 - float(y) / texHeight) / 1;
                    bufs.push_back(vtx);
                }
            }
        }
        _vertexArray.UpdateVertexBuffer(bufs);
        _vertexArray.UpdateElementBuffer({});
        _vertexArray.Activate();
    }
    
    bool _InternalUpdate() override {
        if (_needUpdate == false)
            return true;
        

        _AddVertexByDrawArray();
        
        GLint p = -1;
        glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &p);
        glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &p);
        
        return GLRendererBase::_InternalUpdate();
    }
    
    bool _InternalRender() override {
//        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 取消注释后将启用线框模式
        GLRendererBase::_InternalRender();
//        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // 取消注释后将启用线框模式
        static bool b = true;
        if (b) {
            if (auto buffer = _frameBuffer->GetOutputTexture()->DownloadBuffer())
                writeBMP2File("output.bmp", buffer->data.get(), buffer->width, buffer->height, 4);
            b = false;
        }
        return true;
    }
    
protected:
    int _charWidth = 8, _charHeight = 12;
};


}

#endif /* GLRendererBase_hpp */
