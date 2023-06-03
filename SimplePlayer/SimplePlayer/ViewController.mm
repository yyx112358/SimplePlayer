//
//  ViewController.m
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/3.
//

#import "ViewController.h"
#import "IPreviewManager.hpp"

@interface ViewController ()

@property (nonatomic, assign) std::shared_ptr<IPreviewManager> preview;

@end


@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.preview = IPreviewManager::createIPreviewManager();
    self.preview->setParentViews((__bridge_retained void *)self.view);
}


- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}


@end
