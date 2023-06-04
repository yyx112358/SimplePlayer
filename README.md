# SimplePlayer
简易跨平台播放器


# 2023年5月30日

目标：通过实现一个自己的播放器，系统性学习音视频相关知识。近期路线图：
1. 实现一个预览器，可将FBO通过OpenGL预览上屏。
2. 实现一个解码器，使用FFMpeg解码音视频。
3. 将解码的帧上屏
4. 音画同步
5. 支持暂停、播放、停止、重播
6. 支持多段视频
7. 支持图文混排
9. 实现合成导出
9. 使用cmake建立工程。拆分独立音视频SDK，UI各端实现。暂定实现Mac、Win，实现无缝开发衔接。
10. 回顾总结，以待重构 

这一轮主要以快速实现框架为主，不必太在乎代码质量。后续可以在TT中间页切换时候重写

综合ChatGPT、[CMake 良心教程，教你从入门到入魂](https://zhuanlan.zhihu.com/p/500002865)、[全网最细的CMake教程！(强烈建议收藏)](https://zhuanlan.zhihu.com/p/534439206) 编写cmakelist，构造了基本的工程。cmake入门还是很简单的。


# 2023年5月31日

尝试使用cmake生成一个UI程序，比较困难，网上缺少例子。参考GitHub代码：

[Simple iOS Application with iPad and iPhone storyboards.](https://github.com/forexample/testapp)<br>
[CMake Build Configuration for iOS](https://github.com/sheldonth/ios-cmake)

# 2023年6月1日

cmake生成一个MacOS端的GUI程序实在是太困难了，网上没有找到合适的代码，基本都是iOS + cmake的。使用以下代码可以编译，但无法显示UI窗口。目测主要的问题是没有找到方案可以添加并编译Main.storyboard。
``` cmake_minimum_required(VERSION 3.14)
project(SimplePlayer VERSION 0.0.1)

# 设置C++编译器
set(CMAKE_CXX_COMPILER "clang++")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)

# 收集当前目录下的所有cpp文件
file(GLOB SDK_FILES src/*)

# 添加资源文件
set(RESOURCE_FILES
    src/Main.storyboard
)

# 添加可执行文件并将所有cpp文件链接起来
add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SDK_FILES} ${RESOURCE_FILES})

target_sources(${PROJECT_NAME} PRIVATE ${RESOURCE_FILES})

# 添加依赖库
target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
```

纠结再三，还是直接使用Universal程序吧。纠结这种问题实在是既麻烦又没收益，我们的重点还是音视频学习。

# 2023年6月4日

切了一个mac分支，重新建了一个纯Mac的环境，开始学习OpenGL。计划主要学习2D绘图，3D不必深入。主要参考资料：
1. [最好的OpenGL教程之一(B站视频)](https://www.bilibili.com/video/BV1MJ411u7Bc?p=3&spm_id_from=pageDriver&vd_source=486f641ca2720afac8d75c2261136b11)
2. [LearnOpenGL-CN](https://learnopengl-cn.github.io/)
3. [docs.GL](https://docs.gl/)
4. [About OpenGL for OS X | 苹果官方Mac OpenGL文档](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_intro/opengl_intro.html#//apple_ref/doc/uid/TP40001987-CH207-TP9)
5. VEPreviewUnit

## Legacy OpenGL绘制三角形
NSOpenGLView + Legacy OpenGL绘制三角形：
``` 
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

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    // 获取当前OpenGL上下文
    NSOpenGLContext *openGLContext = [self openGLContext];
    [openGLContext makeCurrentContext];
    
    // 清空窗口颜色
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // 设置绘制模式
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // 绘制三角形
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(0, 1);
    glVertex2f(1, 0);
    glVertex2f(-1, 0);
    glEnd();
    
    // 在OpenGL绘制完成后，调用flush方法将绘制的结果显示到窗口上
    [openGLContext flushBuffer];
}
```
需要注意的是，**三角形顶点需要是顺时针**，否则无法显示（逆时针的三角形是反面对镜头）。<br>

## 使用Shader绘制三角形
![OpenGL基本渲染链路](https://learnopengl-cn.github.io/img/01/04/pipeline.png)
- **Vertex Shader** 顶点着色器：坐标变换和基本顶点属性处理，**每个顶点执行一次**
- Primitive Assembly 图元装配：顶点组装为多边形
- Geometry Shader 几何着色器：由已有顶点生成新的顶点
- Rasterization 光栅化：图元映射为像素，生成片段Fragment
- **Fragment Shader** 片段着色器：计算像素颜色，**每像素执行一次**
- Alpha Test and Blending α测试和混合：深度测试，丢弃被遮挡物体；并根据alpha透明度进行混合

Vertex, Fragment是必须的。


![标准化设备坐标(Normalized Device Coordinates, NDC)](https://learnopengl-cn.github.io/img/01/04/ndc.png)<br>
更详细的坐标系介绍：[openGL中的坐标系](https://www.jianshu.com/p/f6820de32557)

以前学的时候没想过，现在想来，之所以要使用缓冲区这样的东西，是为了一次性将所有的数据输入给GPU。这样，可以减少GPU数据传输、提高缓存命中率，操作“多个相同属性数组组成的对象”效率也比“多个对象组成的数组”要高。
使用顶点缓冲区替换Legacy OpenGL三角形绘制：
```
- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    // 获取当前OpenGL上下文
    NSOpenGLContext *openGLContext = [self openGLContext];
    [openGLContext makeCurrentContext];
    
    // 创建并编译 Vertex shader
    /**
     * #version 330 core 显式指定版本
     * layout (location = 0) in vec3 aPos;
     * gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); 位置透传。第四个分量（w）为
     */
    const GLchar *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    })";
    GLuint vertexShaderId;
    vertexShaderId = glCreateShader(GL_VERTEX_SHADER); // 创建并绑定Shader
    glShaderSource(vertexShaderId, 1, &vertexShaderSource, NULL); // 绑定Shader源码
    glCompileShader(vertexShaderId); // 编译Shader
    // 可选，检查编译状态。非常有用
    int success;
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char buf[512];
        glGetShaderInfoLog(vertexShaderId, sizeof(buf), NULL, buf);
        NSLog(@"%s", buf);
    }
    
    // 创建并编译Fragment Shader，方法基本一致
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
    
    // 链接Shader为Program。和CPU程序很类似，编译.o文件、链接为可执行文件。【耗时非常长】
    GLuint shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShaderId); // 绑定shader
    glAttachShader(shaderProgram, fragmentShaderId);
    glLinkProgram(shaderProgram); // 链接Shader为完整着色器程序
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success); // 检查编译是否成功
    if (!success) {
        char buf[512];
        glGetProgramInfoLog(shaderProgram, sizeof(buf), NULL, buf);
        NSLog(@"%s", buf);
    }
    
    // 删除不用的Shader，释放资源
    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);
    
    // 创建Vertex Array Object(VAO)。后续所有顶点操作都会储存到VAO中。OpenGL core模式下VAO必须要有。
    unsigned int vertexArrayId;
    glGenVertexArrays(1, &vertexArrayId); // 生成顶点Array对象。【必须在创建Buffer前】
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
    // stride：步长，每个顶点占用字节数；pointer，该属性在buffer中位置
    // 例如，一个顶点有以下float类型属性：0~2，三维位置；3~4，纹理坐标。则对位置属性，参数：vertexBufId, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0。对纹理属性：vertexBufId, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 3
    // 配置Vertex属性
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufId); // 绑定为GL_ARRAY_BUFFER

    // 这一行的作用是解除vertexBufId的激活状态，避免其它操作不小心改动到这里
    glBindVertexArray(0);
    
    // 上屏绘制
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(shaderProgram); // 启用Shader程序
    glBindVertexArray(vertexArrayId); // 绑定Vertex Array
    glDrawArrays(GL_TRIANGLES, 0, 3); // 绘制三角形
    
    GLenum error = glGetError();
    if (error) {
        NSLog(@"GL error:%d", error);
    }
    
    // 在OpenGL绘制完成后，调用flush方法将绘制的结果显示到窗口上
    [openGLContext flushBuffer];
}
```
一开始怎么都画不出三角形，折腾了很久，查了很多资料，再配合getError、glGetShaderiv等方法，最后发现Mac端有这些要注意的：
1. 需要#import <OpenGL/gl3.h>而不是#import <OpenGL/gl.h>
2. 初始化NSOpenGLPixelFormat时，需要显式指定OpenGL 3.2 即 NSOpenGLProfileVersion3_2Core
3. Shader需要指定#version 330 core
4. 最重要的，构造VBO时候，需要先创建并绑定VertexArray，再创建Vertex Buffer，这与[最好的OpenGL教程之一(B站视频)](https://www.bilibili.com/video/BV1MJ411u7Bc?p=3&spm_id_from=pageDriver&vd_source=486f641ca2720afac8d75c2261136b11)所述不完全一致
5. 另外，勤用debug函数：glGetShaderiv、glGetProgramiv、glGetError()

对这些配置的个人精简理解：
- Shader：GPU上运行的程序，也需要编译、链接成Program才能用
- Vertex：顶点，除了位置属性外，还可以有颜色、纹理、法线，甚至速度向量等属性。
- Vertex Shader：每个顶点执行一次的程序
- Fragment Shader：每个像素执行一次的程序
- Vertex Buffer Object：顶点属性对象数组，需要glVertexAttribPointer告诉GPU解析方式
![](https://learnopengl-cn.github.io/img/01/04/vertex_attribute_pointer.png)
- Vertex Array Object：存储多个顶点属性的数组，便于切换不同顶点数据和属性配置（不是太理解）
![](https://learnopengl-cn.github.io/img/01/04/vertex_array_objects.png)

OpenGL ES的Shader关键字有变化[StackOverflow:OpenGL shader builder errors on compiling](https://stackoverflow.com/questions/24737705/opengl-shader-builder-errors-on-compiling)：<br>
```
顶点着色器
in --> attribute
out --> varying
片段着色器
in --> varying
out --> (delete)
```

其它参考文档：
- [各个主流平台基本OpenGL操作](https://segmentfault.com/a/1190000040018969)
- [Learning OpenGL(ES) —— OpenGL Model, Pipeline and Practices](https://niyaoyao.github.io/2018/05/23/learning_opengl%28es%29_opengl_model_pipeline_and_practices/)

2D图形绘制，后面会用：
- [【译】OpenGL 教程：二维图形绘制](https://zhuanlan.zhihu.com/p/103925920)
- [音视频-OpenGL ES渲染视频图像](https://www.jianshu.com/p/37d8132a5d4c)
- [OpenGL ES总结（四）OpenGL 渲染视频画面](https://blog.51cto.com/u_15069450/2934927)
