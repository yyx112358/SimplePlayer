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
2. VEPreviewUnit

## Legacy OpenGL绘制三角形
NSOpenGLView + Legacy OpenGL绘制三角形：
``` 
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

![标准化设备坐标(Normalized Device Coordinates, NDC)](https://learnopengl-cn.github.io/img/01/04/ndc.png)
使用顶点缓冲区替换Legacy OpenGL三角形绘制：
```

```
需要注意的是，OpenGL ES的Shader关键字有变化[StackOverflow:OpenGL shader builder errors on compiling](https://stackoverflow.com/questions/24737705/opengl-shader-builder-errors-on-compiling)：<br>
```
顶点着色器
in --> attribute
out --> varying
片段着色器
in --> varying
out --> (delete)
```

以前学的时候没想过，现在想来，之所以要使用缓冲区这样的东西，是为了一次性将所有的数据输入给GPU。这样，可以减少GPU数据传输、提高缓存命中率，操作“多个相同属性数组组成的对象”效率也比“多个对象组成的数组”要高。
