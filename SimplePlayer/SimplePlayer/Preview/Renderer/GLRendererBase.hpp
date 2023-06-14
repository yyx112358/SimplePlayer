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

class GLRendererProgram {
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
    GLRendererProgram(std::shared_ptr<GLContext> context) :_context(context) {}
    virtual ~GLRendererProgram() {
        _context->switchContext();
        _programId.reset();
        CheckError();
    }
    
    bool Activate() {
        _context->switchContext();
        
        if (auto program = _CompileProgramLazy())
            _programId = std::move(program);
        else
            return false;
        
        glUseProgram(*_programId);
        _UpdateUniform();
        
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
        } else {
            shader.reset(new GLuint(shaderId));
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
        } else {
            programId.reset(new GLuint(shaderProgram));
        }
        return programId;
    }
    
    // 使用_vertexShaderSource和_fragmentShaderSource
    virtual GL_IdHolder _CompileProgramLazy() {
        GL_IdHolder program(nullptr, PROGRAM_DELETER);
        if (_vertexShaderSource.empty() == false || _fragmentShaderSource.empty() == false) {
            assert(_vertexShaderSource.size() > 0 && _fragmentShaderSource.size() > 0);

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
    GLRendererBase(std::shared_ptr<GLContext> context) :_context(context), _program(new GLRendererProgram(context)) {}
    
    virtual ~GLRendererBase() {
        _context->switchContext();
        
        _program.reset();
        _vertexArrayId.reset();
    }
    
    bool UpdateShader(const std::vector<std::string> &vertexShader, const std::vector<std::string> &fragmentShader) {
        _program->UpdateShader(vertexShader, fragmentShader);
        return true;
    }
    
    bool UpdateTexture(const ImageBuffer &buffer) {
        _textureBuffer = buffer;
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
        glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        
//        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 取消注释后将启用线框模式
        _program->Activate(); // 启用Shader程序
        
        glActiveTexture(GL_TEXTURE0 + 1); // 激活纹理单元1
        glBindTexture(GL_TEXTURE_2D, _textureId); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
        _program->UpdateUniform("texture1", 1); // 更新纹理uniform
        
        glm::mat4 trans(1.0f);
        static int angle = 0;
//        trans = glm::translate(trans, glm::vec3(0.5f, 0.5f, 0.0f));
//        trans = glm::rotate(trans, glm::radians(float(angle++)), glm::vec3(0.0f, 0.0f, 1.0f));
//        trans = glm::scale(trans, glm::vec3(0.75f, 0.75f, 0.75f));
        _program->UpdateUniform("transform", trans);
        
        _program->FlushUniform();
        
        glBindVertexArray(*_vertexArrayId); // 绑定Vertex Array
//        glDrawArrays(GL_TRIANGLES, 0, 6); // 绘制三角形
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // 使用Element绘制三角形
        
        if (CheckError())
            return false;
        
        return true;
    }
    
protected:
    virtual bool _InternalUpdate() {
        if (_needUpdate == false)
            return true;
        
        _program->Activate();
        
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
        if (_textureBuffer.data != nullptr) {
            GLuint textureId;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            
            // warp参数
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    // set texture wrapping to GL_REPEAT (default wrapping method)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            // 插值filter参数
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)_textureBuffer.width, (GLsizei)_textureBuffer.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _textureBuffer.data.get());
            // glGenerateMipmap(GL_TEXTURE_2D); // 如果需要生成mipmap的话
            _textureBuffer.data.reset();// 释放内存
            
            if (CheckError())
                return false;
            _textureId = textureId;
            
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
    std::unique_ptr<GLRendererProgram> _program;
    bool _needUpdate = true;
    
    ImageBuffer _textureBuffer;
    
    GL_IdHolder _vertexArrayId = GL_IdHolder(nullptr, VERTEX_ARRAY_DELETER);
    GLuint _textureId;
        
    std::array<GLfloat, 4> _clearColor;
};

class GLRendererPreview : public GLRendererBase {
    
};


}

#endif /* GLRendererBase_hpp */
