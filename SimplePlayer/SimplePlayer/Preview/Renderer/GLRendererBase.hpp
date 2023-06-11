//
//  GLRendererBase.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#ifndef GLRendererBase_hpp
#define GLRendererBase_hpp

#import <Foundation/Foundation.h>
#include <string>
#include <optional>
#include <array>
#include <any>
#include <mutex>


#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#import "ImageReader.hpp"

namespace sp {

/**
 * GL环境
 * TODO: 抽出公共跨平台部分
 */
class GLContext {
public:
    virtual ~GLContext() {
        std::lock_guard lock(_mutex);
        _context = nil;
    }
    
    virtual bool init() {
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
    
    virtual NSOpenGLContext *context() {
        return _context;
    }
    
    /// 切换到本Context
    virtual bool switchContext() {
        if (_context == nil && init() == false)
            return false;
        std::lock_guard lock(_mutex);
        
        [_context makeCurrentContext];
        return true;
    }
    
    // 在OpenGL绘制完成后，调用flush方法将绘制的结果显示到窗口上
    // 应在最后一个GL操作完成时调用
    virtual bool flush() {
        if (_context == nil && init() == false)
            return false;
        std::lock_guard lock(_mutex);
        
        [_context flushBuffer];
        return true;
    }
    
protected:
    NSOpenGLContext *_context;
    std::mutex _mutex;
};

#define CheckError() _CheckGLError(__FILE__, __LINE__)

class BaseGLRenderer {
public:
    typedef std::unique_ptr<GLuint, void(*)(GLuint *)> GL_IdHolder; // 持有GL ID的unique_ptr，支持自动释放
    static void SHADER_DELETER(GLuint *p) {
        NSLog(@"Delete shader %d", *p);
        glDeleteShader(*p);
    }
    static void PROGRAM_DELETER(GLuint *p) {
        NSLog(@"Delete program %d", *p);
        glDeleteProgram(*p);
    };
    static void VERTEX_ARRAY_DELETER(GLuint *p) {
        NSLog(@"Delete vertex array %d", *p);
        glDeleteVertexArrays(1, p);
    }
public:
    BaseGLRenderer(std::shared_ptr<GLContext> context) :_context(context) {}
    
    virtual ~BaseGLRenderer() {
        _context->switchContext();
        
        _programId.reset();
        _vertexArrayId.reset();
    }
    
    bool UpdateShader(const std::string &vertexShader, const std::string &fragmentShader) {
        _vertexShaderSource = vertexShader;
        _fragmentShaderSource = fragmentShader;
        _needUpdate = true;
        return true;
    }
    
    bool UpdateTexture(const ImageBuffer &buffer) {
        _textureBuffer = buffer;
        _needUpdate = true;
        return true;
    }
    
//    bool UpdateUniform(const std::string &name, )
    
    void SetClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
        _clearColor[0] = r;
        _clearColor[1] = g;
        _clearColor[2] = b;
        _clearColor[3] = a;
    }
    
    void SetEnableBlend(bool enable) {
        
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
        
        glUseProgram(*_programId); // 启用Shader程序
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textureId);
        
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
        if (_vertexShaderSource.empty() == false || _fragmentShaderSource.empty() == false) {
            // 编译Shader
            GL_IdHolder vertexShaderId = _CompileShader(GL_VERTEX_SHADER, _vertexShaderSource);
            if (vertexShaderId == nullptr)
                return false;
            GL_IdHolder fragmentShaderId = _CompileShader(GL_FRAGMENT_SHADER, _fragmentShaderSource);
            if (fragmentShaderId == nullptr)
                return false;
            
            // 链接Shader为Program。和CPU程序很类似，编译.o文件、链接为可执行文件。【耗时非常长】
            GLuint shaderProgram;
            shaderProgram = glCreateProgram();
            glAttachShader(shaderProgram, *vertexShaderId); // 绑定shader
            glAttachShader(shaderProgram, *fragmentShaderId);
            glLinkProgram(shaderProgram); // 链接Shader为完整着色器程序
            
            // 检查Program是否链接成功
            int success;
            glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success); // 检查编译是否成功
            if (!success) {
                char buf[512];
                glGetProgramInfoLog(shaderProgram, sizeof(buf), NULL, buf);
                NSLog(@"%s", buf);
                return false;
            } else {
                _programId = GL_IdHolder(new GLuint(shaderProgram), PROGRAM_DELETER);
            }
        }
        
        // 创建Vertex Array Object(VAO)。后续所有顶点操作都会储存到VAO中。OpenGL core模式下VAO必须要有。
        GLuint vertexArrayId;
        glGenVertexArrays(1, &vertexArrayId); // 生成顶点Array对象。【必须在创建Buffer前】
        glBindVertexArray(vertexArrayId); // 绑定顶点Array
        
        // Vertex Buffer Object(VBO)
        GLuint vertexBufId;
        GLfloat vertexBuf[] = {
            // --位置-- // --- 颜色 --- // --纹理--
            -0.75, 0.75, 1.0, 0.0, 0.0, 0.0, 1.0, // 左上
             0.75, 0.75, 0.0, 1.0, 0.0, 1.0, 1.0, // 右上
            -0.75,-0.75, 0.0, 0.0, 1.0, 0.0, 0.0, // 左下
             0.75,-0.75, 1.0, 0.0, 0.0, 1.0, 0.0, // 右下
        };
        glGenBuffers(1, &vertexBufId); // 生成 1 个顶点缓冲区对象，vertexBufId是绑定的唯一OpenGL标识
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufId); // 绑定为GL_ARRAY_BUFFER
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBuf), vertexBuf, GL_STATIC_DRAW); // 传输数据
        
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (GLvoid *)(0));// 位置
        glEnableVertexAttribArray(0); // 启用VertexAttribArray
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (GLvoid *)(2 * sizeof(float)));// 颜色
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (GLvoid *)(5 * sizeof(float)));// 纹理
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
            
            glUseProgram(*_programId);
            GLint uniLocation = glGetUniformLocation(*_programId, "texture1");
            glUniform1i(uniLocation, 0);
        }
        
        
        // 这一行的作用是解除vertexBufId的激活状态，避免其它操作不小心改动到这里。不过这种情况很少见。
        glBindVertexArray(0);
        if (CheckError())
            return false;


        _needUpdate = false;
        return true;
    }
    
    /// 编译Shader
    virtual GL_IdHolder _CompileShader(GLenum shaderType, const std::string &source) {
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
            return GL_IdHolder(nullptr, SHADER_DELETER);
        } else {
            return GL_IdHolder(new GLuint(shaderId), SHADER_DELETER);
        }
    }
    
    static bool _CheckGLError(const char *file, int line) {
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
    
protected:
    const std::shared_ptr<GLContext> _context;
    bool _needUpdate = true;
    
    std::string _vertexShaderSource;
    std::string _fragmentShaderSource;
    ImageBuffer _textureBuffer;
    
    GL_IdHolder _programId = GL_IdHolder(nullptr, PROGRAM_DELETER);
    GL_IdHolder _vertexArrayId = GL_IdHolder(nullptr, VERTEX_ARRAY_DELETER);
    GLuint _textureId;
        
    std::array<GLfloat, 4> _clearColor;
};
}

#endif /* GLRendererBase_hpp */
