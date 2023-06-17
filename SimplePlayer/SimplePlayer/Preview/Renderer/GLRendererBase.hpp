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
#include <vector>
#include <string>
#include <optional>
#include <array>
#include <any>
#include <map>
#include <unordered_map>
#include <typeindex>


#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#import "../../../../thirdParty/glm/glm/glm.hpp"
#include "../../../../thirdParty/glm/glm/gtc/matrix_transform.hpp"
#include "../../../../thirdParty/glm/glm/gtc/type_ptr.hpp"

#import "GLContext.hpp"
#import "ImageReader.hpp"

namespace sp {


typedef std::unique_ptr<GLuint, void(*)(GLuint *)> GL_IdHolder; // 持有GL ID的unique_ptr，支持自动释放

class GLProgram {
public:
    static void SHADER_DELETER(GLuint *p) {
        NSLog(@"Delete shader %d", *p);
        glDeleteShader(*p);
    }
    static void PROGRAM_DELETER(GLuint *p) {
        NSLog(@"Delete program %d", *p);
        glDeleteProgram(*p);
    };
public:
    GLProgram(std::shared_ptr<GLContext> context) :_context(context) {}
    
    virtual ~GLProgram() {
        _context->switchContext();
        _programId.reset();
        CheckError();
    }
    
    bool Activate() {
        _context->switchContext();
        
        _programId = _CompileOrGetProgram();
        assert(_programId != nullptr);
        if (_programId != nullptr) {
            glUseProgram(*_programId);
            FlushUniform();
        }
        
        return !CheckError();
    }
    
    bool DeActivate() {
        _context->switchContext();
        glUseProgram(0);
        return true;
    }
    
    bool UpdateShader(const std::vector<std::string> &vertexShader, const std::vector<std::string> &fragmentShader) {
        _vertexShaderSource = vertexShader;
        _fragmentShaderSource = fragmentShader;
        _needUpdate = true;
        return true;
    }
    
    bool UpdateUniform(const std::string &name, std::any uniform) {
        _uniformMap[name] = uniform;
        return true;
    }
    
    bool FlushUniform() {
        _UpdateUniform();
        return true;
    }
    
protected:
    /// 编译Shader
    virtual GL_IdHolder _CompileShader(GLenum shaderType, const std::string &source) const {
        GL_IdHolder shader(nullptr, SHADER_DELETER);

        GLuint shaderId;
        shaderId = glCreateShader(shaderType); // 创建并绑定Shader
        const char *sourceAddr = source.c_str();
        glShaderSource(shaderId, 1, &sourceAddr, NULL); // 绑定Shader源码
        glCompileShader(shaderId); // 编译Shader

        // 可选，检查编译状态。非常有用
        int success;
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
        if (!success) {
            char buf[512];
            glGetShaderInfoLog(shaderId, sizeof(buf), NULL, buf);
            NSLog(@"%s", buf);
            assert(0);
        } else {
            shader.reset(new GLuint(shaderId));
            NSLog(@"Create shader %d", *shader);
        }
        return shader;
    }
    
    // 使用shaders编译Program
    virtual GL_IdHolder _CompileProgram(const std::vector<GL_IdHolder> &shaders) const {
        // 链接Shader为Program。和CPU程序很类似，编译.o文件、链接为可执行文件。【耗时非常长】
        GL_IdHolder programId(nullptr, PROGRAM_DELETER);
        GLuint shaderProgram;
        shaderProgram = glCreateProgram();
        for (auto &shader : shaders) // 绑定shader
            glAttachShader(shaderProgram, *shader);
        glLinkProgram(shaderProgram); // 链接Shader为完整着色器程序
        
        // 检查Program是否链接成功
        int success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success); // 检查编译是否成功
        if (!success) {
            char buf[512];
            glGetProgramInfoLog(shaderProgram, sizeof(buf), NULL, buf);
            NSLog(@"%s", buf);
            assert(0);
        } else {
            programId.reset(new GLuint(shaderProgram));
            NSLog(@"Create program %d", *programId);
        }
        return programId;
    }
    
    // 使用_vertexShaderSource和_fragmentShaderSource
    virtual GL_IdHolder _CompileOrGetProgram() {
        GL_IdHolder program(nullptr, PROGRAM_DELETER);
        if (_vertexShaderSource.empty() == false || _fragmentShaderSource.empty() == false) {
            // 编译Shader
            std::vector<GL_IdHolder> shaders;
            for (auto &source : _vertexShaderSource) {
                GL_IdHolder vertexShaderId = _CompileShader(GL_VERTEX_SHADER, source);
                if (vertexShaderId != nullptr)
                    shaders.push_back(std::move(vertexShaderId));
                else
                    return program;
            }
            for (auto &source : _fragmentShaderSource) {
                GL_IdHolder fragmentShaderId = _CompileShader(GL_FRAGMENT_SHADER, source);
                if (fragmentShaderId != nullptr)
                    shaders.push_back(std::move(fragmentShaderId));
                else
                    return program;
            }
            program = _CompileProgram(shaders);
            
            // 删除Shader
            _vertexShaderSource.clear();
            _fragmentShaderSource.clear();
            shaders.clear();
        } else {
            // Source无更新，直接返回_programId
            program = std::move(_programId);
        }

        return program;
    }
    
    virtual void _UpdateUniform() {
        // 更新Uniform
        if (_programId == nullptr)
            return;
        glUseProgram(*_programId);
        for (const auto &uniPair : _uniformMap) {
            // 根据type调用对应的glUniformx()
            const static std::unordered_map<std::type_index, std::function<void(GLint location, const std::any &)>>tbl = {
                {typeid(int), [](GLint location, const std::any &val){ glUniform1i(location, std::any_cast<int>(val));}},
                {typeid(float), [](GLint location, const std::any &val){ glUniform1f(location, std::any_cast<float>(val));}},
                {typeid(glm::mat4), [](GLint location, const std::any &val){ glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(std::any_cast<glm::mat4>(val))); }},
            };
            
            
            // 查找对应的方法
            const std::any &value = uniPair.second;
            GLint location = glGetUniformLocation(*_programId, uniPair.first.c_str());
            if (tbl.count(value.type()) == 0) {
                NSLog(@"%s not found in %s", value.type().name(), __FUNCTION__);
                abort();
            }
            if (value.has_value() == false || location < 0 || tbl.count(value.type()) == 0)
                continue;
            
            // 调用
            auto &f = tbl.at(value.type());
            f(location, value);
        }
        _uniformMap.clear();
    }
    
protected:
    const std::shared_ptr<GLContext> _context;
    bool _needUpdate = true;
    
    std::vector<std::string> _vertexShaderSource;
    std::vector<std::string> _fragmentShaderSource;
    std::map<std::string, std::any> _uniformMap;
    
    GL_IdHolder _programId = GL_IdHolder(nullptr, PROGRAM_DELETER);
};

class GLTexture {
public:
    static void TEXTURE_DELETER(GLuint *p) {
        NSLog(@"Delete texture %d", *p);
        glDeleteTextures(1, p);
    }
    
public:
    GLTexture(std::shared_ptr<GLContext>context) : _context(context) {}
    GLTexture(GLTexture &&other) : _context(other._context), _textureId(std::move(other._textureId)), _buffer(other._buffer), _textureWrapS(other._textureWrapS), _textureWrapT(other._textureWrapT), _textureMinFilter(other._textureMinFilter), _textureMagFilter(other._textureMagFilter) {}
    
    virtual ~GLTexture() {
        _context->switchContext();
        
        _textureId.reset();
        _buffer.reset();
    }
    
    void UploadBuffer(ImageBuffer buffer) {
        _buffer = buffer;
    }
    
    std::optional<ImageBuffer> DownloadBuffer() {
        std::optional<ImageBuffer> buffer;
        if (_textureId == nullptr)
            return buffer;
        
        _context->switchContext();
        buffer = *_buffer;
        buffer->data = std::shared_ptr<uint8_t[]>(new uint8_t[_buffer->width * _buffer->height * 4]);
        glReadPixels(0, 0, (GLsizei)_buffer->width, (GLsizei)_buffer->height, _buffer->pixelFormat, GL_UNSIGNED_BYTE, buffer->data.get());
        if (CheckError())
            return std::optional<ImageBuffer>();
        else
            return buffer;
    }
    
    bool Activate() {
        _context->switchContext();
        
        if (_UploadBuffer() == false)
            return false;
        
        return true;
    }
    
    std::optional<GLuint> id() const {
        return _textureId != nullptr ? std::make_optional<GLuint>(*_textureId) : std::make_optional<GLuint>();
    }
    
protected:
    virtual bool _UploadBuffer() {
        if (_buffer.has_value() == false)
            return true;
        
        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        // warp参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _textureWrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _textureWrapT);
        // 插值filter参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _textureMinFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _textureMagFilter);
        
        glTexImage2D(GL_TEXTURE_2D, 0, _buffer->pixelFormat, (GLsizei)_buffer->width, (GLsizei)_buffer->height, 0, _buffer->pixelFormat, GL_UNSIGNED_BYTE, _buffer->data.get()); // 上传纹理。如果_buffer->data为空，则生成空纹理
        // glGenerateMipmap(GL_TEXTURE_2D); // 如果需要生成mipmap的话
        _buffer->data.reset();// 释放内存
        
        if (CheckError())
            return false;
        _textureId.reset(new GLuint(textureId));
        glBindTexture(GL_TEXTURE_2D, 0);
        
        NSLog(@"Create texture %d", *_textureId);
        return true;
    }
    
protected:
    const std::shared_ptr<GLContext> _context;
    GL_IdHolder _textureId = GL_IdHolder(nullptr, TEXTURE_DELETER);
    
    std::optional<ImageBuffer> _buffer;
    GLenum _textureWrapS = GL_CLAMP_TO_EDGE, _textureWrapT = GL_CLAMP_TO_EDGE;
    GLenum _textureMinFilter = GL_NEAREST, _textureMagFilter = GL_LINEAR;
};

class GLVertexArray {
public:
    
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
        _vertexArrayId.reset();
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
        
        glBindVertexArray(*_vertexArrayId); // 绑定Vertex Array
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
//        glDrawArrays(GL_TRIANGLES, 0, 6); // 绘制三角形
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // 使用Element绘制三角形
        
        CheckError();
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        _screenProgram->Activate();
        glBindVertexArray(*_screenVertexArrayId);
        glActiveTexture(GL_TEXTURE0 + 1); // 激活纹理单元1
        glBindTexture(GL_TEXTURE_2D, *_frameBufferTextureId); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
        _screenProgram->UpdateUniform("screenTexture", 1); // 更新纹理uniform
        _screenProgram->FlushUniform();

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // 使用Element绘制三角形
        
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
        CheckError();
        _screenProgram->Activate();
        {
            // 创建Vertex Array Object(VAO)。后续所有顶点操作都会储存到VAO中。OpenGL core模式下VAO必须要有。
            GLuint vertexArrayId;
            glGenVertexArrays(1, &vertexArrayId); // 生成顶点Array对象。【必须在创建Buffer前】
            glBindVertexArray(vertexArrayId); // 绑定顶点Array

            // Vertex Buffer Object(VBO)
            GLuint vertexBufId;
            struct Vertex {
                std::array<GLfloat, 2> location;
                std::array<GLfloat, 2> texture;
            } vertexBuf[4] = {
                {{-0.75, 0.75}, {0.0, 1.0},}, // 左上
                {{ 0.75, 0.75}, {1.0, 1.0},}, // 右上
                {{-0.75,-0.75}, {0.0, 0.0},}, // 左下
                {{ 0.75,-0.75}, {1.0, 0.0},}, // 右下
            };
            glGenBuffers(1, &vertexBufId); // 生成 1 个顶点缓冲区对象，vertexBufId是绑定的唯一OpenGL标识
            glBindBuffer(GL_ARRAY_BUFFER, vertexBufId); // 绑定为GL_ARRAY_BUFFER
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBuf), vertexBuf, GL_STATIC_DRAW); // 传输数据

            glVertexAttribPointer(0, vertexBuf[0].location.size(), GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, location)));// 位置
            glEnableVertexAttribArray(0); // 启用VertexAttribArray
            glVertexAttribPointer(1, vertexBuf[0].texture.size(), GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, texture)));// 纹理
            glEnableVertexAttribArray(1);

            // Element Buffer Object(EBO)
            GLuint elementBufId;
            GLuint elementBuf[] = {
                0, 1, 2, // 第一个三角形
                1, 3, 2, // 第二个三角形
            };
            glGenBuffers(1, &elementBufId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufId); // 绑定为GL_ELEMENT_ARRAY_BUFFER
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elementBuf), elementBuf, GL_STATIC_DRAW);

            _screenVertexArrayId.reset(new GLuint(vertexArrayId));
            glBindVertexArray(0);
            if (CheckError())
                return false;
        }
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
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // 重新绑定到屏幕缓冲
        
        // 创建Vertex Array Object(VAO)。后续所有顶点操作都会储存到VAO中。OpenGL core模式下VAO必须要有。
        GLuint vertexArrayId;
        glGenVertexArrays(1, &vertexArrayId); // 生成顶点Array对象。【必须在创建Buffer前】
        glBindVertexArray(vertexArrayId); // 绑定顶点Array
        
        // Vertex Buffer Object(VBO)
        GLuint vertexBufId;
        struct Vertex {
            std::array<GLfloat, 2> location;
            std::array<GLfloat, 3> color;
            std::array<GLfloat, 2> texture;
        } vertexBuf[4] = {
            {{-0.75, 0.75}, {1.0, 0.0, 0.0}, {0.0, 1.0},}, // 左上
            {{ 0.75, 0.75}, {0.0, 1.0, 0.0}, {1.0, 1.0},}, // 右上
            {{-0.75,-0.75}, {0.0, 0.0, 1.0}, {0.0, 0.0},}, // 左下
            {{ 0.75,-0.75}, {1.0, 0.0, 0.0}, {1.0, 0.0},}, // 右下
        };
        glGenBuffers(1, &vertexBufId); // 生成 1 个顶点缓冲区对象，vertexBufId是绑定的唯一OpenGL标识
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufId); // 绑定为GL_ARRAY_BUFFER
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBuf), vertexBuf, GL_STATIC_DRAW); // 传输数据
        
        glVertexAttribPointer(0, vertexBuf[0].location.size(), GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, location)));// 位置
        glEnableVertexAttribArray(0); // 启用VertexAttribArray
        glVertexAttribPointer(1, vertexBuf[0].color.size(), GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, color)));// 颜色
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, vertexBuf[0].texture.size(), GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, texture)));// 纹理
        glEnableVertexAttribArray(2);
        
        
        // Element Buffer Object(EBO)
        GLuint elementBufId;
        GLuint elementBuf[] = {
            0, 1, 2, // 第一个三角形
            1, 3, 2, // 第二个三角形
        };
        glGenBuffers(1, &elementBufId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufId); // 绑定为GL_ELEMENT_ARRAY_BUFFER
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elementBuf), elementBuf, GL_STATIC_DRAW);
        
        _vertexArrayId.reset(new GLuint(vertexArrayId));
        
        
        // 创建纹理
        for (auto &texture : _textures) {
            if (texture.Activate() == false)
                return false;
        }
        
        
        
        // 这一行的作用是解除vertexBufId的激活状态，避免其它操作不小心改动到这里。不过这种情况很少见。
        glBindVertexArray(0);
        if (CheckError())
            return false;



        _needUpdate = false;
        return true;
    }
    

    

    
protected:
    const std::shared_ptr<GLContext> _context;
    bool _needUpdate = true;
    
    std::vector<GLTexture> _textures;
    
    std::unique_ptr<GLProgram> _program;
    GL_IdHolder _vertexArrayId = GL_IdHolder(nullptr, VERTEX_ARRAY_DELETER);
    GL_IdHolder _frameBufferId = GL_IdHolder(nullptr, FRAME_BUFFER_DELETER);
    
    
    std::unique_ptr<GLProgram> _screenProgram;
    GL_IdHolder _screenVertexArrayId = GL_IdHolder(nullptr, VERTEX_ARRAY_DELETER);
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
