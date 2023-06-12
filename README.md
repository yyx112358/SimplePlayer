# SimplePlayer
简易跨平台播放器


# 2023年5月30日

目标：通过实现一个自己的播放器，系统性学习音视频相关知识。近期路线图：
- [ ] 实现一个预览器，可将FBO通过OpenGL预览上屏。
- [ ] 实现一个解码器，使用FFMpeg解码音视频。
- [ ] 将解码的帧上屏
- [ ] 音画同步
- [ ] 支持暂停、播放、停止、重播
- [ ] 支持多段视频
- [ ] 支持图文混排
- [ ] 实现合成导出
- [ ] 使用cmake建立工程。拆分独立音视频SDK，UI各端实现。暂定实现Mac、iOS、Win，实现无缝开发衔接。
- [ ] 回顾总结，以待重构 

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

# 2023年6月3日

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

# 2023年6月4日
## 使用Shader绘制三角形
![OpenGL基本渲染链路](https://learnopengl-cn.github.io/img/01/04/pipeline.png)
| 名称 | 功能 |
| --- | ---|
| **Vertex Shader** 顶点着色器 | 坐标变换和基本顶点属性处理，**每个顶点执行一次** |
| Primitive Assembly 图元装配 | 顶点组装为多边形 |
| Geometry Shader 几何着色器|由已有顶点生成新的顶点|
| Rasterization 光栅化|图元映射为像素，生成片段Fragment|
| **Fragment Shader** 片段着色器|计算像素颜色，**每像素执行一次**|
| Alpha Test and Blending α测试和混合|深度测试，丢弃被遮挡物体；并根据alpha透明度进行混合|


Vertex, Fragment是必须的。


![标准化设备坐标(Normalized Device Coordinates, NDC)](https://learnopengl-cn.github.io/img/01/04/ndc.png)<br>
更详细的坐标系介绍：[openGL中的坐标系](https://www.jianshu.com/p/f6820de32557)

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
1. 需要`#import <OpenGL/gl3.h>`而不是`#import <OpenGL/gl.h>`
2. 初始化`NSOpenGLPixelFormat`时，需要显式指定OpenGL 3.2 即 `NSOpenGLProfileVersion3_2Core`
3. Shader需要指定`#version 330 core`
4. 最重要的，构造VBO时候，需要先创建并绑定VertexArray，再创建Vertex Buffer，这与[最好的OpenGL教程之一(B站视频)](https://www.bilibili.com/video/BV1MJ411u7Bc?p=3&spm_id_from=pageDriver&vd_source=486f641ca2720afac8d75c2261136b11)所述不完全一致
5. 另外，勤用debug函数：`glGetShaderiv`、`glGetProgramiv`、`glGetError()`。[一些教程](https://learnopengl.com/#!In-Practice/Debugging)

对这些配置的个人精简理解：
- Shader：GPU上运行的程序，也需要编译、链接成Program才能用
- Vertex：顶点，除了位置属性外，还可以有颜色、纹理、法线，甚至速度向量等属性。
- Vertex Shader：每个顶点执行一次的程序
- Fragment Shader：每个像素执行一次的程序
- Vertex Buffer Object：顶点属性对象数组，需要glVertexAttribPointer告诉GPU解析方式
![](https://learnopengl-cn.github.io/img/01/04/vertex_attribute_pointer.png)
- Vertex Array Object：存储多个顶点属性的数组，便于切换不同顶点数据和属性配置（不是太理解）
![](https://learnopengl-cn.github.io/img/01/04/vertex_array_objects_ebo.png)
- 以前学的时候没想过，现在想来，之所以要使用缓冲区这样的东西，是为了一次性将所有的数据输入给GPU。这样，可以减少GPU数据传输、提高缓存命中率，操作“多个相同属性数组组成的对象”效率也比“多个对象组成的数组”要高。

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

# 2023年6月9日

这些天事情都好多，只来得及看看视频。今天继续学OpenGL。

今天的主要进展就是将相关的OpenGL抽象为C++类。比较有意思的是，使用自定义deleter的unique_ptr封装了OpenGL的id：

``` 
// ===== 定义 =====
typedef std::unique_ptr<GLuint, void(*)(GLuint *)> GL_IdHolder; // 持有GL ID的unique_ptr，支持自动释放
static void SHADER_DELETER(GLuint *p) {
    NSLog(@"Delete shader %d", *p);
    glDeleteShader(*p);
}
static void PROGRAM_DELETER(GLuint *p) {
    NSLog(@"Delete program %d", *p);
    glDeleteProgram(*p);
};

// ===== 使用 =====
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
```
不过，多线程、多Context时候，会不会导致还没切换到对应Context的时候就释放了呢？以后再研究吧。<br>
另外，考虑到EBO、坐标变化、uniform还没学习，C++类还没完全抽出来。

# 2023年6月11日

## Element Buffer Object
Element Buffer Object 相当于是一个索引列表，每一个值相当于是特定顶点的坐标。之所以加入EBO，是因为在复杂模型中，**每一个顶点可能会被多个三角形使用**（比如对一个两个三角形拼成的矩形，就有两个顶点是共用的）。复杂程序中，每一个顶点可能有多个属性，如果不能复用的话，会占用大量不必要的的内存和带宽。<br>
EBO也是顶点属性的一种，配置方式与Vertex Buffer差不多。要点是：
1. 使用前绑定VAO
2. Buffer类型为`GL_ELEMENT_ARRAY_BUFFER`
3. 使用`glDrawElements`替代`glDrawArrays`绘制

这里直接摘抄learnOpenGL代码。
```
// ..:: 初始化代码 :: ..
// 1. 绑定顶点数组对象
glBindVertexArray(VAO);
// 2. 把我们的顶点数组复制到一个顶点缓冲中，供OpenGL使用
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
// 3. 复制我们的索引数组到一个索引缓冲中，供OpenGL使用
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
// 4. 设定顶点属性指针
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

[...]

// ..:: 绘制代码（渲染循环中） :: ..
glUseProgram(shaderProgram);
glBindVertexArray(VAO);
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
glBindVertexArray(0);
```

考虑到视频播放器并不需要太多的三角形，添加EBO还会增加复杂性，这里仅供学习，后续可能并不会使用。

## 《你好，三角形》章节练习题
[第一节练习题](https://learnopengl-cn.github.io/01%20Getting%20started/04%20Hello%20Triangle/)
> 添加更多顶点到数据中，使用glDrawArrays，尝试绘制两个彼此相连的三角形：[参考解答](https://learnopengl.com/code_viewer_gh.php?code=src/1.getting_started/2.3.hello_triangle_exercise1/hello_triangle_exercise1.cpp)
> 创建相同的两个三角形，但对它们的数据使用不同的VAO和VBO：[参考解答](https://learnopengl.com/code_viewer_gh.php?code=src/1.getting_started/2.4.hello_triangle_exercise2/hello_triangle_exercise2.cpp)
> 创建两个着色器程序，第二个程序使用一个不同的片段着色器，输出黄色；再次绘制这两个三角形，让其中一个输出为黄色：[参考解答](https://learnopengl.com/code_viewer_gh.php?code=src/1.getting_started/2.5.hello_triangle_exercise3/hello_triangle_exercise3.cpp)

这个题目还是还是很简单的。把旧代码复制一遍就可以了。关键点有两个：
1. 所有顶点数据使用Vertex Array Object存储
2. 每一帧渲染前，调用`glBindVertexArray()`和`glUseProgram()`绑定一下

漫长的第一节结束，继续！

## GLSL语言 [LearnOpenGL：着色器](https://learnopengl-cn.github.io/01%20Getting%20started/05%20Shaders/)

运行于GPU的程序。基本结构：
```
#version version_number
in type in_variable_name;
in type in_variable_name;

out type out_variable_name;

uniform type uniform_name;

int main()
{
  // 处理输入并进行一些图形操作
  ...
  // 输出处理过的结果到输出变量
  out_variable_name = weird_stuff_we_processed;
}
```

知识点：
- GLSL必须指定版本，随后是输入、输出、uniform变量。函数入口为main()
- 顶点着色器输入数量有上限，至少16个。通过`glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes)`查看
- 数据类型：
  - `vecn` | `bvecn` | `ivecn` | `uvecn` | `dvecn`，表示长度为n的特定类型向量。一般用`vecn`，即float类型。
  - 重组：使用xyzw的组合创建新的变量。例如：
    ```
    vec2 someVec;
    vec4 differentVec = someVec.xyxx;
    vec3 anotherVec = differentVec.zyw;
    vec4 otherVec = someVec.xxxx + anotherVec.yxzy;
    vec4 assignVec = (someVec.xy, 1.0, 1.0);
    ```
- 输入输出 
  - 关键字：`in` | `out`
  - Vertex Shader：**需要特别指定输入变量布局** `layout (location = 0)`
  - Fragment Shader：**需要特别指定`vec4`颜色输出变量**，否则输出黑/白
  - 链接方式：Shader1 的**输出**与Shader2 的**输入****名称和类型一样**
- Uniform
  - 同一个Shader Program内，各个Shader间通用的**全局变量**
  - 声明：任意Shader内 `uniform uniType uniName;`
  - 更新：
    ```
    // 获取uniform位置
    int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor"); 
    // 切换program
    glUseProgram(shaderProgram); 
    // 更新uniform
    glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f); 
    ```

## 《着色器》章节练习题
> 1. 修改顶点着色器让三角形上下颠倒：[参考解答](https://learnopengl.com/code_viewer.php?code=getting-started/shaders-exercise1)
> 2. 使用uniform定义一个水平偏移量，在顶点着色器中使用这个偏移量把三角形移动到屏幕右侧：[参考解答](https://learnopengl.com/code_viewer.php?code=getting-started/shaders-exercise2)
> 3. 使用out关键字把顶点位置输出到片段着色器，并将片段的颜色设置为与顶点位置相等（来看看连顶点位置值都在三角形中被插值的结果）。做完这些后，尝试回答下面的问题：为什么在三角形的左下角是黑的?：[参考解答](https://learnopengl.com/code_viewer.php?code=getting-started/shaders-exercise3)

解答：
1. 顶点着色器的y坐标取反即可：`gl_Position = vec4(aPos.x, -aPos.y, 0.0, 1.0);`
2. 使用uniform改变x坐标即可，注意归一化：<br>
    渲染循环：
    ```
    int location = glGetUniformLocation(*_programId, "xOffset");
    glUniform1f(location, 0.5f);
    ```
    片段着色器
    ```
    #version 330 core
    layout (location = 0) in vec2 aPos;
    uniform float xOffset;

    void main()
    {
        gl_Position = vec4(aPos.x + xOffset, aPos.y, 0.0, 1.0);
    }
    ```
3. 顶点着色器输出位置参数，片段着色器使用位置参数作为坐标即可。
    顶点着色器：
    ```
    #version 330 core
    layout (location = 0) in vec2 aPos;        
    out vec3 vtxColor;
    void main()
    {
        gl_Position = vec4(aPos.xy, 0.0, 1.0);
        vtxColor = gl_Position.xyz;
    }
    ```
    片段着色器：
    ```
    #version 330 core
    in vec3  vtxColor;
    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(vtxColor, 1.0);
    }
    ```
   左下角黑色是因为左下角顶点x、y坐标是负数，因此r<0、g<0、b=0，显示为黑色


## 纹理
终于到了纹理了。纹理可是核心要点。

知识点：
- 坐标系：从左到右，X轴 [0,1]，从下到上，Y轴[0,1]<br>
  ![](https://learnopengl-cn.github.io/img/01/06/tex_coords.png)
- 纹理环绕方式：
  ![](https://learnopengl-cn.github.io/img/01/06/texture_wrapping.png)
  - 默认是GL_REPEAT。修改代码：
    ```
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    ```
  - 如果是`GL_CLAMP_TO_BORDER`，可以指定边缘颜色：
    ```
    float borderColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    ```
- 纹理过滤：缩放方式，常用最近邻采样`GL_NEAREST`和线性插值`GL_LINEAR`。配置方式：
- 多级渐远纹理mipmap：LOD，实际上就是在不同距离上采用不同精度的纹理。对音视频播放没什么用。
- 纹理单元
  - 用于标注当前使用纹理的位置。我的理解是GPU内可以同时存在成百上千个纹理，但是GPU同一时间内能处理的纹理数量是有限的，因此需要**将当前要用的纹理绑定到纹理单元上**。
  - OpenGL至少有16个纹理单元，GL_TEXTURE0 ~ GL_TEXTURE15。其中GL_Texture0是默认激活的
  - 使用glUniform1i()传递参数。
  - 使用方式：<br>
    GLSL中，纹理单元表示为一种特殊的，类型为sampler2D的uniform变量：
    ```
    #version 330 core
    in vec2  vtxTexCoord;
        
    out vec4 FragColor;
        
    uniform sampler2D texture1; // 纹理单元

    void main()
    {
        // 使用texture1进行颜色采样。
        // 因为纹理坐标系和OpenGL坐标系相反，因此y坐标取1-vtxTexCoord.y
        FragColor = texture(texture1, vec2(vtxTexCoord.x, 1-vtxTexCoord.y));
    }
    ```
    初始化时：
    ```
    //生成并绑定纹理
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    // ... 配置纹理参数 ...
    ```
    渲染循环
    ```
    glUseProgram(programId);

    glActiveTexture(GL_TEXTURE0 + 1); // 激活纹理单元1
    glBindTexture(GL_TEXTURE_2D, _textureId); // 绑定纹理，根据上下文，绑定到了纹理单元1
    glUniform1i(glGetUniformLocation(programId, "texture1"), 1); // 将纹理单元传递给uniform

    // ... 渲染多边形 ...
    ```

# 2023年6月12日
关键部分通过了，剩下的快速通过。

## [LearnOpenGL：变换](https://learnopengl-cn.github.io/01%20Getting%20started/07%20Transformations/)

主要是通过矩阵运算实现仿射变换。记录一下基本的矩阵变换，以供记忆。

位移矩阵：
$$
\begin{bmatrix}  \color{red}1 & \color{red}0 & \color{red}0 & \color{red}{T_x} \\ \color{green}0 & \color{green}1 & \color{green}0 & \color{green}{T_y} \\ \color{blue}0 & \color{blue}0 & \color{blue}1 & \color{blue}{T_z} \\ \color{purple}0 & \color{purple}0 & \color{purple}0 & \color{purple}1 \end{bmatrix} \cdot \begin{pmatrix} x \\ y \\ z \\ 1 \end{pmatrix} = \begin{pmatrix} x + \color{red}{T_x} \\ y + \color{green}{T_y} \\ z + \color{blue}{T_z} \\ 1 \end{pmatrix}
$$
缩放矩阵：
$$
\begin{bmatrix} \color{red}{S_1} & \color{red}0 & \color{red}0 & \color{red}0 \\ \color{green}0 & \color{green}{S_2} & \color{green}0 & \color{green}0 \\ \color{blue}0 & \color{blue}0 & \color{blue}{S_3} & \color{blue}0 \\ \color{purple}0 & \color{purple}0 & \color{purple}0 & \color{purple}1 \end{bmatrix} \cdot \begin{pmatrix} x \\ y \\ z \\ 1 \end{pmatrix} = \begin{pmatrix} \color{red}{S_1} \cdot x \\ \color{green}{S_2} \cdot y \\ \color{blue}{S_3} \cdot z \\ 1 \end{pmatrix}
$$
旋转矩阵（绕z轴）：
$$
\begin{bmatrix} \color{red}{\cos \theta} & - \color{red}{\sin \theta} & \color{red}0 & \color{red}0 \\ \color{green}{\sin \theta} & \color{green}{\cos \theta} & \color{green}0 & \color{green}0 \\ \color{blue}0 & \color{blue}0 & \color{blue}1 & \color{blue}0 \\ \color{purple}0 & \color{purple}0 & \color{purple}0 & \color{purple}1 \end{bmatrix} \cdot \begin{pmatrix} x \\ y \\ z \\ 1 \end{pmatrix} = \begin{pmatrix} \color{red}{\cos \theta} \cdot x - \color{red}{\sin \theta} \cdot y  \\ \color{green}{\sin \theta} \cdot x + \color{green}{\cos \theta} \cdot y \\ z \\ 1 \end{pmatrix}
$$
任意轴旋转：
$$
\begin{bmatrix} \cos \theta + \color{red}{R_x}^2(1 - \cos \theta) & \color{red}{R_x}\color{green}{R_y}(1 - \cos \theta) - \color{blue}{R_z} \sin \theta & \color{red}{R_x}\color{blue}{R_z}(1 - \cos \theta) + \color{green}{R_y} \sin \theta & 0 \\ \color{green}{R_y}\color{red}{R_x} (1 - \cos \theta) + \color{blue}{R_z} \sin \theta & \cos \theta + \color{green}{R_y}^2(1 - \cos \theta) & \color{green}{R_y}\color{blue}{R_z}(1 - \cos \theta) - \color{red}{R_x} \sin \theta & 0 \\ \color{blue}{R_z}\color{red}{R_x}(1 - \cos \theta) - \color{green}{R_y} \sin \theta & \color{blue}{R_z}\color{green}{R_y}(1 - \cos \theta) + \color{red}{R_x} \sin \theta & \cos \theta + \color{blue}{R_z}^2(1 - \cos \theta) & 0 \\ 0 & 0 & 0 & 1 \end{bmatrix}
$$

以上矩阵运算均可使用GLM库进行计算。这是一个专门用于OpenGL矩阵变换的header-only库。[Git仓库](https://github.com/g-truc/glm)<br>
获取矩阵之后，就可以使用`glUniformMatrix4fv()`将矩阵作为uniform传递给Vertex Shader了。<br>
矩阵计算代码。需要注意的是，因为glm的vec、mat是模版，为了避免编译器推断类型错误，**最好保证每一个数字都带有后缀'f'**。
```
glUseProgram(*_programId); // 启用Shader程序
        
glm::mat4 trans(1.0f); // 单位矩阵
trans = glm::translate(trans, glm::vec3(0.5f, 0.5f, 0.0f));
trans = glm::rotate(trans, glm::radians(60.0f), glm::vec3(0.0f, 0.0f, 1.0f));
trans = glm::scale(trans, glm::vec3(0.75f, 0.75f, 0.75f));

unsigned int transformLoc = glGetUniformLocation(ourShader.ID, "transform");
glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));
```
Vertex Shader。GLSL直接用乘法就可以进行矩阵计算。
```
#version 330 core
layout (location = 0) in vec2 aPos;
        
uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(aPos.x, aPos.y, 0.0, 1.0);
}
```

## 使用std::any实现任意类型uniform更新

通过std::any、std::type_index以及lambda表达式，可以实现任意类型的glUniformX()调用而不需要写重载。重载效率会高一些，不过写起来太麻烦了。以后正式编写时候应该会使用重载，但是在学习阶段，使用一下无妨。等以后公司完全切到C++17，就可以直接借用了。

```
virtual void _UpdateUniform() {
    // 更新Uniform
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
```
之所以使用map而不用switch，是因为type_info都是运行时数据，编译期无法获取，也就不能使用switch case语句。

## [LearnOpenGL：坐标系统](https://learnopengl-cn.github.io/01%20Getting%20started/08%20Coordinate%20Systems/)

![](https://learnopengl-cn.github.io/img/01/08/coordinate_systems.png)


# 优质参考资料

[LearnOpenGL-CN](https://learnopengl-cn.github.io/)<br>
[OpenGL参考文档docs.GL](https://docs.gl/)
[OPENGL ES 2.0 知识串讲](http://geekfaner.com/shineengine/blog2_OpenGLESv2_1.html)
