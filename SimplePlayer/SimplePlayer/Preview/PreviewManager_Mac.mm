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

#include "GLContextMac.hpp"
#include "GLRendererCharPainting.hpp"
#include "GLRendererPreview.hpp"
#include "ImageReader.hpp"

std::optional<sp::Frame> LoadBufferFromImage(NSImage *image) {
    std::optional<sp::Frame> result;
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
    sp::Frame imageBuffer;
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
    std::shared_ptr<sp::GLContextMac> pGLContext;
    std::unique_ptr<sp::GLRendererCharPainting> pRenderer;
    std::unique_ptr<sp::GLRendererPreview> pRendererPreview;
    
    // TODO: 引入队列
    std::shared_ptr<sp::GLTexture> imageTexture;
    std::shared_ptr<sp::GLTexture> charTexture;
    
    std::optional<sp::Frame> imageBuffer;
    std::optional<sp::Frame> charBuffer;
}

@end

@implementation Preview_Mac

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        // 创建并初始化GLContext
        pGLContext = dynamic_pointer_cast<sp::GLContextMac>(sp::IGLContext::CreateGLContext());
        if (pGLContext->Init() == false)
            return nil;
        [self setOpenGLContext:pGLContext->context()];
        
        // 创建并初始化Renderer
        pRenderer = std::make_unique<sp::GLRendererCharPainting>(pGLContext);
        pRenderer->SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        
        // 指定字符尺寸
        pRenderer->SetCharSize(8, 12);
        
        // 上屏Renderer
        pRendererPreview = std::make_unique<sp::GLRendererPreview>(pGLContext);
        pRendererPreview->SetClearColor(0.75f, 0.5f, 0.5f, 1.0f);
        
//        imageBuffer = LoadBufferFromImage([NSImage imageNamed:@"texture.jpg"]); // 待转换图像
        charBuffer = LoadBufferFromImage([NSImage imageNamed:@"charTexture.bmp"]); // 字符纹理
        
        imageTexture = std::make_shared<sp::GLTexture>(pGLContext);
        charTexture = std::make_shared<sp::GLTexture>(pGLContext, *charBuffer);
    }
    return self;
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize {
    [self drawRect:self.superview.bounds];
}

- (void)drawRect:(NSRect)dirtyRect {
    NSDate *date = [NSDate date];
    [super drawRect:dirtyRect];
    if (imageBuffer.has_value() == false || charBuffer.has_value() == false)
        return;
    NSLog(@"%@", NSStringFromRect(dirtyRect));
    
    pGLContext->SwitchContext();
    
    // 加载纹理
    imageTexture->UploadBuffer(*imageBuffer);
//    charTexture->UploadBuffer(*charBuffer);
    imageBuffer.reset();
    
    pRenderer->UpdateTexture({imageTexture, charTexture});
    
    if (std::shared_ptr<sp::GLTexture> outputTexture = pRenderer->GetOutputTexture(); outputTexture == nullptr) {
        pRenderer->UpdateOutputTexture(std::make_shared<sp::GLTexture>(pGLContext, sp::Frame{.width = imageTexture->width(), .height = imageTexture->height(), .pixelFormat = AV_PIX_FMT_RGBA}));
    } else if (outputTexture->width() != imageTexture->width() || outputTexture->height() != imageTexture->height()) {
        outputTexture->UploadBuffer(sp::Frame{.width = imageTexture->width(), .height = imageTexture->height(), .pixelFormat = AV_PIX_FMT_RGBA});
    }
    pRendererPreview->UpdateTexture({pRenderer->GetOutputTexture()});
    
    pRenderer->Render();
    
    float scale = 2; // TODO: 自动获取Retina scale
    
    pRendererPreview->UpdatePreviewSize(dirtyRect.size.width * scale, dirtyRect.size.height * scale);
    pRendererPreview->Render();
    
    pGLContext->Flush();
//    glFinish(); // 添加glFinish()以阻塞等待GPU执行完成

    NSLog(@"耗时：%.2fms", [[NSDate  date] timeIntervalSinceDate:date] * 1000.0f);
}

- (void) setBuffer:(std::optional<sp::Frame>)frame {
    imageBuffer = frame;
}

@end

NSMutableArray<Preview_Mac *> *previews;

PreviewManager_Mac::~PreviewManager_Mac() {
    for (Preview_Mac *preview in previews) {
        [preview removeFromSuperview];
    }
    [previews removeAllObjects];
}

bool PreviewManager_Mac::setParentViews(void *parents) {
    if (previews == nil)
        previews = [NSMutableArray array];
    
    NSView *superView = (__bridge NSView *)parents;
    Preview_Mac * preview = [[Preview_Mac alloc] initWithFrame:superView.bounds];
    [superView addSubview:preview];
    [previews addObject:preview];
    
    return true;
}

bool PreviewManager_Mac::render(std::optional<sp::Frame> frame) {
    for (Preview_Mac *preview in previews) {
        [preview setBuffer:frame];
    }
    
    return true;
}

std::shared_ptr<IPreviewManager> IPreviewManager::createIPreviewManager() {
    return std::make_shared<PreviewManager_Mac>();
}
