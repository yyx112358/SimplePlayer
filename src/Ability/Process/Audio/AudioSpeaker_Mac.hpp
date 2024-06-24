//
//  AudioSpeaker_Mac.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

#include "Pipeline.hpp"
#include <CoreAudioTypes/CoreAudioBaseTypes.h>


namespace sp {

class AudioSpeaker_Mac_Opaque;

class AudioSpeaker_Mac {
public:
    AudioSpeaker_Mac();
    ~AudioSpeaker_Mac();
    
    bool init(const AudioStreamBasicDescription &desc);
    
    bool start(bool isSync);
    bool stop(bool isSync);
    bool pause(bool isSync);
    
public:
    bool enqueue(std::shared_ptr<sp::AudioFrame>);
    
    std::optional<double> getAudioClock();
private:
    const std::unique_ptr<AudioSpeaker_Mac_Opaque> _impl;
};

}
