# Agents

## Release Process

To publish a new release, use the `/release` skill.

## Build Notes

- The Makefile supports an `ARCH_FLAGS` variable (default: empty, uses native arch).
- For releases, build with `ARCH_FLAGS="-arch x86_64 -arch arm64"` to produce a universal binary that works on both Intel and Apple Silicon Macs.
- The target audience uses pre-2017 MacBook butterfly keyboards (Intel), so x86_64 support is required.
- GitHub Actions `macos-latest` runs on Apple Silicon — always pass the universal arch flags in CI.
- Deployment target is macOS 10.13 (High Sierra). Any API newer than 10.13 must be wrapped in `@available()` with a fallback.
