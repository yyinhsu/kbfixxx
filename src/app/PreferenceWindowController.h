#ifndef PREFERENCEWINDOWCONTROLLER_H
#define PREFERENCEWINDOWCONTROLLER_H

#import <Cocoa/Cocoa.h>

@class AppDelegate;

@interface PreferenceWindowController : NSWindowController <NSTableViewDataSource, NSTableViewDelegate>

- (instancetype)initWithAppDelegate:(AppDelegate *)appDelegate;
- (void)refreshTable;

@end

#endif
