# iOS Build Guide for Gambler

This guide covers building Gambler for iOS/iPadOS as both a Standalone app and AUv3 plugin.

## Prerequisites

1. **macOS** with Xcode 14+ installed
2. **CMake 3.25+** (included with CLion or install via Homebrew)
3. **Apple Developer Account** (free account works for development, paid for distribution)
4. **iOS Device or Simulator** running iOS 12.0 or later

## Quick Start

### Using the Build Script (Recommended)

The easiest way to build for iOS is using the provided build script:

```bash
# Build for simulator (Debug)
./build-ios.sh

# Build for device (Debug)
./build-ios.sh --device

# Build for device (Release)
./build-ios.sh --release --device

# Clean build for simulator
./build-ios.sh --clean
```

### Manual CMake Configuration

If you prefer manual control or are using CLion:

```bash
# Configure for iOS Simulator
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake \
    -Bbuild-ios -GXcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DCMAKE_OSX_SYSROOT=iphonesimulator

# Build Standalone
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake \
    --build build-ios --config Debug --target Gambler2_Standalone \
    -- -sdk iphonesimulator -allowProvisioningUpdates

# Build AUv3
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake \
    --build build-ios --config Debug --target Gambler2_AUv3 \
    -- -sdk iphonesimulator -allowProvisioningUpdates
```

For device builds, replace `iphonesimulator` with `iphoneos`.

## Configuration Details

### CMakeLists.txt Configuration

The iOS configuration is automatically applied when building for iOS. Key settings:

- **Deployment Target**: iOS 12.0 (configurable in `cmake/PamplejuceIOS.cmake`)
- **Architectures**: 
  - Device: `arm64`
  - Simulator: `x86_64` and `arm64` (for Intel and Apple Silicon Macs)
- **Screen Orientations**: All orientations supported (portrait and landscape)
- **Bundle ID**: `com.ToneGarden.Gambler2` (set in `CMakeLists.txt`)

### Info.plist Customization

The iOS-specific Info.plist (`packaging/Info-iOS.plist`) includes:

- **Privacy Permissions**:
  - Microphone access (for audio input)
  - Bluetooth access (for Bluetooth MIDI)
  - Local network access (for network MIDI)
  
- **Background Modes**:
  - Audio (allows app to run in background)
  
- **File Sharing**:
  - Enabled for preset import/export via Files app
  
- **Document Types**:
  - Custom `.gamblerpreset` file type support

To customize these settings, edit `packaging/Info-iOS.plist` before building.

## Code Signing

### Development Builds

For development and testing on your own devices:

1. Open the generated Xcode project: `open build-ios/Gambler2.xcodeproj`
2. Select the target (Gambler2_Standalone or Gambler2_AUv3)
3. Go to "Signing & Capabilities"
4. Select your Team
5. Xcode will automatically create a provisioning profile

The build script uses `-allowProvisioningUpdates` to handle this automatically.

### Distribution Builds

For App Store or TestFlight distribution:

1. Set your Development Team ID in `cmake/PamplejuceIOS.cmake`:
   ```cmake
   set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "YOUR_TEAM_ID" CACHE STRING "Development team ID")
   ```

2. Or set it via command line:
   ```bash
   cmake -Bbuild-ios -GXcode \
       -DCMAKE_SYSTEM_NAME=iOS \
       -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="YOUR_TEAM_ID"
   ```

## Building in Xcode

After running CMake configuration, you can work directly in Xcode:

```bash
open build-ios/Gambler2.xcodeproj
```

### Running on Simulator

1. Select `Gambler2_Standalone` or `Gambler2_AUv3` scheme
2. Choose an iOS Simulator from the device menu
3. Click Run (⌘R)

### Running on Device

1. Connect your iPad/iPhone via USB
2. Select your device from the device menu
3. Click Run (⌘R)
4. On first run, you may need to trust the developer certificate on the device:
   - Settings → General → VPN & Device Management → Trust

### Testing AUv3 in GarageBand/AUM

**Important**: AUv3 plugins cannot run standalone - they must be loaded in a host app.

1. **Build (don't run) the AUv3 target**:
   - Select `Gambler2_AUv3` scheme in Xcode
   - Select your iPad as destination
   - Click **Build** (⌘B) - **NOT Run**
   - This registers the AUv3 with iOS

2. **Open a host app on your iPad**:
   - GarageBand (free, pre-installed)
   - AUM (paid, excellent for testing)
   - Or any other AUv3 host

3. **Load your plugin**:
   - **GarageBand**: Add track → Keyboard → Tap plugin button → Audio Units → "Gambler2" under "ToneGarden"
   - **AUM**: Add MIDI effect → Audio Units → Gambler2

4. **Debug if needed**:
   - In Xcode: Debug → Attach to Process → Wait for "Gambler2" to appear in the list
   - Set breakpoints and they'll trigger when the plugin is loaded

**Note**: If you try to "Run" the AUv3 target, Xcode may show an error about AudioPluginHost.app. This is expected - just Build instead of Run.

## Archiving for Distribution

To create an archive for App Store submission:

1. Build for device (not simulator)
2. Open Xcode project
3. Select "Any iOS Device" as destination
4. Product → Archive
5. Once complete, the Organizer window opens
6. Click "Distribute App" and follow the wizard

## Troubleshooting

### "No provisioning profile found"

- Open the project in Xcode and select your team in Signing & Capabilities
- Or use the `-allowProvisioningUpdates` flag (included in build script)

### "Library not found" errors when archiving

If you encounter missing static libraries during archiving, this is a known CMake/Xcode issue. The configuration includes workarounds, but if problems persist:

```cmake
set_target_properties(problematic_target PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "./")
```

### AUv3 not appearing in host apps

- Ensure the AUv3 was **built** (not run) at least once on the device
- Check that the bundle ID matches between Standalone and AUv3
- Force quit and restart the host app (GarageBand, AUM, etc.)
- Restart your iPad if still not appearing
- Check Console.app on Mac (with iPad connected) for any loading errors

### "AudioPluginHost.app is not a valid bundle" error

This is **expected** when trying to Run the AUv3 target. AUv3 plugins cannot run standalone.

**Solution**:
- Click **Build** (⌘B) instead of Run (⌘R)
- Then open a host app (GarageBag, AUM) to test the plugin
- Or use Debug → Attach to Process to debug after loading in a host

### Simulator build fails

- Ensure you're using Xcode 14+ with iOS 16+ SDK
- Try cleaning the build directory: `./build-ios.sh --clean`
- Check that you have the iOS Simulator installed in Xcode

## File Locations

After building, artifacts are located in:

```
build-ios/Gambler2_artefacts/Debug/
├── Standalone/
│   └── Gambler2.app          # Standalone iOS app
└── AUv3/
    └── Gambler2.appex        # AUv3 plugin bundle
```

## Customization

### Changing Deployment Target

Edit `cmake/PamplejuceIOS.cmake`:

```cmake
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum iOS deployment version" FORCE)
```

### Changing Supported Orientations

Edit `CMakeLists.txt` in the `juce_add_plugin` section:

```cmake
IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft
IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
```

### Adding Capabilities

Edit `packaging/Info-iOS.plist` to add additional capabilities or permissions.

## Next Steps

- Test on multiple iOS versions and device sizes
- Set up TestFlight for beta testing
- Configure App Store metadata
- Add app icons for iOS (in an .xcassets catalog)
- Consider adding iPad-specific UI optimizations

