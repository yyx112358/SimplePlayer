//
//  PreviewManager_Mac.m
//  SimplePlayer
//
//  Created by YangYixuan on 2023/5/29.
//

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFAudio/AVFAudio.h>
#import <AppKit/NSImage.h>
#include "PreviewManager_Mac.h"
#include <string>
#include <optional>
#include <array>
#include <queue>
#include <mutex>
#include <any>


#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#include "GLContextMac.hpp"
#include "GLRendererCharPainting.hpp"
#include "GLRendererPreview.hpp"
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
    if (imageBuffer == nullptr || charBuffer.has_value() == false)
        return;
    NSLog(@"%@", NSStringFromRect(dirtyRect));
    
    pGLContext->SwitchContext();
    
    // 加载纹理
    imageTexture->UploadBuffer(*imageBuffer);
//    charTexture->UploadBuffer(*charBuffer);
    imageBuffer.reset();
    
    pRenderer->UpdateTexture({imageTexture, charTexture});
    
    if (std::shared_ptr<sp::GLTexture> outputTexture = pRenderer->GetOutputTexture(); outputTexture == nullptr) {
        pRenderer->UpdateOutputTexture(std::make_shared<sp::GLTexture>(pGLContext, sp::VideoFrame{.width = imageTexture->width(), .height = imageTexture->height(), .pixelFormat = AV_PIX_FMT_RGBA}));
    } else if (outputTexture->width() != imageTexture->width() || outputTexture->height() != imageTexture->height()) {
        outputTexture->UploadBuffer(sp::VideoFrame{.width = imageTexture->width(), .height = imageTexture->height(), .pixelFormat = AV_PIX_FMT_RGBA});
    }
    pRendererPreview->UpdateTexture({pRenderer->GetOutputTexture()});
    
    pRenderer->Render();
    
    float scale = 2; // TODO: 自动获取Retina scale
    
    pRendererPreview->UpdatePreviewSize(dirtyRect.size.width * scale, dirtyRect.size.height * scale);
    pRendererPreview->Render();
    
    pGLContext->Flush();
//    glFinish(); // 添加glFinish()以阻塞等待GPU执行完成

//    NSLog(@"耗时：%.2fms", [[NSDate  date] timeIntervalSinceDate:date] * 1000.0f);
}

- (void) setBuffer:(std::shared_ptr<sp::VideoFrame>)frame {
    if (frame != nullptr && frame->data != nullptr) {
        assert(frame->pixelFormat == AV_PIX_FMT_RGBA);
    }
    imageBuffer = frame;
}

@end

void audioQueueOutputCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer);

@interface AudioSpeaker_Mac : NSObject
{
    AudioQueueRef audioQueue;          // 音频队列
//    AudioQueueBufferRef audioBuffer;   // 音频缓冲区
    std::queue<AudioQueueBufferRef> audioBufferQueue;
    std::mutex audioBufferLock;
    
    bool isRunning;
}


- (instancetype)init;
- (void)enqueue;
- (void)enqueue:(std::shared_ptr<sp::AudioFrame>)audioFrame;



@end


@implementation AudioSpeaker_Mac

- (instancetype)init {
    if (self = [super init]) {
        
        // 创建音频队列
        AudioStreamBasicDescription audioFormat;
        audioFormat.mSampleRate = 44100.0;
        audioFormat.mFormatID = kAudioFormatLinearPCM;
        audioFormat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        audioFormat.mFramesPerPacket = 1;
        audioFormat.mChannelsPerFrame = 1;
        audioFormat.mBitsPerChannel = 32;
        audioFormat.mBytesPerPacket = audioFormat.mBytesPerFrame = (audioFormat.mBitsPerChannel / 8) * audioFormat.mChannelsPerFrame;
        
        OSStatus status = AudioQueueNewOutput(&audioFormat, audioQueueOutputCallback, (__bridge void *)(self), NULL, NULL, 0, &audioQueue);
        
        isRunning = false;
    }
    return self;
}

- (void)dealloc {
    // 等待音频播放完成
    AudioQueueFlush(audioQueue);
    AudioQueueStop(audioQueue, true);
    
    // 销毁音频队列和缓冲区
    AudioQueueDispose(audioQueue, true);
    while (audioBufferQueue.size() > 0) {
        audioBufferLock.lock();
        AudioQueueBufferRef buffer = audioBufferQueue.front();
        audioBufferQueue.pop();
        audioBufferLock.unlock();
        
        AudioQueueFreeBuffer(audioQueue, buffer);
    }
}

- (void)play {
    if (isRunning == false && audioBufferQueue.size() > 0) {
        AudioQueueEnqueueBuffer(audioQueue, [self deque], 0, 0);
        AudioQueueStart(audioQueue, NULL);
    }
}

- (void)enqueue:(std::shared_ptr<sp::AudioFrame>)audioFrame {
    if (audioFrame == nullptr || audioFrame->data == nullptr)
        return;
    
    OSStatus status = 0;
    AudioQueueBufferRef audioBuffer = nullptr;
    status = AudioQueueAllocateBuffer(audioQueue, (UInt32)audioFrame->dataSize, &audioBuffer);
    memcpy(audioBuffer->mAudioData, audioFrame->data.get(), audioFrame->dataSize);
    audioBuffer->mAudioDataByteSize = (UInt32)audioFrame->dataSize;
    
    // TODO: 支持队列上限
    audioBufferLock.lock();
    audioBufferQueue.push(audioBuffer);
    audioBufferLock.unlock();
}

- (AudioQueueBufferRef)deque {
    if (audioBufferQueue.empty())
        return nullptr;
    
    audioBufferLock.lock();
    AudioQueueBufferRef buffer = audioBufferQueue.front();
    audioBufferQueue.pop();
    audioBufferLock.unlock();

    return buffer;
}

@end

// 音频数据回调函数
void audioQueueOutputCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer) {
    // 在回调函数中填充音频数据到缓冲区
    // inBuffer->mAudioData 是缓冲区的数据指针
    // inBuffer->mAudioDataByteSize 是缓冲区的大小
    AudioSpeaker_Mac *speaker = (__bridge AudioSpeaker_Mac *)inUserData;
    
    // TODO: 支持队列上限
    AudioQueueBufferRef audioBuffer = [speaker deque];
    
    
    // 重新将缓冲区添加到音频队列中
    AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
    AudioQueueFreeBuffer(inAQ, inBuffer);// TODO: 支持回收
}

NSMutableArray<Preview_Mac *> *previews;
NSMutableArray<AudioSpeaker_Mac *> *speakers;

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
    
    if (speakers == nil)
        speakers = [NSMutableArray array];
    
    AudioSpeaker_Mac * speaker = [AudioSpeaker_Mac new];
    [speakers addObject:speaker];
    
    return true;
}

bool PreviewManager_Mac::render(std::shared_ptr<sp::Pipeline> pipeline) {
    if (pipeline->videoFrame != nullptr && pipeline->videoFrame->data != nullptr) {
        for (Preview_Mac *preview in previews) {
            [preview setBuffer:pipeline->videoFrame];
        }
    }
    
    if (pipeline->audioFrame != nullptr) {
        AudioSpeaker_Mac *speaker = speakers.firstObject;
        [speaker enqueue:pipeline->audioFrame];
        
        [speakers.firstObject play];
    }
    
    return true;
}

std::shared_ptr<IPreviewManager> IPreviewManager::createIPreviewManager() {
    return std::make_shared<PreviewManager_Mac>();
}
