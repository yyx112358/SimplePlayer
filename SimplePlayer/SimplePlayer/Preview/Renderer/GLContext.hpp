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
#include <variant>
#include <unordered_map>
#include <vector>
#include <memory>

#import "../../../../thirdParty/glm/glm/fwd.hpp"

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

typedef std::variant<int32_t, uint32_t, float, glm::mat4> GLUniform;

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
    const std::shared_ptr<GLContext> _context;
    bool _needUpdate = true;
    
    std::vector<std::string> _vertexShaderSource;
    std::vector<std::string> _fragmentShaderSource;
    std::unordered_map<std::string, GLUniform> _uniformMap;
    
    GL_IdHolder _programId = GL_IdHolder(nullptr, PROGRAM_DELETER);
};


class GLTexture {
public:
    static void TEXTURE_DELETER(GLuint *p);
    
public:
    GLTexture(std::shared_ptr<GLContext>context) : _context(context) {}
    GLTexture(std::shared_ptr<GLContext>context, ImageBuffer buffer) : _context(context), _buffer(std::move(buffer)) {}
    GLTexture(std::shared_ptr<GLContext>context, GLsizei width, GLsizei height) : _context(context), _buffer(ImageBuffer{.width = width, .height = height, .pixelFormat = GL_RGBA}) {}
    
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
    GLsizei width() const {return _buffer ? _buffer->width : -1;}
    GLsizei height() const {return _buffer ? _buffer->height : -1;}
    
protected:
    virtual bool _UploadBuffer();
    
protected:
    const std::shared_ptr<GLContext> _context;
    GL_IdHolder _textureId = GL_IdHolder(nullptr, TEXTURE_DELETER);
    bool _needUpdate = true;
    
    std::optional<ImageBuffer> _buffer;
    GLenum _textureWrapS = GL_CLAMP_TO_EDGE, _textureWrapT = GL_CLAMP_TO_EDGE;
    GLenum _textureMinFilter = GL_NEAREST, _textureMagFilter = GL_LINEAR;
};

/// Render Buffer Object (RBO)
/// 仅可写入的缓冲区，为离屏渲染到FBO优化
class GLRenderBuffer {
public:
    static void RENDER_BUFFER_DELETER(GLuint *p) {
        NSLog(@"Delete render buffer %d", *p);
        glDeleteRenderbuffers(1, p);
    }
    
public:
    GLRenderBuffer(std::shared_ptr<GLContext>context) : _context(context) {}
    GLRenderBuffer(std::shared_ptr<GLContext>context, GLsizei width, GLsizei height) : _context(context), _width(width), _height(height) {}
    virtual ~GLRenderBuffer() {
        _context->switchContext();
    }
    
    void setSize(GLsizei width, GLsizei height) {
        _width = width;
        _height = height;
    }
    
    std::optional<ImageBuffer> DownloadBuffer();
    
    bool Activate() {
        _context->switchContext();
        
        if (_renderBufferId == nullptr) {
            if (_width.has_value() == false || _height.has_value() == false)
                return false;
            
            GLuint renderBufferId;
            glGenRenderbuffers(1, &renderBufferId);
            auto holder = GL_IdHolder(new GLuint(renderBufferId), RENDER_BUFFER_DELETER);
            NSLog(@"Create render buffer %d", renderBufferId);
            
            glBindRenderbuffer(GL_RENDERBUFFER, renderBufferId);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, *_width, *_height);
            
            if (CheckError())
                return false;
            
            _renderBufferId = std::move(holder);
        }
        glBindRenderbuffer(GL_RENDERBUFFER, *_renderBufferId);
        
        if (CheckError())
            return false;
        
        // glBindRenderbuffer( GL_RENDERBUFFER, 0 );
        return true;
    }
    
    std::optional<GLuint> id() const {
        return _renderBufferId != nullptr ? std::make_optional<GLuint>(*_renderBufferId) : std::make_optional<GLuint>();
    }
    
protected:
    const std::shared_ptr<GLContext> _context;
    GL_IdHolder _renderBufferId = GL_IdHolder(nullptr, RENDER_BUFFER_DELETER);
    
    std::optional<GLsizei> _width, _height;
};


class Parameter {
public:
//    std::any parameter;
//    std::any shadowParameter;
//    
//    void update(const std::any&input) {
//        shadowParameter = input;
//    }
//    std::any& get() {
//        return shadowParameter.has_value() ? shadowParameter : parameter;
//    }
//    std::any getReal() {
//        return parameter;
//    }
//    std::any& getAndUpdate() {
//        parameter.swap(shadowParameter);
//        shadowParameter.reset();
//        return get();
//    }
//    bool isUpdated() {
//        return shadowParameter.has_value() == false;
//    }
};

}

#endif /* GLContext_hpp */
