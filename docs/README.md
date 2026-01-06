# Gambler Documentation

Welcome to the Gambler documentation! This directory contains guides for building and developing Gambler across different platforms.

## Documentation Index

### iOS Development
- **[iOS Build Guide](iOS-Build-Guide.md)** - Comprehensive guide for building Gambler for iOS/iPadOS
- **[CLion iOS Setup](CLion-iOS-Setup.md)** - Setting up CLion for iOS development
- **[iOS Quick Reference](iOS-Quick-Reference.md)** - Quick commands and common tasks

### Architecture
- **[CLAUDE.md](../CLAUDE.md)** - Comprehensive architecture documentation for the codebase

## Quick Start

### macOS Development
```bash
# Configure
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake -Bcmake-build-debug

# Build standalone
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake --build cmake-build-debug --target Gambler2_Standalone -j 8

# Run
./cmake-build-debug/Gambler2_artefacts/Debug/Standalone/Gambler.app/Contents/MacOS/Gambler
```

### iOS Development
```bash
# Build for simulator
./build-ios.sh

# Build for device
./build-ios.sh --device

# Open in Xcode
./open-xcode.sh
```

## Project Structure

```
Gambler/
├── source/              # Source code
│   ├── backend/        # Domain logic (thread-safe)
│   ├── ui/             # UI components
│   └── data/           # Parameter definitions
├── cmake/              # CMake configuration files
│   ├── PamplejuceIOS.cmake    # iOS-specific config
│   └── PamplejuceMacOS.cmake  # macOS-specific config
├── packaging/          # Build artifacts and resources
│   └── Info-iOS.plist  # iOS Info.plist template
├── docs/               # Documentation (you are here)
├── build-ios.sh        # iOS build script
└── open-xcode.sh       # Quick Xcode launcher
```

## Key Features

- **Standalone App**: Full-featured MIDI sequencer
- **AUv3 Plugin**: Use in GarageBand, AUM, and other iOS hosts
- **Cross-Platform**: macOS and iOS support
- **CMake-Based**: Minimal Xcode configuration needed

## Development Workflow

1. **Edit code** in CLion (excellent C++ support)
2. **Build** using provided scripts or CLion
3. **Test** in Xcode on simulator/device
4. **Debug** using Xcode's tools when needed

## Configuration Files

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Main build configuration |
| `cmake/PamplejuceIOS.cmake` | iOS deployment target, architectures |
| `packaging/Info-iOS.plist` | iOS permissions, capabilities |
| `VERSION` | Version number (auto-incremented) |

## Common Customizations

### Change App Name
Edit `CMakeLists.txt`:
```cmake
set(PRODUCT_NAME "YourAppName")
```

### Change Bundle ID
Edit `CMakeLists.txt`:
```cmake
set(BUNDLE_ID "com.YourCompany.YourApp")
```

### Change iOS Deployment Target
Edit `cmake/PamplejuceIOS.cmake`:
```cmake
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" ...)
```

### Add iOS Permissions
Edit `packaging/Info-iOS.plist` to add privacy descriptions.

## Building for Distribution

### macOS
```bash
# Build Release
cmake -Bcmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --target Gambler2_Standalone -j 8

# Create installer (if configured)
# See packaging/ directory
```

### iOS
```bash
# Build Release for device
./build-ios.sh --release --device

# Then in Xcode:
# 1. Open project: ./open-xcode.sh
# 2. Select "Any iOS Device"
# 3. Product → Archive
# 4. Distribute via Organizer
```

## Testing

### Unit Tests
```bash
./cmake-build-debug/Tests --reporter rspec
```

### Manual Testing
- **macOS**: Run standalone app directly
- **iOS Simulator**: Build and run in Xcode
- **iOS Device**: Connect device and run in Xcode
- **AUv3**: Test in host apps (GarageBand, AUM, etc.)

## Troubleshooting

See the specific guides for detailed troubleshooting:
- [iOS Build Guide - Troubleshooting](iOS-Build-Guide.md#troubleshooting)
- [CLion iOS Setup - Troubleshooting](CLion-iOS-Setup.md#troubleshooting)

## Resources

- [JUCE Framework](https://juce.com)
- [JUCE CMake API](../JUCE/docs/CMake%20API.md)
- [Pamplejuce Template](https://github.com/sudara/pamplejuce)
- [Apple Developer](https://developer.apple.com)

## Contributing

When making changes:
1. Update relevant documentation
2. Test on both macOS and iOS
3. Run unit tests
4. Update version in `VERSION` file if needed

## License

See the main repository LICENSE file.

