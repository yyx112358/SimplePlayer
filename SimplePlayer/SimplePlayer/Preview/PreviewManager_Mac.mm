//
//  PreviewManager_Mac.m
//  SimplePlayer
//
//  Created by YangYixuan on 2023/5/29.
//

#import <Foundation/Foundation.h>
#import <AVFAudio/AVFAudio.h>
#import <AppKit/NSImage.h>
#include "PreviewManager_Mac.h"
#include <string>
#include <optional>
#include <array>
#include <queue>
#include <mutex>
#include <any>
#include <numeric>


#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#include "SPLog.h"
#include "GLContextMac.hpp"
#include "GLRendererCharPainting.hpp"
#include "GLRendererPreview.hpp"
#include "GLRendererMultiBlend.hpp"
#include "ImageReader.hpp"

std::optional<sp::VideoFrame> LoadBufferFromImage(NSImage *image) {
    std::optional<sp::VideoFrame> result;
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
    sp::VideoFrame imageBuffer;
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
    CVPixelBufferRelease(pixelBuf);
    
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
    std::unique_ptr<sp::GLRendererMultiBlend> pBlendRenderer;
    std::unique_ptr<sp::GLRendererCharPainting> pRenderer;
    std::unique_ptr<sp::GLRendererPreview> pRendererPreview;
    
    // TODO: 引入队列
    std::shared_ptr<sp::GLTexture> imageTexture;
    std::shared_ptr<sp::GLTexture> charTexture;
    
    std::shared_ptr<sp::VideoFrame> imageBuffer;
    std::optional<sp::VideoFrame> charBuffer;
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
        
        pBlendRenderer = std::make_unique<sp::GLRendererMultiBlend>(pGLContext);
        
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
    if (imageBuffer == nullptr || charBuffer.has_value() == false) {
        // 缩放窗口时，需要Preview Renderer使用上一次的图像重绘一次
        pRendererPreview->UpdatePreviewSize(dirtyRect.size.width * 2, dirtyRect.size.height * 2);
        pRendererPreview->Render();
        pGLContext->Flush();
        return;
    }
    
    pGLContext->SwitchContext();
    
    // 加载纹理
    imageTexture->UploadBuffer(*imageBuffer);
//    charTexture->UploadBuffer(*charBuffer);
    imageBuffer.reset();
    
    std::vector<std::shared_ptr<sp::GLTexture>> blendTexs(256, imageTexture);
    pBlendRenderer->UpdateTexture(blendTexs);
    
    {
        static bool b = true;
        srand(0);
        if (b) {
            for (int i = 0; i < blendTexs.size(); i++) {
                pBlendRenderer->SetScale(i, (float)rand() / RAND_MAX);
                pBlendRenderer->SetTransX(i, ((float)rand() / RAND_MAX - 0.5) * 1920.0);
                pBlendRenderer->SetTransY(i, ((float)rand() / RAND_MAX - 0.5) * 1080.0);
            }
            b = false;
        }
    }
    pBlendRenderer->SetDisplayRotation(1, sp::EDisplayRotation::Rotation90);
    for (int i = 0; i < blendTexs.size(); i++)
        pBlendRenderer->SetFreeRotation(i, pBlendRenderer->GetFreeRotation(i) + i % 4 + 1);
    
    if (std::shared_ptr<sp::GLTexture> outputTexture = pBlendRenderer->GetOutputTexture(); outputTexture == nullptr) {
        pBlendRenderer->UpdateOutputTexture(std::make_shared<sp::GLTexture>(pGLContext, sp::VideoFrame{.width = imageTexture->width(), .height = imageTexture->height(), .pixelFormat = AV_PIX_FMT_RGBA}));
    } else if (outputTexture->width() != imageTexture->width() || outputTexture->height() != imageTexture->height()) {
        outputTexture->UploadBuffer(sp::VideoFrame{.width = imageTexture->width(), .height = imageTexture->height(), .pixelFormat = AV_PIX_FMT_RGBA});
    }

    pBlendRenderer->Render();
    
    pRenderer->UpdateTexture({pBlendRenderer->GetOutputTexture(), charTexture});
    
    if (std::shared_ptr<sp::GLTexture> outputTexture = pRenderer->GetOutputTexture(); outputTexture == nullptr) {
        pRenderer->UpdateOutputTexture(std::make_shared<sp::GLTexture>(pGLContext, sp::VideoFrame{.width = imageTexture->width(), .height = imageTexture->height(), .pixelFormat = AV_PIX_FMT_RGBA}));
    } else if (outputTexture->width() != imageTexture->width() || outputTexture->height() != imageTexture->height()) {
        outputTexture->UploadBuffer(sp::VideoFrame{.width = imageTexture->width(), .height = imageTexture->height(), .pixelFormat = AV_PIX_FMT_RGBA});
    }
    
    pRenderer->Render();
    
    float scale = 2; // TODO: 自动获取Retina scale
    
    pRendererPreview->UpdateTexture({pRenderer->GetOutputTexture()});
    pRendererPreview->UpdatePreviewSize(dirtyRect.size.width * scale, dirtyRect.size.height * scale);
    pRendererPreview->Render();
    
    pGLContext->Flush();
//    glFinish(); // 添加glFinish()以阻塞等待GPU执行完成

    double duration = [[NSDate  date] timeIntervalSinceDate:date] * 1000.0f;
    static std::vector<double> allDuration;
    if (allDuration.size() < 400) {
        allDuration.push_back(duration);
        double mean = std::reduce(allDuration.cbegin(), allDuration.cend(), 0);
        mean /= allDuration.size();
        NSLog(@"渲染耗时：%.2fms, %.2f ms", mean, duration);
    } else {
        double mean = std::reduce(allDuration.cbegin(), allDuration.cend(), 0);
        mean /= allDuration.size();
        NSLog(@"======渲染耗时：%.2fms, %.2f ms", mean, duration);
    }
}

- (void) setBuffer:(std::shared_ptr<sp::VideoFrame>)frame {
    if (frame != nullptr && frame->data != nullptr) {
        SPASSERT(frame->pixelFormat == AV_PIX_FMT_RGBA);
    }
    imageBuffer = frame;
}

@end


NSMutableArray<Preview_Mac *> *previews;

PreviewManager_Mac::~PreviewManager_Mac() {
    stop(true);
    
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

bool PreviewManager_Mac::notifyParentViewsChanged() {
    for (Preview_Mac *preview in previews) {
        [preview setNeedsLayout:YES];
    }
    return true;
}

bool PreviewManager_Mac::start(bool isSync) {
    
    if (_videoDisplayLink == NULL) {
        CVDisplayLinkCreateWithActiveCGDisplays(&_videoDisplayLink);
        CVDisplayLinkSetOutputCallback(_videoDisplayLink, &PreviewManager_Mac::_displayLinkCallback, this); // TODO: 使用weak_ptr
    }
    CVDisplayLinkStart(_videoDisplayLink);
    
    return true;
}

bool PreviewManager_Mac::stop(bool isSync) {
    
    if (_videoDisplayLink != NULL) {
        CVDisplayLinkStop(_videoDisplayLink);
        _videoDisplayLink = NULL;
    }
    
    return true;
}

CVReturn PreviewManager_Mac::_displayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *now, const CVTimeStamp *outputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext) {
    PreviewManager_Mac *preview = static_cast<PreviewManager_Mac *>(displayLinkContext);
    
    if (preview->_videoQueue->size() > 0) {
        auto pipeline = preview->_videoQueue->deque();
        // TODO: 等待音频同步
        // 感谢FFMpeg，都不用做啥等待操作就已经很同步很流畅了~~
        if (pipeline->videoFrame != nullptr && pipeline->videoFrame->data != nullptr) {
            for (Preview_Mac *preview in previews) {
                [preview setBuffer:pipeline->videoFrame];
            }
            dispatch_async(dispatch_get_main_queue(), ^{
                for (Preview_Mac *preview in previews) {
                    [preview setFrame:preview.frame];
                    [preview setNeedsDisplay:YES];
                    [preview setNeedsLayout:YES];
                }
            });
        }
    }
    
    return kCVReturnSuccess;
}

std::shared_ptr<IPreviewManager> IPreviewManager::createIPreviewManager() {
    return std::make_shared<PreviewManager_Mac>();
}
