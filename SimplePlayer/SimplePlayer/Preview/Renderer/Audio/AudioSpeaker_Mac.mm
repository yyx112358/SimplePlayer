//
//  AudioSpeaker_Mac.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#include "AudioSpeaker_Mac.hpp"
#include "SPLog.h"
#include "RingQueue.hpp"

#import <AudioToolbox/AudioToolbox.h>

@class AudioSpeaker_Mac_Impl;

namespace sp {

class AudioSpeaker_Mac_Opaque {
public:
    AudioSpeaker_Mac_Impl * obj;
};

}

/// 音频回调函数
void audioQueueOutputCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer);

using namespace sp;

@interface AudioSpeaker_Mac_Impl : NSObject
{
    AudioStreamBasicDescription _audioFormat;
    AudioQueueRef _audioQueue;          // 音频队列
    sp::RingQueue<std::shared_ptr<sp::AudioFrame>, 3> _audioBufferQueue;
}

@property (nonatomic, readonly) BOOL isRunning;

- (instancetype)init;
- (void)enqueue:(std::shared_ptr<sp::AudioFrame>)audioFrame;

@end


@implementation AudioSpeaker_Mac_Impl

/// 使用默认配置初始化
- (instancetype)init {

    AudioStreamBasicDescription audioFormat;
    audioFormat.mSampleRate = 44100;
    audioFormat.mFormatID = kAudioFormatLinearPCM;
    audioFormat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    audioFormat.mFramesPerPacket = 1;
    audioFormat.mChannelsPerFrame = 1;
    audioFormat.mBitsPerChannel = 32;
    audioFormat.mBytesPerPacket = audioFormat.mBytesPerFrame = (audioFormat.mBitsPerChannel / 8) * audioFormat.mChannelsPerFrame;
    
    return [self initWithAudioFormat:audioFormat];
}

/// 使用给定audioFormat初始化
- (instancetype)initWithAudioFormat:(AudioStreamBasicDescription)audioFormat {
    
    if (self = [super init]) {
        // 创建音频队列
        _audioFormat = audioFormat;
        OSStatus status = AudioQueueNewOutput(&_audioFormat, audioQueueOutputCallback, (__bridge void *)(self), NULL, NULL, 0, &_audioQueue);
        if (status != 0) {
            SPLOGE("AudioQueueNewOutput Failed: %d", status);
        }
        
    }
    return self;
}

- (void)dealloc {
    // 等待音频播放完成
    [self stop];
    
    // 销毁音频队列和缓冲区
    AudioQueueDispose(_audioQueue, true);
}

- (BOOL)isRunning {
    UInt32 outData = 0, ioDataSize = sizeof(outData);
    AudioQueueGetProperty(_audioQueue, kAudioQueueProperty_IsRunning, &outData, &ioDataSize);
    return outData != 0;
}

/// 当前音频时间戳
- (double)currentPts {
    AudioTimeStamp outTimeStamp;
    Boolean outTimelineDiscontinuity;
    if (OSStatus status = AudioQueueGetCurrentTime(_audioQueue, nil, &outTimeStamp, &outTimelineDiscontinuity); status == 0) {
        return outTimeStamp.mSampleTime;
    } else
        return -1;
}

- (AudioStreamBasicDescription)getAudioFormat {
    return _audioFormat;
}

- (void)play {
    if (self.isRunning == false) {
        if (_audioBufferQueue.empty() == false) {
            AudioQueueBufferRef audioBuffer = nil;
            auto audioFrame = [self dequeue];
            [self frameToBuffer:audioFrame audioBuffer:&audioBuffer];
            AudioQueueEnqueueBuffer(_audioQueue, audioBuffer, 0, 0);
        }
    }
    AudioQueueStart(_audioQueue, NULL);
}

- (void)stop {
    while(_audioBufferQueue.size() > 0)
        _audioBufferQueue.deque();
    if (self.isRunning == true) {
        AudioQueueFlush(_audioQueue);
        AudioQueueStop(_audioQueue, true);
    }
}

- (void)pause {
    AudioQueuePause(_audioQueue);
}

- (void)enqueue:(std::shared_ptr<sp::AudioFrame>)audioFrame {
    if (audioFrame == nullptr || audioFrame->data == nullptr)
        return;
    
    _audioBufferQueue.enqueue(audioFrame);
}

- (std::shared_ptr<sp::AudioFrame>)dequeue {
    if (_audioBufferQueue.empty())
        return nullptr;
    else
        return _audioBufferQueue.deque();
}

- (void)frameToBuffer:(std::shared_ptr<sp::AudioFrame>)audioFrame audioBuffer:(AudioQueueBufferRef *)pAudioBuffer {
    
    if (audioFrame == nullptr)
        return;
    
    if (*pAudioBuffer == nullptr) {
        OSStatus status = 0;
        status = AudioQueueAllocateBuffer(_audioQueue, (UInt32)audioFrame->dataSize, pAudioBuffer);
        SPASSERTEX(status == 0, "AudioQueueAllocateBuffer Failed: %d", status);
        if (status != 0) {
            SPLOGE("AudioQueueAllocateBuffer Failed: %d", status);
            return;
        }
    }
    
    AudioQueueBufferRef audioBuffer = *pAudioBuffer;
    SPASSERT(audioFrame->dataSize <= audioBuffer->mAudioDataBytesCapacity);
    SPASSERT(audioFrame->sampleFormat == AV_SAMPLE_FMT_FLTP);
    SPASSERT(audioFrame->sampleRate == _audioFormat.mSampleRate);
//    SPASSERT(audioFrame->channels == audioFormat.mChannelsPerFrame);
    memcpy(audioBuffer->mAudioData, audioFrame->data.get(), audioFrame->dataSize);
    audioBuffer->mAudioDataByteSize = (UInt32)audioFrame->dataSize;
    
}

@end

// 音频数据回调函数
void audioQueueOutputCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer) {
    AudioSpeaker_Mac_Impl *speaker = (__bridge AudioSpeaker_Mac_Impl *)inUserData;
    if (speaker.isRunning == false)
        return;
  
    // 循环等待下一段音频数据
    std::shared_ptr<sp::AudioFrame> audioFrame;
    int repeat = 10; // 音频帧等待时长，超时将停止音频播放 TODO: 根据sampleRate动态调整。目前实验等待15ms是可以恢复播放的，保留一定余量，超时定为10ms
    do {
        audioFrame = [speaker dequeue];
        [NSThread sleepForTimeInterval:0.001];
        
        if (speaker.isRunning == false)
            return;
    } while (audioFrame == nullptr && --repeat > 0);

    // 长时间等待无效果，则填充静音帧。可能出现爆音、卡顿
    // TODO: 填充静音帧而不是stop，可能会导致收到新buffer时延迟10~20ms才能收到，是否有必要优化呢？
    if (repeat <= 0) {
        SPLOGE("dequeue audio buffer failed");
        memset(inBuffer->mAudioData, 0, inBuffer->mAudioDataByteSize);
//        // 另一种思路，直接stop/pause
//        [speaker pause];
//        return;
    }
    
    // 重新将缓冲区添加到音频队列中
    [speaker frameToBuffer:audioFrame audioBuffer:&inBuffer];
    OSStatus status = AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
    if (status != 0) {
        [speaker stop];
    }
}

#pragma mark - AudioSpeaker_Mac

AudioSpeaker_Mac::AudioSpeaker_Mac(): _impl(std::make_unique<AudioSpeaker_Mac_Opaque>()) {
    
}

AudioSpeaker_Mac::~AudioSpeaker_Mac() {
    
}

bool AudioSpeaker_Mac::init(const AudioStreamBasicDescription &desc) {
    _impl->obj = [[AudioSpeaker_Mac_Impl alloc] initWithAudioFormat:desc];
    
    return true;
}

bool AudioSpeaker_Mac::start(bool isSync) {
    SPASSERT(_impl->obj);
    [_impl->obj play];
    
    return true;
}

bool AudioSpeaker_Mac::stop(bool isSync) {
    // TODO: 异步操作。否则有20ms左右延迟
    SPASSERT(_impl->obj);
    [_impl->obj stop];
    return true;
}

bool AudioSpeaker_Mac::pause(bool isSync) {
    // TODO: 异步操作。否则有20ms左右延迟
    SPASSERT(_impl->obj);
    [_impl->obj pause];
    return true;
}

bool AudioSpeaker_Mac::enqueue(std::shared_ptr<sp::AudioFrame> frame) {
    [_impl->obj enqueue:frame];
    
    return true;
}

std::optional<double> AudioSpeaker_Mac::getAudioClock() {
    if (_impl->obj != nullptr)
        return [_impl->obj currentPts];
    else
        return std::nullopt;
}

