#import "LogWindowController.h"
#import "AppDelegate.h"
#include "keymap.h"
#include <mach/mach_time.h>

#define MAX_LOG_LINES 2000

@interface LogWindowController ()
@property (nonatomic, weak) AppDelegate *appDelegate;
@property (nonatomic, strong) NSTextView *textView;
@property (nonatomic, assign) NSInteger lineCount;
@property (nonatomic, strong) NSButton *autoScrollCheck;
@property (nonatomic, strong) NSButton *suppressedOnlyCheck;
@end

@implementation LogWindowController

- (instancetype)initWithAppDelegate:(AppDelegate *)appDelegate {
    NSWindow *window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 700, 400)
                  styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                            NSWindowStyleMaskResizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    window.title = @"kbfixxx — Event Log";
    window.minSize = NSMakeSize(500, 200);
    [window center];

    self = [super initWithWindow:window];
    if (self) {
        _appDelegate = appDelegate;
        _lineCount = 0;
        [self setupUI];
    }
    return self;
}

- (void)setupUI {
    NSView *content = self.window.contentView;

    /* Toolbar */
    NSView *toolbar = [[NSView alloc] initWithFrame:NSMakeRect(0, 370, 700, 30)];
    toolbar.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;

    _autoScrollCheck = [NSButton checkboxWithTitle:@"Auto-scroll"
                                            target:nil
                                            action:nil];
    _autoScrollCheck.frame = NSMakeRect(10, 2, 120, 24);
    _autoScrollCheck.state = NSControlStateValueOn;
    [toolbar addSubview:_autoScrollCheck];

    _suppressedOnlyCheck = [NSButton checkboxWithTitle:@"Suppressed only"
                                                target:nil
                                                action:nil];
    _suppressedOnlyCheck.frame = NSMakeRect(140, 2, 150, 24);
    _suppressedOnlyCheck.state = NSControlStateValueOff;
    [toolbar addSubview:_suppressedOnlyCheck];

    NSButton *clearBtn = [NSButton buttonWithTitle:@"Clear" target:self action:@selector(clearLog:)];
    clearBtn.frame = NSMakeRect(620, 2, 60, 24);
    clearBtn.autoresizingMask = NSViewMinXMargin;
    [toolbar addSubview:clearBtn];

    [content addSubview:toolbar];

    /* Text view */
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 700, 370)];
    scrollView.hasVerticalScroller = YES;
    scrollView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    _textView = [[NSTextView alloc] initWithFrame:scrollView.bounds];
    _textView.editable = NO;
    if (@available(macOS 10.15, *)) {
        _textView.font = [NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
    } else {
        _textView.font = [NSFont fontWithName:@"Menlo" size:11];
    }
    _textView.backgroundColor = [NSColor colorWithWhite:0.05 alpha:1.0];
    _textView.textColor = [NSColor colorWithWhite:0.9 alpha:1.0];
    _textView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    /* Add header */
    NSString *header = @"Timestamp          Key          Event     Action      Elapsed\n"
                       @"─────────────────────────────────────────────────────────────\n";
    [_textView.textStorage appendAttributedString:
        [[NSAttributedString alloc] initWithString:header
                                        attributes:@{
            NSFontAttributeName: _textView.font,
            NSForegroundColorAttributeName: [NSColor colorWithWhite:0.6 alpha:1.0]
        }]];

    scrollView.documentView = _textView;
    [content addSubview:scrollView];
}

- (void)appendLogEntry:(const LogEntry *)entry {
    if (!self.window.isVisible) return;

    /* Filter if "suppressed only" is checked */
    if (_suppressedOnlyCheck.state == NSControlStateValueOn && !entry->suppressed) return;

    const char *keyName = keycode_to_name(entry->keycode);
    const char *eventType = (entry->event_type == 10) ? "keyDown" :
                            (entry->event_type == 11) ? "keyUp  " : "flags  ";
    const char *action = entry->suppressed ? "BLOCKED" : "passed ";

    /* Format timestamp as HH:MM:SS.mmm */
    double ts = entry->timestamp;
    /* CFAbsoluteTime is seconds since 2001-01-01, convert to time of day */
    time_t epochSecs = (time_t)(ts + 978307200.0); /* CF epoch to Unix epoch */
    struct tm tm;
    localtime_r(&epochSecs, &tm);
    int millis = (int)((ts - (long)ts) * 1000) % 1000;
    if (millis < 0) millis += 1000;

    char line[256];
    snprintf(line, sizeof(line), "%02d:%02d:%02d.%03d   %-12s %-9s %-11s %.1f ms\n",
             tm.tm_hour, tm.tm_min, tm.tm_sec, millis,
             keyName, eventType, action, entry->elapsed_ms);

    NSColor *textColor;
    if (entry->suppressed) {
        textColor = [NSColor systemRedColor];
    } else {
        textColor = [NSColor colorWithWhite:0.8 alpha:1.0];
    }

    NSAttributedString *attrStr = [[NSAttributedString alloc]
        initWithString:[NSString stringWithUTF8String:line]
            attributes:@{
                NSFontAttributeName: _textView.font,
                NSForegroundColorAttributeName: textColor
            }];

    [_textView.textStorage appendAttributedString:attrStr];
    _lineCount++;

    /* Prune old lines */
    if (_lineCount > MAX_LOG_LINES) {
        NSString *text = _textView.textStorage.string;
        NSRange firstNewline = [text rangeOfString:@"\n"];
        if (firstNewline.location != NSNotFound) {
            [_textView.textStorage deleteCharactersInRange:NSMakeRange(0, firstNewline.location + 1)];
            _lineCount--;
        }
    }

    /* Auto-scroll */
    if (_autoScrollCheck.state == NSControlStateValueOn) {
        [_textView scrollRangeToVisible:NSMakeRange(_textView.textStorage.length, 0)];
    }
}

- (void)clearLog:(id)sender {
    (void)sender;
    [_textView.textStorage setAttributedString:[[NSAttributedString alloc] initWithString:@""]];
    _lineCount = 0;
}

@end
