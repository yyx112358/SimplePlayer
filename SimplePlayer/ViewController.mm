//
//  ViewController.m
//  SimplePlayer
//
//  Created by YangYixuan on 2023/5/29.
//

#import "ViewController.h"
#include "IPreviewManager.hpp"

@interface ViewController()

@property (nonatomic, assign) std::shared_ptr<IPreviewManager> previewManager;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.previewManager = IPreviewManager::createIPreviewManager();
    self.previewManager->setParentViews((__bridge void *)self.view);
//    self.preview = [[Preview_Mac alloc] initWithFrame:self.view.bounds];
//    [self.view addSubview:self.preview];
}


- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

//    [self.preview needsToDrawRect:self.view.bounds];
    
}


@end
