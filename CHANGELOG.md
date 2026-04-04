# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Show Accessibility and Input Monitoring permission status in status bar menu with quick links to System Preferences

## [v0.6.0] - 2026-04-04

### Added
- Custom app icon and status bar icon

## [v0.5.0] - 2026-04-01

### Fixed
- Catch keyDown→keyDown bounce pattern (fixes double characters)
- Stop spamming event tap re-enable, show permission alert after 3 failures

### Docs
- Add accessibility troubleshooting guide, update how-it-works diagrams

## [v0.4.0] - 2026-04-01

### Fixed
- Simplify debouncer to prevent bounce leaks
- Remove `AUTO_REPEAT_IGNORE_THRESHOLD` that bypassed bounce detection
- Increase max delay limit to 10000ms and fix preference UI
- Prevent config wipe on reload failure and refresh preference UI state

## [v0.3.0] - 2026-03-27

### Changed
- Improve preference UI layout

### Docs
- Add build-from-source instructions

## [v0.2.0] - 2026-03-27

### Changed
- Lower minimum macOS to 10.13 (High Sierra)

## [v0.1.2] - 2026-03-27

### Fixed
- Build universal binary (x86_64 + arm64)

## [v0.1.1] - 2026-03-27

### Fixed
- Zip app bundle correctly in release workflow

## [v0.1.0] - 2026-03-27

### Added
- Initial release: keyboard debounce tool for macOS
- Preference UI for configuring debounce delay
- Installation docs and release workflow
