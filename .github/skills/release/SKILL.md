---
name: release
description: 'Publish a new kbfixxx release. Use when: creating a release, bumping version, tagging, or publishing the app.'
---

# Release Process

## Steps

1. Update `CFBundleShortVersionString` in `resources/Info.plist` to the new version
2. Commit all changes to `main`
3. Create and push a version tag:
   ```sh
   git tag v<version>
   git push origin v<version>
   ```

## What Happens

The GitHub Actions workflow (`.github/workflows/release.yml`) will automatically:
- Build the app on macOS
- Run tests
- Zip `kbfixxx.app`
- Create a GitHub Release with `kbfixxx.app.zip` attached

## Version Convention

Follow semver: `v0.1.0`, `v0.2.0`, `v1.0.0`. The tag version must match `CFBundleShortVersionString` in `resources/Info.plist`.
