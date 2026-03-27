#import "PreferenceWindowController.h"
#import "AppDelegate.h"
#include "keymap.h"
#include "config.h"

/* Row model for the table */
@interface KeyConfigRow : NSObject
@property (nonatomic, assign) int keycode;
@property (nonatomic, copy) NSString *keyName;
@property (nonatomic, assign) int delayMs;
@property (nonatomic, assign) int maxBounceCount;
@property (nonatomic, assign) BOOL enabled;
@end

@implementation KeyConfigRow
@end

@interface PreferenceWindowController ()
@property (nonatomic, weak) AppDelegate *appDelegate;
@property (nonatomic, strong) NSTableView *tableView;
@property (nonatomic, strong) NSMutableArray<KeyConfigRow *> *rows;
@property (nonatomic, strong) NSButton *detectKeyButton;
@property (nonatomic, assign) BOOL detectingKey;
@property (nonatomic, strong) id localMonitor;
@end

@implementation PreferenceWindowController

- (instancetype)initWithAppDelegate:(AppDelegate *)appDelegate {
    NSWindow *window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 600, 500)
                  styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                            NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    window.title = @"kbfixxx — Configure Keys";
    window.minSize = NSMakeSize(500, 300);
    [window center];

    self = [super initWithWindow:window];
    if (self) {
        _appDelegate = appDelegate;
        _rows = [NSMutableArray array];
        _detectingKey = NO;
        [self setupUI];
        [self loadRows];
    }
    return self;
}

- (void)setupUI {
    NSView *content = self.window.contentView;

    /* Scroll view + table */
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 80, 600, 420)];
    scrollView.hasVerticalScroller = YES;
    scrollView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    _tableView = [[NSTableView alloc] initWithFrame:scrollView.bounds];
    _tableView.dataSource = self;
    _tableView.delegate = self;
    _tableView.rowHeight = 24;

    /* Columns */
    NSTableColumn *enabledCol = [[NSTableColumn alloc] initWithIdentifier:@"enabled"];
    enabledCol.title = @"✓";
    enabledCol.width = 30;
    enabledCol.minWidth = 30;
    enabledCol.maxWidth = 30;
    [_tableView addTableColumn:enabledCol];

    NSTableColumn *keyCol = [[NSTableColumn alloc] initWithIdentifier:@"key"];
    keyCol.title = @"Key";
    keyCol.width = 120;
    keyCol.minWidth = 80;
    [_tableView addTableColumn:keyCol];

    NSTableColumn *delayCol = [[NSTableColumn alloc] initWithIdentifier:@"delay"];
    delayCol.title = @"Delay (ms)";
    delayCol.width = 100;
    delayCol.minWidth = 80;
    [_tableView addTableColumn:delayCol];

    NSTableColumn *bounceCol = [[NSTableColumn alloc] initWithIdentifier:@"bounce"];
    bounceCol.title = @"Max Bounce";
    bounceCol.width = 100;
    bounceCol.minWidth = 80;
    [_tableView addTableColumn:bounceCol];

    NSTableColumn *codeCol = [[NSTableColumn alloc] initWithIdentifier:@"code"];
    codeCol.title = @"Key Code";
    codeCol.width = 80;
    codeCol.minWidth = 60;
    [_tableView addTableColumn:codeCol];

    scrollView.documentView = _tableView;
    [content addSubview:scrollView];

    /* Bottom toolbar */
    NSView *toolbar = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 600, 80)];
    toolbar.autoresizingMask = NSViewWidthSizable;

    /* Add Key button */
    NSButton *addBtn = [NSButton buttonWithTitle:@"Add Key" target:self action:@selector(addKey:)];
    addBtn.frame = NSMakeRect(10, 42, 80, 30);
    [toolbar addSubview:addBtn];

    /* Detect Key button */
    _detectKeyButton = [NSButton buttonWithTitle:@"Detect Key…" target:self action:@selector(detectKey:)];
    _detectKeyButton.frame = NSMakeRect(100, 42, 110, 30);
    [toolbar addSubview:_detectKeyButton];

    /* Remove Key button */
    NSButton *removeBtn = [NSButton buttonWithTitle:@"Remove" target:self action:@selector(removeKey:)];
    removeBtn.frame = NSMakeRect(220, 42, 80, 30);
    [toolbar addSubview:removeBtn];

    /* Save button */
    NSButton *saveBtn = [NSButton buttonWithTitle:@"Save & Apply" target:self action:@selector(saveConfig:)];
    saveBtn.frame = NSMakeRect(480, 8, 110, 30);
    saveBtn.bezelStyle = NSBezelStyleRounded;
    saveBtn.keyEquivalent = @"\r";
    [toolbar addSubview:saveBtn];

    /* Ignore external keyboard checkbox */
    NSButton *ignExtCheck = [NSButton checkboxWithTitle:@"Ignore External KB"
                                                target:self
                                                action:@selector(toggleIgnoreExternal:)];
    ignExtCheck.frame = NSMakeRect(320, 42, 160, 30);
    ignExtCheck.state = [_appDelegate configPtr]->ignore_external_keyboard ? NSControlStateValueOn : NSControlStateValueOff;
    ignExtCheck.tag = 100;
    [toolbar addSubview:ignExtCheck];

    [content addSubview:toolbar];
}

- (void)loadRows {
    [_rows removeAllObjects];
    Config *cfg = [_appDelegate configPtr];
    for (int i = 0; i < N_VIRTUAL_KEY; i++) {
        if (cfg->keys[i].delay_ms > 0 || cfg->keys[i].enabled) {
            KeyConfigRow *row = [[KeyConfigRow alloc] init];
            row.keycode = i;
            row.keyName = [NSString stringWithUTF8String:keycode_to_name(i)];
            row.delayMs = cfg->keys[i].delay_ms;
            row.maxBounceCount = cfg->keys[i].max_bounce_count;
            row.enabled = cfg->keys[i].enabled;
            [_rows addObject:row];
        }
    }
    [_tableView reloadData];
}

- (void)refreshTable {
    [self loadRows];
}

#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    (void)tableView;
    return (NSInteger)_rows.count;
}

#pragma mark - NSTableViewDelegate

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    (void)tableView;
    KeyConfigRow *item = _rows[(NSUInteger)row];
    NSString *identifier = tableColumn.identifier;

    if ([identifier isEqualToString:@"enabled"]) {
        NSButton *check = [NSButton checkboxWithTitle:@"" target:self action:@selector(enabledToggled:)];
        check.state = item.enabled ? NSControlStateValueOn : NSControlStateValueOff;
        check.tag = row;
        return check;
    }

    if ([identifier isEqualToString:@"key"]) {
        NSPopUpButton *popup = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
        popup.bordered = NO;
        popup.font = [NSFont systemFontOfSize:[NSFont systemFontSize]];
        for (int i = 0; i < N_VIRTUAL_KEY; i++) {
            if (keycode_names[i]) {
                NSString *title = [NSString stringWithUTF8String:keycode_names[i]];
                [popup addItemWithTitle:title];
                popup.lastItem.tag = i;
            }
        }
        [popup selectItemWithTitle:item.keyName];
        popup.tag = row;
        popup.target = self;
        popup.action = @selector(keyPopupChanged:);
        return popup;
    }

    NSTextField *cell = [NSTextField textFieldWithString:@""];
    cell.bordered = NO;
    cell.drawsBackground = NO;

    if ([identifier isEqualToString:@"delay"]) {
        cell.stringValue = [NSString stringWithFormat:@"%d", item.delayMs];
        cell.editable = YES;
        cell.tag = row;
        cell.delegate = (id<NSTextFieldDelegate>)self;
        cell.identifier = @"delay_field";
    } else if ([identifier isEqualToString:@"bounce"]) {
        cell.stringValue = [NSString stringWithFormat:@"%d", item.maxBounceCount];
        cell.editable = YES;
        cell.tag = row;
        cell.delegate = (id<NSTextFieldDelegate>)self;
        cell.identifier = @"bounce_field";
    } else if ([identifier isEqualToString:@"code"]) {
        cell.stringValue = [NSString stringWithFormat:@"0x%02X", item.keycode];
        cell.editable = YES;
        cell.tag = row;
        cell.delegate = (id<NSTextFieldDelegate>)self;
        cell.identifier = @"code_field";
        cell.textColor = NSColor.secondaryLabelColor;
    }

    return cell;
}

- (void)controlTextDidEndEditing:(NSNotification *)notification {
    NSTextField *field = notification.object;
    NSInteger row = field.tag;
    if (row < 0 || row >= (NSInteger)_rows.count) return;

    KeyConfigRow *item = _rows[(NSUInteger)row];
    int value = field.intValue;

    if ([field.identifier isEqualToString:@"delay_field"]) {
        if (value < 0) value = 0;
        if (value > 2000) value = 2000;
        item.delayMs = value;
    } else if ([field.identifier isEqualToString:@"bounce_field"]) {
        if (value < 1) value = 1;
        if (value > 10) value = 10;
        item.maxBounceCount = value;
    } else if ([field.identifier isEqualToString:@"code_field"]) {
        /* Parse hex value (with or without 0x prefix) */
        unsigned int hexVal = 0;
        NSString *text = [field.stringValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
        NSScanner *scanner = [NSScanner scannerWithString:text];
        [scanner scanHexInt:&hexVal];
        int keycode = (int)hexVal;
        if (keycode < 0) keycode = 0;
        if (keycode >= N_VIRTUAL_KEY) keycode = N_VIRTUAL_KEY - 1;
        item.keycode = keycode;
        item.keyName = [NSString stringWithUTF8String:keycode_to_name(keycode)];
        [_tableView reloadData];
    }
}

- (void)enabledToggled:(NSButton *)sender {
    NSInteger row = sender.tag;
    if (row < 0 || row >= (NSInteger)_rows.count) return;
    _rows[(NSUInteger)row].enabled = (sender.state == NSControlStateValueOn);
}

- (void)keyPopupChanged:(NSPopUpButton *)sender {
    NSInteger row = sender.tag;
    if (row < 0 || row >= (NSInteger)_rows.count) return;
    KeyConfigRow *item = _rows[(NSUInteger)row];
    item.keycode = (int)sender.selectedItem.tag;
    item.keyName = sender.selectedItem.title;
    /* Refresh to update the Key Code column */
    [_tableView reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)row]
                          columnIndexes:[NSIndexSet indexSetWithIndex:4]];
}

#pragma mark - Actions

- (void)addKey:(id)sender {
    (void)sender;
    /* Add a new row with default values — user picks key by Detect or manual code */
    KeyConfigRow *row = [[KeyConfigRow alloc] init];
    row.keycode = 0;
    row.keyName = @"a";
    row.delayMs = 60;
    row.maxBounceCount = 1;
    row.enabled = YES;
    [_rows addObject:row];
    [_tableView reloadData];
    [_tableView scrollRowToVisible:(NSInteger)_rows.count - 1];
}

- (void)detectKey:(id)sender {
    (void)sender;
    if (_detectingKey) {
        [self stopDetecting];
        return;
    }

    _detectingKey = YES;
    _detectKeyButton.title = @"Press a key…";

    __weak typeof(self) weakSelf = self;
    _localMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
                                                          handler:^NSEvent *(NSEvent *event) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf) return event;

        int keycode = (int)event.keyCode;
        [strongSelf stopDetecting];

        /* Check if this key already exists in the table */
        for (NSUInteger i = 0; i < strongSelf.rows.count; i++) {
            if (strongSelf.rows[i].keycode == keycode) {
                [strongSelf.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:i]
                                  byExtendingSelection:NO];
                [strongSelf.tableView scrollRowToVisible:(NSInteger)i];
                return nil;
            }
        }

        /* Add new row for this key */
        KeyConfigRow *row = [[KeyConfigRow alloc] init];
        row.keycode = keycode;
        row.keyName = [NSString stringWithUTF8String:keycode_to_name(keycode)];
        row.delayMs = 60;
        row.maxBounceCount = 1;
        row.enabled = YES;
        [strongSelf.rows addObject:row];
        [strongSelf.tableView reloadData];
        NSInteger newRow = (NSInteger)strongSelf.rows.count - 1;
        [strongSelf.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)newRow]
                          byExtendingSelection:NO];
        [strongSelf.tableView scrollRowToVisible:newRow];

        return nil; /* consume the event */
    }];
}

- (void)stopDetecting {
    if (_localMonitor) {
        [NSEvent removeMonitor:_localMonitor];
        _localMonitor = nil;
    }
    _detectingKey = NO;
    _detectKeyButton.title = @"Detect Key…";
}

- (void)removeKey:(id)sender {
    (void)sender;
    NSInteger row = _tableView.selectedRow;
    if (row < 0 || row >= (NSInteger)_rows.count) return;
    [_rows removeObjectAtIndex:(NSUInteger)row];
    [_tableView reloadData];
}

- (void)saveConfig:(id)sender {
    (void)sender;
    Config *cfg = [_appDelegate configPtr];

    /* Clear all key configs first */
    for (int i = 0; i < N_VIRTUAL_KEY; i++) {
        cfg->keys[i].delay_ms = 0;
        cfg->keys[i].max_bounce_count = 1;
        cfg->keys[i].enabled = false;
    }

    /* Apply rows */
    for (KeyConfigRow *row in _rows) {
        int kc = row.keycode;
        if (kc >= 0 && kc < N_VIRTUAL_KEY) {
            cfg->keys[kc].delay_ms = row.delayMs;
            cfg->keys[kc].max_bounce_count = row.maxBounceCount;
            cfg->keys[kc].enabled = row.enabled;
        }
    }

    /* Update ignore_external from checkbox */
    NSView *toolbar = self.window.contentView.subviews.lastObject;
    for (NSView *v in toolbar.subviews) {
        if (v.tag == 100 && [v isKindOfClass:[NSButton class]]) {
            cfg->ignore_external_keyboard = (((NSButton *)v).state == NSControlStateValueOn);
        }
    }

    /* Save to file */
    config_save(cfg, config_default_path());
    debouncer_reload_config([_appDelegate debouncerPtr]);
    [_appDelegate rebuildMenu];

    NSLog(@"kbfixxx: config saved and applied");

    /* Show confirmation to the user */
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Config Saved";
    alert.informativeText = [NSString stringWithFormat:@"Configuration saved and applied (%lu key(s) configured).",
                             (unsigned long)_rows.count];
    alert.alertStyle = NSAlertStyleInformational;
    [alert addButtonWithTitle:@"OK"];
    [alert beginSheetModalForWindow:self.window completionHandler:nil];
}

- (void)toggleIgnoreExternal:(NSButton *)sender {
    [_appDelegate configPtr]->ignore_external_keyboard = (sender.state == NSControlStateValueOn);
}

@end
