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

@property (weak) IBOutlet NSView *playerView;


@property (nonatomic, strong) NSTimer *timer;
@end



@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    preview = IPreviewManager::createIPreviewManager();
    preview->setParentViews((__bridge_retained void *)self.playerView);
    
    // FIXME: 这种写法会引入循环引用
    self.timer = [NSTimer scheduledTimerWithTimeInterval:1.0 / 60 target:self selector:@selector(refresh:) userInfo:self repeats:YES];
    [self loadVideo];
}

- (void)loadVideo {
     NSString *video = [[NSBundle mainBundle] pathForResource:@"Sync" ofType:@"mp4"];
//    NSString *video = [[NSBundle mainBundle] pathForResource:@"1：1" ofType:@"MOV"];
    
    decoder = std::make_shared<sp::DecoderManager>();
    decoder->init(video.UTF8String);
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
            if (auto pipeline = decoder->getNextFrame()) {
                if (pipeline->status == sp::Pipeline::EStatus::END_OF_FILE) {
                    [self exit];
                    return;
                }
                
                if (preview->render(pipeline) == true)
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
    preview = nullptr;
    [self.view.window.windowController close];
    __weak ViewController* wself = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 500 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
        __strong ViewController *sself = wself;
        [NSApp terminate:sself];
    });
}

@end
