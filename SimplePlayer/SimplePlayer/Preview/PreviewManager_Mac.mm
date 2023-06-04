//
//  PreviewManager_Mac.m
//  SimplePlayer
//
//  Created by YangYixuan on 2023/5/29.
//

#import <Foundation/Foundation.h>
#include "PreviewManager_Mac.h"

#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
//#import <OpenGL/glu.h>


@interface Preview_Mac : NSOpenGLView

@end

@implementation Preview_Mac

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        NSOpenGLPixelFormatAttribute attrs[] =
        {
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFADepthSize, 24,
            NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,  // 【声明使用OpenGL3.2】，不配置则默认OpenGL 2
            0
        };
        
        NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        super.pixelFormat = pixelFormat;
        NSOpenGLContext *openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
        
        [self setOpenGLContext:openGLContext];
    }
    return self;
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize {
    [self drawRect:self.superview.bounds];
}

- (void)drawRect:(NSRect)dirtyRect {
    NSLog(@"%@", NSStringFromRect(dirtyRect));
    [super drawRect:dirtyRect];
    
    // 获取当前OpenGL上下文
    NSOpenGLContext *openGLContext = [self openGLContext];
    [openGLContext makeCurrentContext];
    

    
    // 设置绘制模式
//    glMatrixMode(GL_PROJECTION);
//    glLoadIdentity();
//    glOrtho(0, dirtyRect.size.width, 0, dirtyRect.size.height, -1, 1);
//    glMatrixMode(GL_MODELVIEW);
//    glLoadIdentity();
    


    // Vertex shader
    const GLchar *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    })";
    GLuint vertexShaderId;
    vertexShaderId = glCreateShader(GL_VERTEX_SHADER); // 创建并绑定Shader
    glShaderSource(vertexShaderId, 1, &vertexShaderSource, NULL); // 附着Shader源码
    glCompileShader(vertexShaderId); // 编译Shader
    // 可选，检查编译状态
    int success;
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char buf[512];
        glGetShaderInfoLog(vertexShaderId, sizeof(buf), NULL, buf);
        NSLog(@"%s", buf);
    }
    
    // Fragment Shader
    const GLchar *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(1.0, 0.5, 0.2, 1.0);
    })";
    
    GLuint fragmentShaderId;
    fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderId, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShaderId);
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char buf[512];
        glGetShaderInfoLog(fragmentShaderId, sizeof(buf), NULL, buf);
        NSLog(@"%s", buf);
    }
    
    // 链接Shader为Program
    GLuint shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShaderId);
    glAttachShader(shaderProgram, fragmentShaderId);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char buf[512];
        glGetProgramInfoLog(shaderProgram, sizeof(buf), NULL, buf);
        NSLog(@"%s", buf);
    }
    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);
    
    // 创建Vertex
    unsigned int vertexArrayId;
    glGenVertexArrays(1, &vertexArrayId); // 生成顶点Array对象。【必须在
    glBindVertexArray(vertexArrayId); // 绑定顶点Array
    
    // Vertex Buffer Object(VBO)
    GLuint vertexBufId;
    GLfloat vertexBuf[] = {
        -0.5,-0.5,
         0.0, 0.5,
         0.5,-0.5,
    };
    glGenBuffers(1, &vertexBufId); // 生成 1 个顶点缓冲区对象，vertexBufId是绑定的唯一OpenGL标识
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufId); // 绑定为GL_ARRAY_BUFFER
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBuf), vertexBuf, GL_STATIC_DRAW); // 第四个usage参数参考https://docs.gl/es3/glBufferData，GL_STATIC_DRAW意为：一次修改频繁使用 + 上层修改而GL做绘制
    // 告诉GPU，Buffer内部结构。参考：https://docs.gl/es3/glVertexAttribPointer
    // index: Buffer标识；size：对应顶点属性分量个数，范围1~4，二维位置为2，三维位置为3；normalized，是否归一化
    // stride：步长，每个顶点占用字节数；pointer，非空时表示该属性在buffer中位置
    // 例如，一个顶点有以下float类型属性：0~2，三维位置；3~4，纹理坐标。则对位置属性，参数：vertexBufId, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0。对纹理属性：vertexBufId, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 3
    // 配置Vertex属性
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufId); // 绑定为GL_ARRAY_BUFFER

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);
    
    // 清屏
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    
    glUseProgram(shaderProgram);
    glBindVertexArray(vertexArrayId); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glDrawArrays(GL_TRIANGLES, 0, 3);

    
    // 在OpenGL绘制完成后，调用flush方法将绘制的结果显示到窗口上
    [openGLContext flushBuffer];
}

@end


bool PreviewManager_Mac::setParentViews(void *parents) {
    NSView *superView = (__bridge NSView *)parents;
    Preview_Mac * preview = [[Preview_Mac alloc] initWithFrame:superView.bounds];
    [superView addSubview:preview];
    
    return true;
}

bool PreviewManager_Mac::render(void *data, int width, int height) {
    
    return true;
}

std::shared_ptr<IPreviewManager> IPreviewManager::createIPreviewManager() {
    return std::make_shared<PreviewManager_Mac>();
}
