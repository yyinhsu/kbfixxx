#ifndef APPDELEGATE_H
#define APPDELEGATE_H

#import <Cocoa/Cocoa.h>
#include "debouncer.h"
#include "config.h"
#include "stats.h"
#include "detector.h"

@class PreferenceWindowController;
@class StatsWindowController;
@class LogWindowController;

@interface AppDelegate : NSObject <NSApplicationDelegate>

@property (nonatomic, assign) Debouncer debouncer;
@property (nonatomic, assign) Config config;
@property (nonatomic, assign) Stats stats;
@property (nonatomic, assign) Detector detector;
@property (nonatomic, assign) BOOL isEnabled;

@property (nonatomic, strong) NSStatusItem *statusItem;
@property (nonatomic, strong) PreferenceWindowController *preferenceController;
@property (nonatomic, strong) StatsWindowController *statsController;
@property (nonatomic, strong) LogWindowController *logController;

- (void)reloadConfig;
- (void)rebuildMenu;

/* Pointer accessors for child controllers to modify structs in-place */
- (Config *)configPtr;
- (Debouncer *)debouncerPtr;
- (Stats *)statsPtr;
- (Detector *)detectorPtr;

@end

#endif /* APPDELEGATE_H */
