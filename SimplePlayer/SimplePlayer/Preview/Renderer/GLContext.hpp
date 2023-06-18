//
//  GLContext.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#ifndef GLContext_hpp
#define GLContext_hpp

#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>

#include <mutex>
#include <any>
#include <unordered_map>
#include <vector>
#include <memory>

#import "ImageReader.hpp"

namespace sp {

typedef std::unique_ptr<GLuint, void(*)(GLuint *)> GL_IdHolder; // 持有GL ID的unique_ptr，支持自动释放

/**
 * GL上下文
 * TODO: 抽出公共跨平台部分
 */
class GLContext {
public:
    virtual ~GLContext();
    
    virtual bool init();
    
    virtual NSOpenGLContext *context();
    
    /// 切换到本Context
    virtual bool switchContext();
    
    // 在OpenGL绘制完成后，调用flush方法将绘制的结果显示到窗口上
    // 应在最后一个GL操作完成时调用
    virtual bool flush();
    
    // 检查错误
    static bool CheckGLError(const char *file, int line);
    
protected:
    NSOpenGLContext *_context;
    std::mutex _mutex;
};

#define CheckError() GLContext::CheckGLError(__FILE__, __LINE__)



class GLProgram {
public:
    static void SHADER_DELETER(GLuint *p);
    static void PROGRAM_DELETER(GLuint *p);
public:
    GLProgram(std::shared_ptr<GLContext> context) :_context(context) {}
    
    virtual ~GLProgram() {
        _context->switchContext();
        _programId.reset();
        CheckError();
    }
    
    bool Activate();
    
    bool DeActivate();
    
    bool UpdateShader(const std::vector<std::string> &vertexShader, const std::vector<std::string> &fragmentShader);
    
    bool UpdateUniform(const std::string &name, std::any uniform);
    
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
    const std::shared_ptr<GLContext> _context;
    bool _needUpdate = true;
    
    std::vector<std::string> _vertexShaderSource;
    std::vector<std::string> _fragmentShaderSource;
    std::unordered_map<std::string, std::any> _uniformMap;
    
    GL_IdHolder _programId = GL_IdHolder(nullptr, PROGRAM_DELETER);
};


class GLTexture {
public:
    static void TEXTURE_DELETER(GLuint *p);
    
public:
    GLTexture(std::shared_ptr<GLContext>context) : _context(context) {}
    GLTexture(GLTexture &&other) : _context(other._context), _textureId(std::move(other._textureId)), _buffer(other._buffer), _textureWrapS(other._textureWrapS), _textureWrapT(other._textureWrapT), _textureMinFilter(other._textureMinFilter), _textureMagFilter(other._textureMagFilter) {}
    
    virtual ~GLTexture() {
        _context->switchContext();
        
        _textureId.reset();
        _buffer.reset();
    }
    
    /// 上传Buffer，不阻塞。Activate时才真正上传
    void UploadBuffer(ImageBuffer buffer);
    
    std::optional<ImageBuffer> DownloadBuffer();
    
    bool Activate();
    
    std::optional<GLuint> id() const;
    
protected:
    virtual bool _UploadBuffer();
    
protected:
    const std::shared_ptr<GLContext> _context;
    GL_IdHolder _textureId = GL_IdHolder(nullptr, TEXTURE_DELETER);
    
    std::optional<ImageBuffer> _buffer;
    GLenum _textureWrapS = GL_CLAMP_TO_EDGE, _textureWrapT = GL_CLAMP_TO_EDGE;
    GLenum _textureMinFilter = GL_NEAREST, _textureMagFilter = GL_LINEAR;
};


class Parameter {
public:
    std::any parameter;
    std::any shadowParameter;
    
    void update(const std::any&input) {
        shadowParameter = input;
    }
    std::any& get() {
        return shadowParameter.has_value() ? shadowParameter : parameter;
    }
    std::any getReal() {
        return parameter;
    }
    std::any& getAndUpdate() {
        parameter.swap(shadowParameter);
        shadowParameter.reset();
        return get();
    }
    bool isUpdated() {
        return shadowParameter.has_value() == false;
    }
};

}

#endif /* GLContext_hpp */
