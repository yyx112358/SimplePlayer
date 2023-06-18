//
//  GLContext.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#include "GLContext.hpp"

#include <typeindex>

#import <OpenGL/gl3.h>
#import "../../../../thirdParty/glm/glm/glm.hpp"
#include "../../../../thirdParty/glm/glm/gtc/matrix_transform.hpp"
#include "../../../../thirdParty/glm/glm/gtc/type_ptr.hpp"

using namespace sp;


#pragma mark -GLContext

GLContext::~GLContext() {
    std::lock_guard lock(_mutex);
    _context = nil;
}

bool GLContext::init() {
    std::lock_guard lock(_mutex);

    NSOpenGLPixelFormatAttribute attrs[] =
    {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,  // 【声明使用OpenGL3.2】，不配置则默认OpenGL 2
        0
    };
    
    NSOpenGLPixelFormat * pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    _context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
    
    return _context != nil;
}

NSOpenGLContext *GLContext::context() {
    return _context;
}

/// 切换到本Context
bool GLContext::switchContext() {
    if (_context == nil && init() == false)
        return false;
    std::lock_guard lock(_mutex);
    
    [_context makeCurrentContext];
    return true;
}

// 在OpenGL绘制完成后，调用flush方法将绘制的结果显示到窗口上
// 应在最后一个GL操作完成时调用
bool GLContext::flush() {
    if (_context == nil && init() == false)
        return false;
    std::lock_guard lock(_mutex);
    
    [_context flushBuffer];
    return true;
}

bool GLContext::CheckGLError(const char *file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        const char *error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
//            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
//            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
            default: assert(0);break;
        }
        NSLog(@"%s | %s[%d]", error, file, line);
    }
    return errorCode != 0;
}

#pragma mark -GLProgram

void GLProgram::SHADER_DELETER(GLuint *p) {
    NSLog(@"Delete shader %d", *p);
    glDeleteShader(*p);
}
void GLProgram::PROGRAM_DELETER(GLuint *p) {
    NSLog(@"Delete program %d", *p);
    glDeleteProgram(*p);
}

bool GLProgram::Activate() {
    _context->switchContext();
    
    _programId = _CompileOrGetProgram();
    assert(_programId != nullptr);
    if (_programId != nullptr) {
        glUseProgram(*_programId);
        FlushUniform();
    }
    
    return !CheckError();
}

bool GLProgram::DeActivate() {
    _context->switchContext();
    glUseProgram(0);
    return true;
}

bool GLProgram::UpdateShader(const std::vector<std::string> &vertexShader, const std::vector<std::string> &fragmentShader) {
    _vertexShaderSource = vertexShader;
    _fragmentShaderSource = fragmentShader;
    _needUpdate = true;
    return true;
}

bool GLProgram::UpdateUniform(const std::string &name, std::any uniform) {
    _uniformMap[name] = uniform;
    return true;
}

bool GLProgram::FlushUniform() {
    _UpdateUniform();
    return true;
}

/// 编译Shader
GL_IdHolder GLProgram::_CompileShader(GLenum shaderType, const std::string &source) const {
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
GL_IdHolder GLProgram::_CompileProgram(const std::vector<GL_IdHolder> &shaders) const {
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
GL_IdHolder GLProgram::_CompileOrGetProgram() {
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

void GLProgram::_UpdateUniform() {
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


#pragma mark -GLTexture

void GLTexture::TEXTURE_DELETER(GLuint *p) {
    NSLog(@"Delete texture %d", *p);
    glDeleteTextures(1, p);
}
    
   
void GLTexture::UploadBuffer(ImageBuffer buffer) {
    _buffer = buffer;
}

std::optional<ImageBuffer> GLTexture::DownloadBuffer() {
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

bool GLTexture::Activate() {
    _context->switchContext();
    
    if (_UploadBuffer() == false)
        return false;
    
    return true;
}

std::optional<GLuint> GLTexture::id() const {
    return _textureId != nullptr ? std::make_optional<GLuint>(*_textureId) : std::make_optional<GLuint>();
}

bool GLTexture::_UploadBuffer() {
    if (_buffer.has_value() == false)
        return true;
    
    GLuint textureId;
    glGenTextures(1, &textureId);
    _textureId.reset(new GLuint(textureId));
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
    
//    glBindTexture(GL_TEXTURE_2D, 0);
    NSLog(@"Create texture %d", *_textureId);
    return true;
}
