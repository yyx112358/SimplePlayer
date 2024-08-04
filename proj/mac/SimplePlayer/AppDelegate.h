//
//  AppDelegate.h
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/3.
//

#import <Cocoa/Cocoa.h>
#import <CoreData/CoreData.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>

@property (readonly, strong) NSPersistentContainer *persistentContainer;


@end

