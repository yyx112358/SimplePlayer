//
//  ViewController.m
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/3.
//

#import "ViewController.h"
#import "IPreviewManager.hpp"


extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}


@interface ViewController ()

@property (nonatomic, assign) std::shared_ptr<IPreviewManager> preview;
@property (nonatomic, strong) NSTimer *timer;

@end


@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.preview = IPreviewManager::createIPreviewManager();
    self.preview->setParentViews((__bridge_retained void *)self.view);
    
    // FIXME: 这种写法会引入循环引用
    self.timer = [NSTimer scheduledTimerWithTimeInterval:1.0 / 30 target:self selector:@selector(refresh:) userInfo:self repeats:YES];
    NSLog(@"%s", avformat_configuration()) ;
    
    av_log_set_level(AV_LOG_VERBOSE);
    AVFormatContext *inputCtx = nullptr;
    int ret = avformat_open_input(&inputCtx, "../../DevelopDiary/SimplePlayer/Videos/Sync.mp4", nullptr, nullptr);
}


- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}

- (void)viewDidLayout {
    for(NSView *subView in self.view.subviews) {
        [subView setFrame:self.view.bounds];
//        [subView setNeedsDisplay:YES];
    }
}

- (void)refresh:(id)obj {
    for(NSView *subView in self.view.subviews) {
//        [subView setFrame:self.view.bounds];
        [subView setNeedsDisplay:YES];
    }
}

@end
