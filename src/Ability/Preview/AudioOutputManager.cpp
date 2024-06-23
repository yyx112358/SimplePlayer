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

AudioOutputManager::~AudioOutputManager() 
{
    uninit();
}

bool AudioOutputManager::init() 
{
    
    return true;
}

bool AudioOutputManager::uninit() 
{
    if (_processThread.joinable()) {
        stop(true);
    }
    _speaker = nullptr;
    return true;
}

bool AudioOutputManager::start(bool isSync) {
    SPASSERT(_inputQueue.lock() != nullptr);
    if (_inputQueue.lock() == nullptr)
        return false;
    
    // stop的_setStatus()到线程完全结束之间还有一小段时间，需要等待完全结束
    if (_processThread.joinable() == true && _status == Status::STOP)
        _processThread.join();
    if (_processThread.joinable() == false) {
        _processThread = std::thread([this]{ _loop(); });
    }
    _status = Status::RUN;
    if (_speaker)
        _speaker->start(isSync);
    
    return true;
}

bool AudioOutputManager::stop(bool isSync) {
    
    if (auto inputQueue = _inputQueue.lock()) {
        inputQueue->clear();
        inputQueue->enqueue(Pipeline::CreateStopPipeline());
    }
    
    if (isSync) {
        if (_processThread.joinable())
            _processThread.join();
    }
    if (_speaker)
        _speaker->stop(isSync);
    return true;
}

bool AudioOutputManager::pause(bool isSync) {
    // stop的_setStatus()到线程完全结束之间还有一小段时间，需要等待完全结束
    if (_processThread.joinable() == true && _status == Status::STOP)
        _processThread.join();
    if (_processThread.joinable() == false) {
        _processThread = std::thread([this]{ _loop(); });
    }
    if (_speaker)
        _speaker->pause(isSync);
    
    return true;
}


void AudioOutputManager::_loop() {
    SPLOGD("Audio output thread start");
    while(1) {
        auto inpueQueue = _inputQueue.lock();
        if (inpueQueue == nullptr) {
            _status = Status::STOP;
            _speaker->stop(false);
            break;
        }
        
        std::shared_ptr<Pipeline> pipeline = inpueQueue->deque();
        
        if (_status == Status::STOP || pipeline->status == Pipeline::EStatus::STOP) {
            _status = Status::STOP;
            _speaker->stop(false);
            break;
        }
        
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
            _speaker->enqueue(pipeline->audioFrame);
            _speaker->start(false);
            continue;
        }
        
        if (_speaker && pipeline->audioFrame) {
            _speaker->enqueue(pipeline->audioFrame);
        }
    }
    SPLOGD("Audio output thread end");
}
