//
//  PreviewManager_Mac.m
//  SimplePlayer
//
//  Created by YangYixuan on 2023/5/29.
//

#import <Foundation/Foundation.h>
#include "PreviewManager_Mac.h"
#include <string>
#include <optional>
#include <array>
#include <any>

#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#include "GLRendererBase.hpp"

@interface Preview_Mac : NSOpenGLView {
    std::shared_ptr<sp::GLContext> pGLContext;
    std::unique_ptr<sp::BaseGLRenderer> pRenderer;
}

@end

@implementation Preview_Mac

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        // 创建并初始化GLContext
        pGLContext = std::make_shared<sp::GLContext>();
        if (pGLContext->init() == false)
            return nil;
        [self setOpenGLContext:pGLContext->context()];
        
        // 创建并初始化Renderer
        pRenderer = std::make_unique<sp::BaseGLRenderer>(pGLContext);
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

        // 创建并编译Fragment Shader，方法基本一致
        const GLchar *fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(1.0, 0.5, 0.2, 1.0);
        })";
        pRenderer->UpdateShader(vertexShaderSource, fragmentShaderSource);
        pRenderer->SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        
    }
    return self;
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize {
    [self drawRect:self.superview.bounds];
}

- (void)drawRect:(NSRect)dirtyRect {
    NSLog(@"%@", NSStringFromRect(dirtyRect));
    auto tm = clock();
    [super drawRect:dirtyRect];
    
    pRenderer->Render();
    pGLContext->flush();

    NSLog(@"耗时：%.2fms", (clock() - tm) / 1000.0f);
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
