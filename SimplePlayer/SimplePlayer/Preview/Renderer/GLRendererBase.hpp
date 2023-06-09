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
        NSLog(@"Delete shader %d", *p);
        glDeleteVertexArrays(1, p);
    }
public:
    BaseGLRenderer(std::shared_ptr<GLContext> context) :_context(context) {}
    
    virtual ~BaseGLRenderer() {
        _context->switchContext();
        
        _programId.reset();
    }
    
    bool UpdateShader(const std::string &vertexShader, const std::string &fragmentShader) {
        if (vertexShader != _vertexShaderSource && fragmentShader != _fragmentShaderSource) {
            _vertexShaderSource = vertexShader;
            _fragmentShaderSource = fragmentShader;
            _needUpdate = true;
        }
        return true;
    }
    
    virtual bool Render() {
        // 获取当前OpenGL上下文
        if (_context == nullptr || _context->switchContext() == false)
            return false;
        // 更新内部参数
        if (_InternalUpdate() == false)
            return false;
        
        // 设置绘制模式
    //    glMatrixMode(GL_PROJECTION);
    //    glLoadIdentity();
    //    glOrtho(0, dirtyRect.size.width, 0, dirtyRect.size.height, -1, 1);
    //    glMatrixMode(GL_MODELVIEW);
    //    glLoadIdentity();
        
        // 创建Vertex Array Object(VAO)。后续所有顶点操作都会储存到VAO中。OpenGL core模式下VAO必须要有。
        GLuint vertexArrayId;
        glGenVertexArrays(1, &vertexArrayId); // 生成顶点Array对象。【必须在创建Buffer前】
        glBindVertexArray(vertexArrayId); // 绑定顶点Array
        
        // Vertex Buffer Object(VBO)
        GLuint vertexBufId;
        GLfloat vertexBuf[] = {
            -1.0, 1.0,
             1.0, 1.0,
            -1.0,-1.0,
            
            -0.0,-1.0,
             1.0,-1.0,
             1.0, 1.0,
        };
        glGenBuffers(1, &vertexBufId); // 生成 1 个顶点缓冲区对象，vertexBufId是绑定的唯一OpenGL标识
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufId); // 绑定为GL_ARRAY_BUFFER
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBuf), vertexBuf, GL_STATIC_DRAW); // 第四个usage参数参考https://docs.gl/es3/glBufferData，GL_STATIC_DRAW意为：一次修改频繁使用 + 上层修改而GL做绘制
        // 告诉GPU，Buffer内部结构。参考：https://docs.gl/es3/glVertexAttribPointer
        // index: Buffer标识；size：对应顶点属性分量个数，范围1~4，二维位置为2，三维位置为3；normalized，是否归一化
        // stride：步长，每个顶点占用字节数；pointer，该属性在buffer中位置
        // 例如，一个顶点有以下float类型属性：0~2，三维位置；3~4，纹理坐标。则对位置属性，参数：vertexBufId, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0。对纹理属性：vertexBufId, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 3
        // 配置Vertex属性
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBufId); // 绑定为GL_ARRAY_BUFFER

        // 这一行的作用是解除vertexBufId的激活状态，避免其它操作不小心改动到这里
        glBindVertexArray(0);
        
        if (_CheckGLError())
            return false;
        
        // 上屏绘制
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(*_programId); // 启用Shader程序
        glBindVertexArray(vertexArrayId); // 绑定Vertex Array
        glDrawArrays(GL_TRIANGLES, 0, 6); // 绘制三角形
        
        if (_CheckGLError())
            return false;
        
        return true;
    }
    
protected:
    virtual bool _InternalUpdate() {
        if (_needUpdate == false)
            return true;
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
    
    bool _CheckGLError() {
        GLenum error = glGetError();
        if (error) {
            NSLog(@"GL error:%d", error);
        }
        return error != 0;
    }
    
protected:
    const std::shared_ptr<GLContext> _context;
    bool _needUpdate = true;
    
    std::string _vertexShaderSource;
    std::string _fragmentShaderSource;
        
    GL_IdHolder _programId = GL_IdHolder(nullptr, PROGRAM_DELETER);
};
}

#endif /* GLRendererBase_hpp */
