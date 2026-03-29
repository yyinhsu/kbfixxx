#import "AppDelegate.h"
#import "PreferenceWindowController.h"
#import "StatsWindowController.h"
#import "LogWindowController.h"
#include "keymap.h"
#include <sys/stat.h>

/* FSEvents callback for config file watching */
static void fsevents_callback(ConstFSEventStreamRef streamRef,
                               void *clientCallBackInfo,
                               size_t numEvents,
                               void *eventPaths,
                               const FSEventStreamEventFlags eventFlags[],
                               const FSEventStreamEventId eventIds[]) {
    (void)streamRef; (void)numEvents; (void)eventPaths;
    (void)eventFlags; (void)eventIds;
    AppDelegate *delegate = (__bridge AppDelegate *)clientCallBackInfo;
    dispatch_async(dispatch_get_main_queue(), ^{
        [delegate reloadConfig];
    });
}

@implementation AppDelegate {
    FSEventStreamRef _fsEventStream;
    NSTimer *_eventTapCheckTimer;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;

    /* Initialize core components */
    config_init_defaults(&_config);
    stats_init(&_stats);
    detector_init(&_detector);

    /* Load config or create default */
    const char *cfg_path = config_default_path();
    [self ensureConfigDirExists];

    if (config_load(&_config, cfg_path) != 0) {
        /* Create default config if it doesn't exist */
        [self createDefaultConfig];
        config_load(&_config, cfg_path);
    }

    /* Initialize debouncer */
    debouncer_init(&_debouncer, &_config, &_stats);
    _isEnabled = YES;

    /* Set up log callback */
    debouncer_set_log_callback(&_debouncer, log_event_callback, (__bridge void *)self);

    /* Set up event tap */
    if (!debouncer_setup_event_tap(&_debouncer)) {
        [self showAccessibilityAlert];
    }

    /* Set up menu bar */
    [self setupStatusItem];

    /* Watch config file for changes */
    [self startConfigFileWatcher];

    /* Periodically check event tap health */
    _eventTapCheckTimer = [NSTimer scheduledTimerWithTimeInterval:5.0
                                                           target:self
                                                         selector:@selector(checkEventTap)
                                                         userInfo:nil
                                                          repeats:YES];
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    (void)notification;
    [self stopConfigFileWatcher];
    [_eventTapCheckTimer invalidate];
    debouncer_remove_event_tap(&_debouncer);
}

#pragma mark - Status Bar

- (void)setupStatusItem {
    _statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];

    /* Use SF Symbol for the icon (keyboard) */
    if (@available(macOS 11.0, *)) {
        NSImage *image = [NSImage imageWithSystemSymbolName:@"keyboard.badge.ellipsis"
                                  accessibilityDescription:@"kbfixxx"];
        if (image) {
            [image setTemplate:YES];
            _statusItem.button.image = image;
        } else {
            _statusItem.button.title = @"KB";
        }
    } else {
        _statusItem.button.title = @"KB";
    }

    [self rebuildMenu];
}

- (void)rebuildMenu {
    NSMenu *menu = [[NSMenu alloc] init];

    /* Enable/Disable toggle */
    NSString *toggleTitle = _isEnabled ? @"✓ Enabled" : @"✗ Disabled";
    NSMenuItem *toggleItem = [[NSMenuItem alloc] initWithTitle:toggleTitle
                                                       action:@selector(toggleEnabled:)
                                                keyEquivalent:@""];
    toggleItem.target = self;
    [menu addItem:toggleItem];

    [menu addItem:[NSMenuItem separatorItem]];

    /* Configure */
    NSMenuItem *configItem = [[NSMenuItem alloc] initWithTitle:@"Configure…"
                                                       action:@selector(showPreferences:)
                                                keyEquivalent:@","];
    configItem.target = self;
    [menu addItem:configItem];

    /* Statistics */
    NSMenuItem *statsItem = [[NSMenuItem alloc] initWithTitle:@"Statistics"
                                                      action:@selector(showStats:)
                                                keyEquivalent:@""];
    statsItem.target = self;
    [menu addItem:statsItem];

    /* Log */
    NSMenuItem *logItem = [[NSMenuItem alloc] initWithTitle:@"Event Log"
                                                    action:@selector(showLog:)
                                               keyEquivalent:@""];
    logItem.target = self;
    [menu addItem:logItem];

    [menu addItem:[NSMenuItem separatorItem]];

    /* Reload Config */
    NSMenuItem *reloadItem = [[NSMenuItem alloc] initWithTitle:@"Reload Config"
                                                       action:@selector(reloadConfigAction:)
                                                keyEquivalent:@"r"];
    reloadItem.target = self;
    [menu addItem:reloadItem];

    /* Open Config File */
    NSMenuItem *openConfigItem = [[NSMenuItem alloc] initWithTitle:@"Open Config File"
                                                           action:@selector(openConfigFile:)
                                                    keyEquivalent:@""];
    openConfigItem.target = self;
    [menu addItem:openConfigItem];

    [menu addItem:[NSMenuItem separatorItem]];

    /* Event Tap Status */
    BOOL tapOK = debouncer_event_tap_enabled(&_debouncer);
    NSString *tapStatus = tapOK ? @"Event Tap: Active" : @"Event Tap: Inactive ⚠️";
    NSMenuItem *tapItem = [[NSMenuItem alloc] initWithTitle:tapStatus
                                                    action:nil
                                             keyEquivalent:@""];
    tapItem.enabled = NO;
    [menu addItem:tapItem];

    /* Configured keys count */
    int configured = 0;
    for (int i = 0; i < N_VIRTUAL_KEY; i++) {
        if (_config.keys[i].enabled && _config.keys[i].delay_ms > 0) configured++;
    }
    NSString *keysInfo = [NSString stringWithFormat:@"Configured Keys: %d", configured];
    NSMenuItem *keysItem = [[NSMenuItem alloc] initWithTitle:keysInfo
                                                     action:nil
                                              keyEquivalent:@""];
    keysItem.enabled = NO;
    [menu addItem:keysItem];

    [menu addItem:[NSMenuItem separatorItem]];

    /* Quit */
    NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit kbfixxx"
                                                     action:@selector(quitApp:)
                                              keyEquivalent:@"q"];
    quitItem.target = self;
    [menu addItem:quitItem];

    _statusItem.menu = menu;
}

#pragma mark - Menu Actions

- (void)toggleEnabled:(id)sender {
    (void)sender;
    _isEnabled = !_isEnabled;
    debouncer_set_disabled(&_debouncer, !_isEnabled);
    [self rebuildMenu];
}

- (void)showPreferences:(id)sender {
    (void)sender;
    if (!_preferenceController) {
        _preferenceController = [[PreferenceWindowController alloc] initWithAppDelegate:self];
    }
    [_preferenceController showWindow:nil];
    [_preferenceController refreshTable];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)showStats:(id)sender {
    (void)sender;
    if (!_statsController) {
        _statsController = [[StatsWindowController alloc] initWithAppDelegate:self];
    }
    [_statsController showWindow:nil];
    [_statsController refreshStats];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)showLog:(id)sender {
    (void)sender;
    if (!_logController) {
        _logController = [[LogWindowController alloc] initWithAppDelegate:self];
    }
    [_logController showWindow:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)reloadConfigAction:(id)sender {
    (void)sender;
    [self reloadConfig];
}

- (void)openConfigFile:(id)sender {
    (void)sender;
    NSString *path = [NSString stringWithUTF8String:config_default_path()];
    [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:path]];
}

- (void)quitApp:(id)sender {
    (void)sender;
    [NSApp terminate:nil];
}

#pragma mark - Config Management

- (void)reloadConfig {
    const char *cfg_path = config_default_path();
    if (config_load(&_config, cfg_path) == 0) {
        debouncer_reload_config(&_debouncer);
        [self rebuildMenu];
        NSLog(@"kbfixxx: config reloaded");
    }
}

- (void)ensureConfigDirExists {
    const char *home = getenv("HOME");
    if (!home) return;
    char dir[1024];
    snprintf(dir, sizeof(dir), "%s/.config/kbfixxx", home);
    mkdir(dir, 0755);
}

- (void)createDefaultConfig {
    const char *cfg_path = config_default_path();

    /* Create a minimal default config */
    config_init_defaults(&_config);
    config_save(&_config, cfg_path);
    NSLog(@"kbfixxx: created default config at %s", cfg_path);
}

#pragma mark - Config File Watcher

- (void)startConfigFileWatcher {
    const char *home = getenv("HOME");
    if (!home) return;
    char dir[1024];
    snprintf(dir, sizeof(dir), "%s/.config/kbfixxx", home);
    NSString *watchPath = [NSString stringWithUTF8String:dir];

    NSArray *paths = @[watchPath];
    FSEventStreamContext context = {0, (__bridge void *)self, NULL, NULL, NULL};

    _fsEventStream = FSEventStreamCreate(kCFAllocatorDefault,
                                          fsevents_callback,
                                          &context,
                                          (__bridge CFArrayRef)paths,
                                          kFSEventStreamEventIdSinceNow,
                                          1.0,
                                          kFSEventStreamCreateFlagFileEvents);

    if (_fsEventStream) {
        FSEventStreamSetDispatchQueue(_fsEventStream, dispatch_get_main_queue());
        FSEventStreamStart(_fsEventStream);
    }
}

- (void)stopConfigFileWatcher {
    if (_fsEventStream) {
        FSEventStreamStop(_fsEventStream);
        FSEventStreamInvalidate(_fsEventStream);
        FSEventStreamRelease(_fsEventStream);
        _fsEventStream = NULL;
    }
}

#pragma mark - Event Tap Health Check

- (void)checkEventTap {
    if (!_isEnabled) return;
    if (!debouncer_event_tap_enabled(&_debouncer)) {
        NSLog(@"kbfixxx: event tap was disabled, attempting to re-enable");
        debouncer_remove_event_tap(&_debouncer);
        if (!debouncer_setup_event_tap(&_debouncer)) {
            NSLog(@"kbfixxx: failed to re-enable event tap");
        }
        [self rebuildMenu];
    }
}

- (void)showAccessibilityAlert {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Accessibility Permission Required";
        alert.informativeText = @"kbfixxx needs Accessibility permission to intercept keyboard events.\n\n"
                                @"Go to System Settings → Privacy & Security → Accessibility, "
                                @"and add kbfixxx.\n\n"
                                @"You may also need to add it to Input Monitoring.";
        alert.alertStyle = NSAlertStyleWarning;
        [alert addButtonWithTitle:@"Open System Settings"];
        [alert addButtonWithTitle:@"Quit"];

        NSModalResponse response = [alert runModal];
        if (response == NSAlertFirstButtonReturn) {
            NSURL *url = [NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"];
            [[NSWorkspace sharedWorkspace] openURL:url];
        } else {
            [NSApp terminate:nil];
        }
    });
}

#pragma mark - Pointer Accessors

- (Config *)configPtr { return &_config; }
- (Debouncer *)debouncerPtr { return &_debouncer; }
- (Stats *)statsPtr { return &_stats; }
- (Detector *)detectorPtr { return &_detector; }

#pragma mark - Log Callback

static void log_event_callback(const LogEntry *entry, void *context) {
    AppDelegate *delegate = (__bridge AppDelegate *)context;
    if (delegate.logController) {
        /* Forward to log window on main thread */
        LogEntry entryCopy = *entry;
        dispatch_async(dispatch_get_main_queue(), ^{
            [delegate.logController appendLogEntry:&entryCopy];
        });
    }

    /* Also feed to detector for auto-detection */
    if (entry->event_type == 10 /* kCGEventKeyDown */) {
        detector_record_event(&(delegate->_detector), entry->keycode, entry->timestamp);
    }
}

@end
