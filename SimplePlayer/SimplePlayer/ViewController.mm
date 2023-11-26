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

@property (weak) IBOutlet NSButton *playBtn;
@property (weak) IBOutlet NSButton *stopBtn;
@property (weak) IBOutlet NSSlider *seekSlider;

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
    preview->setPipelineQueue(decoder->_videoQueue);
    
    decoder->start(false);
    audioRenderer->start(false);
    audioOutput->start(false);
    preview->start(false);
    
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

- (IBAction)playClicked:(NSButtonCell *)sender {
    if ([sender.title isEqualToString:@"▶️"]) {
        if (decoder != nullptr)
            decoder->pause(false);
        if (audioOutput != nullptr)
            audioOutput->pause(false);
        [sender setTitle:@"⏸"];
    } else {
        if (decoder != nullptr)
            decoder->start(false);
        if (audioOutput != nullptr)
            audioOutput->start(false);
        [sender setTitle:@"▶️"];
    }
}

- (IBAction)stopClicked:(id)sender {
    [self exit];
}

- (IBAction)seekChanged:(NSSlider *)sender {
    NSLog(@"");
    
}

- (void)exit {
    std::future<bool> futureDecoder = decoder->stop(false);
    audioRenderer->stop(false);
    audioOutput->stop(false);

    // futureDecoder.wait();
    decoder->unInit();
    decoder = nullptr;

    audioRenderer->uninit();
    audioRenderer = nullptr;

    audioOutput->uninit();
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
