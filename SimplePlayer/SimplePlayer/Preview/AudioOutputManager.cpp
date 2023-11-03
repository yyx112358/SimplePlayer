//
//  AudioOutputManager.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#include "AudioOutputManager.hpp"
#include "AudioSpeaker_Mac.hpp"
#include "SPLog.h"

using namespace sp;

AudioOutputManager::AudioOutputManager() {}

AudioOutputManager::~AudioOutputManager() {}

bool AudioOutputManager::init() {
    
    return true;
}

bool AudioOutputManager::start(bool isSync) {
    SPASSERT(_inputQueue != nullptr);
    if (_inputQueue == nullptr)
        return false;
    
    if (_processThread.joinable() == false) {
        _processThread = std::thread([this]{ _loop(); });
    }
    _status = Status::RUN;
    
    return true;
}


void AudioOutputManager::_loop() {
    while(1) {
        // TODO: 采用cmdQueue
        if (_status == Status::STOP)
            break;
        
        std::shared_ptr<Pipeline> pipeline = _inputQueue->deque();
        
        // TODO: 应该有一个专门的解析流程，将Preview、Speaker的创建和配置集中起来
        // TODO: 不支持更改音频配置。如果存在采样率等参数不一致，应加入一个converter过程做音频重采样
        if (_speaker == nullptr && pipeline->audioFrame != nullptr) {
            std::shared_ptr<sp::AudioFrame> audioFrame = pipeline->audioFrame;
            AudioStreamBasicDescription audioFormat;
    
            // 因为FFMpeg已经解码为PCM数据，因此mFormatID固定为PCM，mFramesPerPacket固定为1。
            audioFormat.mFormatID = kAudioFormatLinearPCM;
            audioFormat.mFramesPerPacket = 1;
            switch (audioFrame->sampleFormat) {
                case AV_SAMPLE_FMT_FLTP:
                    audioFormat.mFormatFlags = kAudioFormatFlagIsFloat;
                    audioFormat.mBitsPerChannel = 8 * sizeof(float);
                    break;
    
                default:
                    SPASSERT_NOT_IMPL;
            }
            audioFormat.mSampleRate = audioFrame->sampleRate;
            audioFormat.mChannelsPerFrame = audioFrame->channels;
            audioFormat.mBytesPerFrame = (audioFormat.mBitsPerChannel / 8) * audioFormat.mChannelsPerFrame;
            audioFormat.mBytesPerPacket = audioFormat.mFramesPerPacket * audioFormat.mBytesPerFrame;
    
            /* 使用系统AudioFileStream解析的代码
            NSURL *audioFileURL = [NSURL fileURLWithPath:"audio.mp3"];
            AudioFileID audioFile;
            OSStatus status = AudioFileOpenURL((__bridge CFURLRef)audioFileURL, kAudioFileReadPermission, 0, &audioFile);
            if (status != noErr) {
                // 处理打开音频文件失败的情况
                return;
            }
            AudioStreamBasicDescription audioStreamDesc;
            UInt32 size = sizeof(audioStreamDesc);
            status = AudioFileGetProperty(audioFile, kAudioFilePropertyDataFormat, &size, &audioStreamDesc);
            if (status != noErr) {
                // 处理获取音频流描述信息失败的情况
                AudioFileClose(audioFile);
                return;
            }
            AudioFileClose(audioFile);
            */
            
            _speaker = std::make_unique<AudioSpeaker_Mac>();
            _speaker->init(audioFormat);
        }
        
        if (_speaker && pipeline->audioFrame) {
            _speaker->enqueue(pipeline->audioFrame);
            _speaker->start(false);
        }
    }
}
