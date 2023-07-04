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
    std::unique_ptr<sp::GLRendererCharPainting> pRenderer;
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
        pRenderer = std::make_unique<sp::GLRendererCharPainting>(pGLContext);
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
        uniform sampler2D texture0;
        uniform sampler2D texture1;
        uniform int texWidth, texHeight;
        uniform int charWidth, charHeight;

        out vec3 vtxColor;
        out vec2 vtxTexCoord;

        void main()
        {
            gl_Position = transform * vec4(aPos.x, - aPos.y, 0.0, 1.0);
        
            // 计算采样矩形坐标范围，aTexCoord是网格小矩形左上角
            int posX = int(aTexCoord.x * texWidth), posY = int(aTexCoord.y * texHeight);
            
            int left = posX / charWidth * charWidth, top = posY / charHeight * charHeight;
            int right = min(left + charWidth, texWidth);
            int bottom = min(top + charHeight, texHeight);
            
            // 计算平均灰度
            float sumR = 0, sumG = 0, sumB = 0;
            int xstep = 4, ystep = 4; // 不需要每一个点取值，取一部分就可以
            int amount = 0;
            for (int y = top; y < bottom; y += ystep)
            {
                for (int x = left; x < right; x += xstep)
                {
                    vec4 color = texture(texture0, vec2(float(x) / texWidth, float(y) / texHeight));
                    sumR += color.r;
                    sumG += color.g;
                    sumB += color.b;
                    amount++;
                }
            }
            float gray = 0.299f * sumR / amount + 0.587f * sumG / amount + 0.114 * sumB / amount;
            gray = min(gray, 255.f / 256);
            vtxColor = vec3(gray, gray, gray);
            
            // 计算对应的字符纹理坐标。字符纹理从左到右划分为256个charWidth * charHeight矩形，第n个矩形的平均灰度值为n。
            float charTexX = int(gray * 256) / 256.0, charTexY = 1;
            if ((aPos.x + 1) / 2 > aTexCoord.x)
                charTexX = charTexX + 1.0f / 256;
            if ((aPos.y + 1) / 2 < aTexCoord.y)
                charTexY = 0;
            vtxTexCoord = vec2(charTexX, charTexY);
        })";

        // 创建并编译Fragment Shader，方法基本一致
        const GLchar *fragmentShaderSource = R"(
        #version 330 core
        in vec3  vtxColor;
        in vec2  vtxTexCoord;
        
        out vec4 FragColor;
        
        uniform sampler2D texture0;
        uniform sampler2D texture1;

        void main()
        {
            FragColor = texture(texture1, vec2(vtxTexCoord.x, vtxTexCoord.y)).rgba;
//            FragColor = vec4(vtxColor.rgb, 1.0);
        })";
        pRenderer->UpdateShader({vertexShaderSource}, {fragmentShaderSource});
        pRenderer->SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        
        
        // initWithContentsOfFile读不出来。不深究了，直接用imageNamed
        //NSImage *image = [[NSImage alloc] initWithContentsOfFile:@"/Users/yangyixuan/Downloads/texture.jpg"];
        auto buffer = LoadBufferFromImage([NSImage imageNamed:@"texture.jpg"]);
        auto charBuffer = LoadBufferFromImage([NSImage imageNamed:@"charTexture.bmp"]);
        if (buffer.has_value() && charBuffer.has_value())
            pRenderer->UpdateTexture({*buffer, *charBuffer});
        pRenderer->UpdateOutputTexture(std::make_shared<sp::GLTexture>(pGLContext, sp::ImageBuffer{.width = 1920, .height = 1080, .pixelFormat = GL_RGBA}));
        
        pRenderer->UpdateUniform("texWidth", buffer->width);
        pRenderer->UpdateUniform("texHeight", buffer->height);
        pRenderer->UpdateUniform("charWidth", 8);
        pRenderer->UpdateUniform("charHeight", 12);
        pRenderer->SetCharSize(8, 12);
        
        pRendererPreview = std::make_unique<sp::GLRendererPreview>(pGLContext);
        pRendererPreview->SetClearColor(0.75f, 0.5f, 0.5f, 1.0f);
        pRendererPreview->UpdateTexture({pRenderer->GetOutputTexture()});
        
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
//    glFinish(); // 添加glFinish()以阻塞等待GPU执行完成

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
