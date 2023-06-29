//
//  PreviewManager_Mac.m
//  SimplePlayer
//
//  Created by YangYixuan on 2023/5/29.
//

#import <Foundation/Foundation.h>
#import <AppKit/NSImage.h>
#include "PreviewManager_Mac.h"
#include <string>
#include <optional>
#include <array>
#include <any>


#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#include "GLRendererBase.hpp"
#include "ImageReader.hpp"

std::optional<sp::ImageBuffer> LoadBufferFromImage(NSImage *image) {
    std::optional<sp::ImageBuffer> result;
    if (image == nil)
        return result;
    // 转CGImage
    CGImageRef cgImg = [image CGImageForProposedRect:nil context:nil hints:nil];
    if (cgImg == nil)
        return result;
    // 转CVPixelBuffer
    size_t width = CGImageGetWidth(cgImg), height = CGImageGetHeight(cgImg);
    CFDictionaryRef empty = CFDictionaryCreate(kCFAllocatorDefault, NULL, NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks); // our empty IOSurface properties dictionary
    NSDictionary *options = @{
//#ifdef __MAC__
        (NSString *)kCVPixelBufferOpenGLCompatibilityKey: @YES,
//#else
//        (NSString *)kCVPixelBufferOpenGLESCompatibilityKey: @YES,
//#endif
        (NSString *)kCVPixelBufferIOSurfacePropertiesKey: (__bridge NSDictionary *)empty,
    };
    CVPixelBufferRef pixelBuf = nil;
    CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, width, height, kCVPixelFormatType_32BGRA, (__bridge CFDictionaryRef)options, &pixelBuf);
    CFRelease(empty);
    if (status != kCVReturnSuccess)
        return result;
    
    CVPixelBufferLockBaseAddress(pixelBuf, 0);
    void *pxdata = CVPixelBufferGetBaseAddress(pixelBuf);
    
    CGContextRef context          = CGBitmapContextCreate(pxdata, width, height, 8, CVPixelBufferGetBytesPerRow(pixelBuf), CGColorSpaceCreateDeviceRGB(), kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
    if (status != kCVReturnSuccess)
        return result;
    
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImg);
    CGContextRelease(context);
    CVPixelBufferUnlockBaseAddress(pixelBuf, 0);
    
    // 转data
    sp::ImageBuffer imageBuffer;
    imageBuffer.width = (GLsizei)width;
    imageBuffer.height = (GLsizei)height;
    imageBuffer.data = std::shared_ptr<uint8_t[]>(new uint8_t[width * height * 4]);
    size_t planeCount = CVPixelBufferGetPlaneCount(pixelBuf);
    OSType format = CVPixelBufferGetPixelFormatType(pixelBuf);
    
    planeCount = planeCount <= 0 ? 1 : planeCount;
    CVPixelBufferLockBaseAddress(pixelBuf, kCVPixelBufferLock_ReadOnly);
    for(int i = 0;i < planeCount;i ++){
        uint8_t *praw = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(pixelBuf, i);
        uint64_t height = CVPixelBufferGetHeightOfPlane(pixelBuf, i);
        uint64_t width = CVPixelBufferGetWidthOfPlane(pixelBuf, i);
        uint64_t alignedBytePerRow = CVPixelBufferGetBytesPerRowOfPlane(pixelBuf, i);
        uint64_t noAlignBytePerRow = width * 4;
        uint64_t size = 0;
        if(!praw ||
           noAlignBytePerRow * height < std::min(noAlignBytePerRow, height) ||
           noAlignBytePerRow > alignedBytePerRow){
            return result;
        }
        
        if(noAlignBytePerRow == alignedBytePerRow){
            size = alignedBytePerRow * height;
            memcpy(imageBuffer.data.get(), praw, size);
        }else{
            for(int64_t j = 0;j < height;j ++){
                memcpy(imageBuffer.data.get() + size, praw + j * alignedBytePerRow, noAlignBytePerRow);
                size += noAlignBytePerRow;
            }
        }
    }
    CVPixelBufferUnlockBaseAddress(pixelBuf, kCVPixelBufferLock_ReadOnly);
    
    // 反转R、B通道
    if (format == kCVPixelFormatType_32BGRA) {
        uint8_t *target = imageBuffer.data.get();
        for (unsigned int row = 0; row < height; ++row) {
            uint8_t *ptrDst = target;
            for (unsigned int col = 0; col < width; ++col) {
                std::swap(ptrDst[0], ptrDst[2]);
                ptrDst += 4;
            }
            target += width * 4;
        }
    }
    
    result = imageBuffer;
    return result;
}


@interface Preview_Mac : NSOpenGLView {
    std::shared_ptr<sp::GLContext> pGLContext;
    std::unique_ptr<sp::GLRendererBase> pRenderer;
    std::unique_ptr<sp::GLRendererPreview> pRendererPreview;
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
        pRenderer = std::make_unique<sp::GLRendererBase>(pGLContext);
        // 创建并编译 Vertex shader
        /**
         * #version 330 core 显式指定版本
         * layout (location = 0) in vec3 aPos;
         * gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); 位置透传。第四个分量（w）为
         */
        const GLchar *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        
        uniform mat4 transform;
        
        out vec3 vtxColor;
        out vec2 vtxTexCoord;

        void main()
        {
            gl_Position = transform * vec4(aPos.x, aPos.y, 0.0, 1.0);
            vtxColor = gl_Position.xyz;
            vtxTexCoord = aTexCoord;
        })";

        // 创建并编译Fragment Shader，方法基本一致
        const GLchar *fragmentShaderSource = R"(
        #version 330 core
        in vec3  vtxColor;
        in vec2  vtxTexCoord;
        
        out vec4 FragColor;
        
        uniform sampler2D texture0;

        void main()
        {
            // 使用texture1进行颜色采样。
            // 纹理坐标系和OpenGL坐标系相反，因此y坐标取1-vtxTexCoord.y
            FragColor = texture(texture0, vec2(vtxTexCoord.x, 1-vtxTexCoord.y)).rgba;
        })";
        pRenderer->UpdateShader({vertexShaderSource}, {fragmentShaderSource});
        pRenderer->SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        
        
        // initWithContentsOfFile读不出来。不深究了，直接用imageNamed
        //NSImage *image = [[NSImage alloc] initWithContentsOfFile:@"/Users/yangyixuan/Downloads/texture.jpg"];
        auto buffer = LoadBufferFromImage([NSImage imageNamed:@"texture.jpg"]);
        if (buffer.has_value())
            pRenderer->UpdateTexture({*buffer});
        
        pRendererPreview = std::make_unique<sp::GLRendererPreview>(pGLContext);
        pRendererPreview->SetClearColor(0.75f, 0.5f, 0.5f, 1.0f);
        pRendererPreview->UpdateTexture({pRenderer->outputTexture});
        
    }
    return self;
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize {
    [self drawRect:self.superview.bounds];
}

- (void)drawRect:(NSRect)dirtyRect {
    NSLog(@"%@", NSStringFromRect(dirtyRect));
    NSDate *date = [NSDate date];
    [super drawRect:dirtyRect];
    
    pGLContext->switchContext();
    pRenderer->Render();
    
    float scale = 2; // TODO: 自动获取Retina scale
    
    pRendererPreview->UpdatePreviewSize(dirtyRect.size.width * scale, dirtyRect.size.height * scale);
    pRendererPreview->Render();
    
    pGLContext->flush();
    glFinish(); // 添加glFinish()以阻塞等待GPU执行完成

    NSLog(@"耗时：%.2fms", [[NSDate  date] timeIntervalSinceDate:date] * 1000.0f);
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
