//
//  GLProgram.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "GLProgram.hpp"

#include <string>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "SPLog.h"

using namespace sp;


void GLProgram::SHADER_DELETER(GLuint *p)
{
    SPLOGD("Delete shader %d", *p);
    glDeleteShader(*p);
}

void GLProgram::PROGRAM_DELETER(GLuint *p)
{
    SPLOGD("Delete program %d", *p);
    glDeleteProgram(*p);
}

bool GLProgram::Activate()
{
    _context->SwitchContext();
    
    _programId = _CompileOrGetProgram();
    assert(_programId != nullptr);
    if (_programId != nullptr) {
        glUseProgram(*_programId);
        FlushUniform();
    }
    
    return !GLCheckError();
}

bool GLProgram::DeActivate()
{
    _context->SwitchContext();
    glUseProgram(0);
    return true;
}

bool GLProgram::UpdateShader(const std::vector<std::string> &vertexShader, const std::vector<std::string> &fragmentShader)
{
    _vertexShaderSource = vertexShader;
    _fragmentShaderSource = fragmentShader;
    _needUpdate = true;
    return true;
}

bool GLProgram::UpdateUniform(const std::string &name, GLUniform uniform)
{
    _uniformMap[name] = uniform;
    return true;
}

bool GLProgram::FlushUniform()
{
    _UpdateUniform();
    return true;
}

/// 编译Shader
GL_IdHolder GLProgram::_CompileShader(GLenum shaderType, const std::string &source) const
{
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
        SPLOGD("%s", buf);
        assert(0);
    } else {
        shader.reset(new GLuint(shaderId));
        SPLOGD("Create shader %d", *shader);
    }
    return shader;
}

// 使用shaders编译Program
GL_IdHolder GLProgram::_CompileProgram(const std::vector<GL_IdHolder> &shaders) const
{
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
        SPLOGD("%s", buf);
        assert(0);
    } else {
        programId.reset(new GLuint(shaderProgram));
        SPLOGD("Create program %d", *programId);
    }
    return programId;
}

// 使用_vertexShaderSource和_fragmentShaderSource
GL_IdHolder GLProgram::_CompileOrGetProgram()
{
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

void GLProgram::_UpdateUniform()
{
    // 更新Uniform
    if (_programId == nullptr)
        return;
    glUseProgram(*_programId);
    for (const auto &uniPair : _uniformMap) {

        const GLUniform &value = uniPair.second;
        
        // 查找对应location
        GLint location = glGetUniformLocation(*_programId, uniPair.first.c_str());
        assert(location >= 0);
        
        // 根据type调用对应的glUniformx()
        if (std::holds_alternative<int>(value))
            glUniform1i(location, std::get<int>(value));
        else if (std::holds_alternative<float>(value))
            glUniform1f(location, std::get<float>(value));
        else if (std::holds_alternative<glm::mat4>(value))
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(std::get<glm::mat4>(value)));
        else
            assert(0);
    }
    _uniformMap.clear();
}
