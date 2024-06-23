//
//  ImageWriterUIImage.mm
//  SimplePlayer
//
//  Created by YangYixuan on 2024/2/2.
//

#include "ImageWriterUIImage.h"
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <AppKit/NSImage.h>

// 反转R、B通道
static void _SwapChannelRB(uint8_t *target, size_t width, size_t height) {
    // 经profile验证，对iOS而言（CPU、GPU共享内存），编译器优化后的直接像素操作是最快的
    // https://bytedance.feishu.cn/docx/XmxYdU05fomCSFxSoRxcTMt8nzc
    for (unsigned int row = 0; row < height; ++row) {
        uint8_t *ptrDst = target;
        for (unsigned int col = 0; col < width; ++col) {
            std::swap(ptrDst[0], ptrDst[2]);
            ptrDst += 4;
        }
        target += width * 4;
    }
}

SPNSObjectHolder writeRGBA2UIImage(const void* data, const int width, const int height, const int bytesPerPixel, bool swapRB) { // Assuming you have the RGBA image data in a buffer
    
    if (swapRB) {
        _SwapChannelRB(reinterpret_cast<uint8_t *>(const_cast<void *>(data)), width, height); // 有时需要调换R、B通道否则色彩会异常
    }
    
    // 构造CGImage
    CGContextRef context = CGBitmapContextCreate(const_cast<void*>(data), width, height, 8, width * bytesPerPixel, CGColorSpaceCreateDeviceRGB(), kCGImageAlphaNoneSkipLast);
    CGImageRef imageRef = CGBitmapContextCreateImage(context);
    CGContextRelease(context);
    
    // 构造NSImage/UIImage
    NSImage *image = [[NSImage alloc] initWithCGImage:imageRef size:CGSizeMake(width, height)];
    return SPNSObjectHolder((void *)CFBridgingRetain(image));
}
