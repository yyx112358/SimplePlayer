//
//  ViewController.m
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/3.
//

#import "ViewController.h"

#include "SPLog.h"
#include "SPGraphPreview.hpp"
#include <spdlog/spdlog.h>
extern "C" {
#include <libavformat/avformat.h>
}


@interface ViewController () {
    std::shared_ptr<sp::SPMediaModel> _model;
    std::shared_ptr<sp::SPGraphPreview> _previewGraph;
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
    
    int v = avformat_version();
    spdlog::info("Welcome to spdlog! {}", avformat_version());
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("%2H:%2M:%2S.%3e %!:%# [SimplePlayer] %v");
    SPDLOG_INFO("中文测试{}");
    
    _model = std::make_shared<sp::SPMediaModel>();
    NSString *video = [[NSBundle mainBundle] pathForResource:@"Sync" ofType:@"mp4"];
    _model->videoTracks.push_back({std::string(video.UTF8String)});
    
    _previewGraph = std::make_shared<sp::SPGraphPreview>();
    if (auto f = _previewGraph->updateModel(*_model, true);f.get() == false)
        SPASSERT_NOT_IMPL;
    _previewGraph->_parentPlayerView = (__bridge_retained void *)self.playerView;
    
    if (auto f = _previewGraph->init(true);f.get() == false)
        SPASSERT_NOT_IMPL;

    if (auto f = _previewGraph->start(true);f.get() == false)
        SPASSERT_NOT_IMPL;
    
//    NSString *video = [[NSBundle mainBundle] pathForResource:@"1：1" ofType:@"MOV"];
//
//    decoder = std::make_shared<sp::DecoderManager>();
//    decoder->init(video.UTF8String);
//    
//    audioRenderer = std::make_shared<sp::AudioRendererManager>();
//    audioRenderer->init();
//    audioRenderer->setInputQueue(decoder->_audioQueue);
//    
//    audioOutput = std::make_shared<sp::AudioOutputManager>();
//    audioOutput->init();
//    audioOutput->setInputQueue(audioRenderer->getOutputQueue());
//    
//    preview = IPreviewManager::createIPreviewManager();
//    preview->setParentViews((__bridge_retained void *)self.playerView);
//    preview->setPipelineQueue(decoder->_videoQueue);
//    
//    decoder->start(false);
//    audioRenderer->start(false);
//    audioOutput->start(false);
//    preview->start(false);
    
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
//        _previewGraph->pause(true);
//        [sender setTitle:@"⏸"];
    } else {
//        _previewGraph->start(true);
//        [sender setTitle:@"▶️"];
    }
}

- (IBAction)stopClicked:(id)sender {
    [self exit];
}

- (IBAction)seekChanged:(NSSlider *)sender {
    NSLog(@"");
    
}

- (void)exit {
//    std::future<bool> futureDecoder = decoder->stop(false);
//    audioRenderer->stop(false);
//    audioOutput->stop(false);
//
//    // futureDecoder.wait();
//    decoder->unInit();
//    decoder = nullptr;
//
//    audioRenderer->uninit();
//    audioRenderer = nullptr;
//
//    audioOutput->uninit();
//    audioOutput = nullptr;
//
//    preview = nullptr;
//    _previewGraph->uninit(true);
//    _previewGraph = nullptr;
//    _model = nullptr;
//
//    [self.view.window.windowController close];
//    __weak ViewController* wself = self;
//    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 500 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
//        __strong ViewController *sself = wself;
//        [NSApp terminate:sself];
//    });
}

@end
