#ifndef LOGWINDOWCONTROLLER_H
#define LOGWINDOWCONTROLLER_H

#import <Cocoa/Cocoa.h>
#include "debouncer.h"

@class AppDelegate;

@interface LogWindowController : NSWindowController

- (instancetype)initWithAppDelegate:(AppDelegate *)appDelegate;
- (void)appendLogEntry:(const LogEntry *)entry;

@end

#endif
