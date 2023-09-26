//
//  GLProgram.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>

#include "IGLContext.hpp"

namespace sp {

typedef std::variant<int32_t, uint32_t, float, glm::mat4> GLUniform;

class GLProgram {
public:
    static void SHADER_DELETER(GLuint p);
    static void PROGRAM_DELETER(GLuint p);
public:
    GLProgram(std::shared_ptr<IGLContext> context) :_context(context) {}
    
    virtual ~GLProgram() {
        _context->SwitchContext();
        _programId.reset();
        GLCheckError();
    }
    
    bool Activate();
    
    bool DeActivate();
    
    bool UpdateShader(const std::vector<std::string> &vertexShader, const std::vector<std::string> &fragmentShader);
    
    bool UpdateUniform(const std::string &name, GLUniform uniform);
    
    bool FlushUniform();
    
protected:
    /// 编译Shader
    virtual GL_IdHolder _CompileShader(GLenum shaderType, const std::string &source) const;
    
    // 使用shaders编译Program
    virtual GL_IdHolder _CompileProgram(const std::vector<GL_IdHolder> &shaders) const ;
    
    // 使用_vertexShaderSource和_fragmentShaderSource
    virtual GL_IdHolder _CompileOrGetProgram();
    
    virtual void _UpdateUniform();
    
protected:
    const std::shared_ptr<IGLContext> _context;
    bool _needUpdate = true;
    
    std::vector<std::string> _vertexShaderSource;
    std::vector<std::string> _fragmentShaderSource;
    std::unordered_map<std::string, GLUniform> _uniformMap;
    
    GL_IdHolder _programId = GL_IdHolder(PROGRAM_DELETER);
};

}

