#ifndef STATSWINDOWCONTROLLER_H
#define STATSWINDOWCONTROLLER_H

#import <Cocoa/Cocoa.h>

@class AppDelegate;

@interface StatsWindowController : NSWindowController <NSTableViewDataSource, NSTableViewDelegate>

- (instancetype)initWithAppDelegate:(AppDelegate *)appDelegate;
- (void)refreshStats;

@end

#endif
