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
    
    NSString *video = [[NSBundle mainBundle] pathForResource:@"Sync" ofType:@"mp4"];
    
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
        bool eof = false;
        if (auto frame = decoder->getNextFrame(eof); frame.has_value()) {
            preview->render(frame);
        } else if (eof == true) {
            decoder = nullptr;
        }
    }

    
    for(NSView *subView in self.view.subviews) {
//        [subView setFrame:self.view.bounds];
        [subView setNeedsDisplay:YES];
    }
}

@end
