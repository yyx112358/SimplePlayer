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
#include <array>

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
    
    std::optional<ImageBuffer> DownloadBuffer(std::optional<GLenum> pixelFormat = {}) const;
    
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
    static void RENDER_BUFFER_DELETER(GLuint *p);
    
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
    
    bool Activate();
    
    std::optional<GLuint> id() const {
        return _renderBufferId != nullptr ? std::make_optional<GLuint>(*_renderBufferId) : std::make_optional<GLuint>();
    }
    
protected:
    const std::shared_ptr<GLContext> _context;
    GL_IdHolder _renderBufferId = GL_IdHolder(nullptr, RENDER_BUFFER_DELETER);
    
    std::optional<GLsizei> _width, _height;
};

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
            {{-0.95, 0.95}, {0.0, 1.0},}, // 左上
            {{ 0.95, 0.95}, {1.0, 1.0},}, // 右上
            {{-0.95,-0.95}, {0.0, 0.0},}, // 左下
            {{ 0.95,-0.95}, {1.0, 0.0},}, // 右下
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
            _vertexBufferSize.emplace(_vertexBuffer.size());
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
