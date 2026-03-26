#import "StatsWindowController.h"
#import "AppDelegate.h"
#import "PreferenceWindowController.h"
#include "keymap.h"
#include "stats.h"
#include "detector.h"

@interface StatsRow : NSObject
@property (nonatomic, assign) int keycode;
@property (nonatomic, copy) NSString *keyName;
@property (nonatomic, assign) uint64_t suppressed;
@property (nonatomic, assign) uint64_t total;
@end

@implementation StatsRow
@end

@interface SuggestionRow : NSObject
@property (nonatomic, assign) int keycode;
@property (nonatomic, copy) NSString *keyName;
@property (nonatomic, assign) int rapidCount;
@property (nonatomic, assign) int suggestedDelay;
@end

@implementation SuggestionRow
@end

@interface StatsWindowController ()
@property (nonatomic, weak) AppDelegate *appDelegate;
@property (nonatomic, strong) NSTableView *statsTable;
@property (nonatomic, strong) NSTableView *suggestionsTable;
@property (nonatomic, strong) NSMutableArray<StatsRow *> *statsRows;
@property (nonatomic, strong) NSMutableArray<SuggestionRow *> *suggestionRows;
@property (nonatomic, strong) NSTimer *refreshTimer;
@end

@implementation StatsWindowController

- (instancetype)initWithAppDelegate:(AppDelegate *)appDelegate {
    NSWindow *window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 550, 500)
                  styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                            NSWindowStyleMaskResizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    window.title = @"kbfixxx — Statistics";
    window.minSize = NSMakeSize(400, 300);
    [window center];

    self = [super initWithWindow:window];
    if (self) {
        _appDelegate = appDelegate;
        _statsRows = [NSMutableArray array];
        _suggestionRows = [NSMutableArray array];
        [self setupUI];
    }
    return self;
}

- (void)showWindow:(id)sender {
    [super showWindow:sender];
    /* Auto-refresh every 2 seconds */
    _refreshTimer = [NSTimer scheduledTimerWithTimeInterval:2.0
                                                     target:self
                                                   selector:@selector(refreshStats)
                                                   userInfo:nil
                                                    repeats:YES];
}

- (void)windowWillClose:(NSNotification *)notification {
    (void)notification;
    [_refreshTimer invalidate];
    _refreshTimer = nil;
}

- (void)setupUI {
    NSView *content = self.window.contentView;

    /* Stats label */
    NSTextField *statsLabel = [NSTextField labelWithString:@"Suppressed Events:"];
    statsLabel.frame = NSMakeRect(10, 470, 200, 20);
    statsLabel.font = [NSFont boldSystemFontOfSize:13];
    statsLabel.autoresizingMask = NSViewMinYMargin;
    [content addSubview:statsLabel];

    /* Stats table */
    NSScrollView *statsScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 200, 550, 270)];
    statsScroll.hasVerticalScroller = YES;
    statsScroll.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    _statsTable = [[NSTableView alloc] initWithFrame:statsScroll.bounds];
    _statsTable.dataSource = self;
    _statsTable.delegate = self;
    _statsTable.rowHeight = 22;
    _statsTable.tag = 1;

    NSTableColumn *keyCol = [[NSTableColumn alloc] initWithIdentifier:@"key"];
    keyCol.title = @"Key";
    keyCol.width = 120;
    [_statsTable addTableColumn:keyCol];

    NSTableColumn *suppCol = [[NSTableColumn alloc] initWithIdentifier:@"suppressed"];
    suppCol.title = @"Suppressed";
    suppCol.width = 100;
    [_statsTable addTableColumn:suppCol];

    NSTableColumn *totalCol = [[NSTableColumn alloc] initWithIdentifier:@"total"];
    totalCol.title = @"Total";
    totalCol.width = 100;
    [_statsTable addTableColumn:totalCol];

    NSTableColumn *rateCol = [[NSTableColumn alloc] initWithIdentifier:@"rate"];
    rateCol.title = @"Bounce Rate";
    rateCol.width = 100;
    [_statsTable addTableColumn:rateCol];

    statsScroll.documentView = _statsTable;
    [content addSubview:statsScroll];

    /* Suggestions label */
    NSTextField *sugLabel = [NSTextField labelWithString:@"Auto-Detected Bouncing Keys:"];
    sugLabel.frame = NSMakeRect(10, 170, 250, 20);
    sugLabel.font = [NSFont boldSystemFontOfSize:13];
    sugLabel.autoresizingMask = NSViewMaxYMargin;
    [content addSubview:sugLabel];

    /* Suggestions table */
    NSScrollView *sugScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 40, 550, 130)];
    sugScroll.hasVerticalScroller = YES;
    sugScroll.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;

    _suggestionsTable = [[NSTableView alloc] initWithFrame:sugScroll.bounds];
    _suggestionsTable.dataSource = self;
    _suggestionsTable.delegate = self;
    _suggestionsTable.rowHeight = 22;
    _suggestionsTable.tag = 2;

    NSTableColumn *sugKeyCol = [[NSTableColumn alloc] initWithIdentifier:@"key"];
    sugKeyCol.title = @"Key";
    sugKeyCol.width = 120;
    [_suggestionsTable addTableColumn:sugKeyCol];

    NSTableColumn *rapidCol = [[NSTableColumn alloc] initWithIdentifier:@"rapid"];
    rapidCol.title = @"Rapid Count";
    rapidCol.width = 100;
    [_suggestionsTable addTableColumn:rapidCol];

    NSTableColumn *sugDelayCol = [[NSTableColumn alloc] initWithIdentifier:@"suggested"];
    sugDelayCol.title = @"Suggested Delay";
    sugDelayCol.width = 120;
    [_suggestionsTable addTableColumn:sugDelayCol];

    sugScroll.documentView = _suggestionsTable;
    [content addSubview:sugScroll];

    /* Bottom buttons */
    NSButton *resetBtn = [NSButton buttonWithTitle:@"Reset Stats" target:self action:@selector(resetStats:)];
    resetBtn.frame = NSMakeRect(10, 5, 100, 30);
    resetBtn.autoresizingMask = NSViewMaxYMargin;
    [content addSubview:resetBtn];

    NSButton *applyBtn = [NSButton buttonWithTitle:@"Apply Suggestions" target:self action:@selector(applySuggestions:)];
    applyBtn.frame = NSMakeRect(380, 5, 150, 30);
    applyBtn.autoresizingMask = NSViewMaxYMargin;
    [content addSubview:applyBtn];
}

- (void)refreshStats {
    Stats *stats = [_appDelegate statsPtr];

    /* Build stats rows — only keys with any events */
    [_statsRows removeAllObjects];
    for (int i = 0; i < N_VIRTUAL_KEY; i++) {
        uint64_t supp = stats_get_suppressed(stats, i);
        uint64_t total = stats_get_total(stats, i);
        if (total > 0 || supp > 0) {
            StatsRow *row = [[StatsRow alloc] init];
            row.keycode = i;
            row.keyName = [NSString stringWithUTF8String:keycode_to_name(i)];
            row.suppressed = supp;
            row.total = total;
            [_statsRows addObject:row];
        }
    }
    /* Sort by suppressed count descending */
    [_statsRows sortUsingComparator:^NSComparisonResult(StatsRow *a, StatsRow *b) {
        if (a.suppressed > b.suppressed) return NSOrderedAscending;
        if (a.suppressed < b.suppressed) return NSOrderedDescending;
        return NSOrderedSame;
    }];
    [_statsTable reloadData];

    /* Build suggestions */
    [_suggestionRows removeAllObjects];
    KeySuggestion suggestions[N_VIRTUAL_KEY];
    double now = CFAbsoluteTimeGetCurrent();
    int count = detector_get_suggestions([_appDelegate detectorPtr], now, suggestions, N_VIRTUAL_KEY);
    for (int i = 0; i < count; i++) {
        SuggestionRow *row = [[SuggestionRow alloc] init];
        row.keycode = suggestions[i].keycode;
        row.keyName = [NSString stringWithUTF8String:keycode_to_name(suggestions[i].keycode)];
        row.rapidCount = suggestions[i].rapid_count;
        row.suggestedDelay = suggestions[i].suggested_delay_ms;
        [_suggestionRows addObject:row];
    }
    [_suggestionsTable reloadData];
}

#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    if (tableView.tag == 1) return (NSInteger)_statsRows.count;
    if (tableView.tag == 2) return (NSInteger)_suggestionRows.count;
    return 0;
}

#pragma mark - NSTableViewDelegate

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSString *identifier = tableColumn.identifier;
    NSTextField *cell = [NSTextField labelWithString:@""];
    cell.font = [NSFont monospacedDigitSystemFontOfSize:12 weight:NSFontWeightRegular];

    if (tableView.tag == 1) {
        StatsRow *item = _statsRows[(NSUInteger)row];
        if ([identifier isEqualToString:@"key"]) {
            cell.stringValue = item.keyName;
        } else if ([identifier isEqualToString:@"suppressed"]) {
            cell.stringValue = [NSString stringWithFormat:@"%llu", item.suppressed];
        } else if ([identifier isEqualToString:@"total"]) {
            cell.stringValue = [NSString stringWithFormat:@"%llu", item.total];
        } else if ([identifier isEqualToString:@"rate"]) {
            if (item.total > 0) {
                double rate = (double)item.suppressed / (double)item.total * 100.0;
                cell.stringValue = [NSString stringWithFormat:@"%.1f%%", rate];
            } else {
                cell.stringValue = @"—";
            }
        }
    } else if (tableView.tag == 2) {
        SuggestionRow *item = _suggestionRows[(NSUInteger)row];
        if ([identifier isEqualToString:@"key"]) {
            cell.stringValue = item.keyName;
        } else if ([identifier isEqualToString:@"rapid"]) {
            cell.stringValue = [NSString stringWithFormat:@"%d", item.rapidCount];
        } else if ([identifier isEqualToString:@"suggested"]) {
            cell.stringValue = [NSString stringWithFormat:@"%d ms", item.suggestedDelay];
        }
    }

    return cell;
}

#pragma mark - Actions

- (void)resetStats:(id)sender {
    (void)sender;
    stats_reset([_appDelegate statsPtr]);
    detector_reset([_appDelegate detectorPtr]);
    [self refreshStats];
}

- (void)applySuggestions:(id)sender {
    (void)sender;
    Config *cfg = [_appDelegate configPtr];

    for (SuggestionRow *sug in _suggestionRows) {
        int kc = sug.keycode;
        if (kc >= 0 && kc < N_VIRTUAL_KEY) {
            cfg->keys[kc].delay_ms = sug.suggestedDelay;
            cfg->keys[kc].max_bounce_count = 1;
            cfg->keys[kc].enabled = true;
        }
    }

    config_save(cfg, config_default_path());
    debouncer_reload_config([_appDelegate debouncerPtr]);
    [_appDelegate rebuildMenu];

    if (_appDelegate.preferenceController) {
        [_appDelegate.preferenceController refreshTable];
    }
}

@end
