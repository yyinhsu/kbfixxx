APP_NAME = kbfixxx
BUNDLE = $(APP_NAME).app
BUNDLE_ID = com.kbfixxx.app

CC = clang
OBJC = clang

CFLAGS = -Wall -Wextra -O2 -std=c11
OBJCFLAGS = -Wall -Wextra -O2 -fobjc-arc
FRAMEWORKS = -framework Cocoa -framework Carbon -framework ApplicationServices -framework ServiceManagement

SRC_CORE = src/core/config.c src/core/debouncer.c src/core/stats.c src/core/detector.c
SRC_VENDOR = src/vendor/cJSON.c
SRC_APP = src/app/main.m src/app/AppDelegate.m src/app/PreferenceWindowController.m \
          src/app/StatsWindowController.m src/app/LogWindowController.m

OBJS_CORE = $(SRC_CORE:.c=.o)
OBJS_VENDOR = $(SRC_VENDOR:.c=.o)
OBJS_APP = $(SRC_APP:.m=.o)
OBJS = $(OBJS_CORE) $(OBJS_VENDOR) $(OBJS_APP)

INCLUDES = -Isrc/core -Isrc/vendor -Isrc/app

# Default target: build the .app bundle
all: bundle

# Compile C sources
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile Objective-C sources
%.o: %.m
	$(OBJC) $(OBJCFLAGS) $(INCLUDES) -c $< -o $@

# Link
$(APP_NAME): $(OBJS)
	$(OBJC) $(OBJCFLAGS) $(FRAMEWORKS) $(OBJS) -o $@

# Create .app bundle
bundle: $(APP_NAME)
	@mkdir -p $(BUNDLE)/Contents/MacOS
	@mkdir -p $(BUNDLE)/Contents/Resources
	@cp $(APP_NAME) $(BUNDLE)/Contents/MacOS/$(APP_NAME)
	@cp resources/Info.plist $(BUNDLE)/Contents/Info.plist
	@[ -f resources/kbfixxx.icns ] && cp resources/kbfixxx.icns $(BUNDLE)/Contents/Resources/ || true
	@echo "Built $(BUNDLE)"

# Install default config if not present
install-config:
	@mkdir -p ~/.config/kbfixxx
	@[ -f ~/.config/kbfixxx/config.json ] || cp config.json ~/.config/kbfixxx/config.json
	@echo "Config installed at ~/.config/kbfixxx/config.json"

# Run the app
run: bundle install-config
	@open $(BUNDLE)

# Unit tests
TEST_SRC = tests/test_debouncer.c src/core/config.c src/core/stats.c src/core/detector.c src/vendor/cJSON.c
test: $(TEST_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -DTESTING $(TEST_SRC) -o test_runner
	./test_runner
	@rm -f test_runner

clean:
	rm -f $(OBJS) $(APP_NAME)
	rm -rf $(BUNDLE)
	rm -f test_runner

.PHONY: all bundle install-config run test clean
