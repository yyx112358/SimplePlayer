//
//  ViewController.m
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/3.
//

#import "ViewController.h"

#include "SPLog.h"
#include "DecoderManager.hpp"
#include "AudioRendererManager.hpp"
#include "AudioOutputManager.hpp"
#import "IPreviewManager.hpp"


@interface ViewController () {
    std::shared_ptr<IPreviewManager> preview;
    std::shared_ptr<sp::AudioRendererManager> audioRenderer;
    std::shared_ptr<sp::AudioOutputManager> audioOutput;
    std::shared_ptr<sp::DecoderManager> decoder;
}

@property (weak) IBOutlet NSView *playerView;


@property (nonatomic, strong) NSTimer *timer;
@end



@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

         NSString *video = [[NSBundle mainBundle] pathForResource:@"Sync" ofType:@"mp4"];
//    NSString *video = [[NSBundle mainBundle] pathForResource:@"1：1" ofType:@"MOV"];
    
    decoder = std::make_shared<sp::DecoderManager>();
    decoder->init(video.UTF8String);
    
    audioRenderer = std::make_shared<sp::AudioRendererManager>();
    audioRenderer->init();
    audioRenderer->setInputQueue(decoder->_audioQueue);
    
    audioOutput = std::make_shared<sp::AudioOutputManager>();
    audioOutput->init();
    audioOutput->setInputQueue(audioRenderer->getOutputQueue());
    
    preview = IPreviewManager::createIPreviewManager();
    preview->setParentViews((__bridge_retained void *)self.playerView);
    preview->setPipelineQueue(decoder->_videoQueue, audioRenderer->getOutputQueue());
    
    decoder->start(false);
    audioRenderer->start(false);
    audioOutput->start(false);
    preview->start(false);
    
    // FIXME: 这种写法会引入循环引用
//    self.timer = [NSTimer scheduledTimerWithTimeInterval:1.0 / 60 target:self selector:@selector(refresh:) userInfo:self repeats:YES];
}


- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}

- (void)viewDidLayout {
    for(NSView *subView in self.playerView.subviews) {
        [subView setFrame:self.playerView.bounds];
        [subView setNeedsDisplay:YES];
    }
}

- (void)refresh:(id)obj {
    if (decoder != nullptr) {
        int cnt = 10;
        do {
            
            if (auto pipeline = decoder->getNextFrame(sp::DecoderManager::MediaType::AUDIO)) {
                if (pipeline->status == sp::Pipeline::EStatus::END_OF_FILE) {
                    [self exit];
                    return;
                }
                preview->addPipeline(pipeline);
            }
            
            if (auto pipeline = decoder->getNextFrame(sp::DecoderManager::MediaType::VIDEO)) {
                if (pipeline->status == sp::Pipeline::EStatus::END_OF_FILE) {
                    [self exit];
                    return;
                }
                
                if (preview->addPipeline(pipeline) == true)
                    break;
            }
        } while(cnt-- >= 0);
        
        if (cnt < 0) {
            [self exit];
            return;
        }
    }

    
    for(NSView *subView in self.playerView.subviews) {
        [subView setFrame:self.playerView.bounds];
        [subView setNeedsDisplay:YES];
    }
}

- (void)exit {
    decoder = nullptr;
    audioRenderer = nullptr;
    audioOutput = nullptr;
    preview = nullptr;
    [self.view.window.windowController close];
    __weak ViewController* wself = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 500 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
        __strong ViewController *sself = wself;
        [NSApp terminate:sself];
    });
}

@end
