//
//  ViewController.m
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/3.
//

#import "ViewController.h"
#import "IPreviewManager.hpp"

#include "SPLog.h"
#include "DecoderManager.hpp"


@interface ViewController () {
    std::shared_ptr<IPreviewManager> preview;
    std::shared_ptr<sp::DecoderManager> decoder;
}

@property (nonatomic, strong) NSTimer *timer;
@end



@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    preview = IPreviewManager::createIPreviewManager();
    preview->setParentViews((__bridge_retained void *)self.view);
    
    // FIXME: 这种写法会引入循环引用
    self.timer = [NSTimer scheduledTimerWithTimeInterval:1.0 / 30 target:self selector:@selector(refresh:) userInfo:self repeats:YES];
    [self loadVideo];
}

- (void)loadVideo {
    
    NSString *video = [[NSBundle mainBundle] pathForResource:@"1：1" ofType:@"MOV"];
    
    decoder = std::make_shared<sp::DecoderManager>();
    decoder->init(video.UTF8String);
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
    if (decoder != nullptr) {
        int cnt = 10;
        do {
            if (auto frame = decoder->getNextFrame(); frame.has_value()) {
                preview->render(frame);
                break;
            }
        } while(--cnt >= 0);
        if (cnt < 0) {
            decoder = nullptr;
            preview = nullptr;
            [self.view.window.windowController close];
            __weak ViewController* wself = self;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 500 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
                __strong ViewController *sself = wself;
                [NSApp terminate:sself];
            });
        }
    }

    
    for(NSView *subView in self.view.subviews) {
//        [subView setFrame:self.view.bounds];
        [subView setNeedsDisplay:YES];
    }
}

@end
