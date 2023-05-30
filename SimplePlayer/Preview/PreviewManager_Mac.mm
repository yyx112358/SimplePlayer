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
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>


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
            0
        };
        
        NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
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
    glOrtho(0, dirtyRect.size.width, 0, dirtyRect.size.height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // 绘制一个简单的三角形
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(dirtyRect.size.width / 2, 0);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex2f(dirtyRect.size.width, dirtyRect.size.height);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex2f(0, dirtyRect.size.height);
    glEnd();
    
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
