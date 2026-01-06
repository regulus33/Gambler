# iOS Quick Reference

Quick commands and tips for iOS development with Gambler.

## Build Commands

```bash
# Quick builds using the script
./build-ios.sh                    # Debug, Simulator
./build-ios.sh -d                 # Debug, Device
./build-ios.sh -r -d              # Release, Device
./build-ios.sh -c                 # Clean build

# Open Xcode project
./open-xcode.sh                   # Auto-configures if needed
open build-ios/Gambler2.xcodeproj # Direct open

# Manual CMake (if you need control)
cmake -Bbuild-ios -GXcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 -DCMAKE_OSX_SYSROOT=iphonesimulator
cmake --build build-ios --config Debug --target Gambler2_Standalone -- -sdk iphonesimulator -allowProvisioningUpdates
```

## File Locations

| Item | Location |
|------|----------|
| iOS CMake config | `cmake/PamplejuceIOS.cmake` |
| iOS Info.plist | `packaging/Info-iOS.plist` |
| Main CMakeLists | `CMakeLists.txt` (lines 70-110 for iOS settings) |
| Build artifacts | `build-ios/Gambler2_artefacts/Debug/` |
| Documentation | `docs/iOS-Build-Guide.md` |

## Key Configuration Values

| Setting | Value | Location |
|---------|-------|----------|
| Bundle ID | `com.ToneGarden.Gambler2` | `CMakeLists.txt` line 38 |
| Deployment Target | iOS 12.0 | `cmake/PamplejuceIOS.cmake` line 7 |
| Company Name | ToneGarden | `CMakeLists.txt` line 35 |
| Plugin Code | P002 | `CMakeLists.txt` line 90 |
| Manufacturer Code | Pamp | `CMakeLists.txt` line 85 |

## Common Tasks

### Change Deployment Target

Edit `cmake/PamplejuceIOS.cmake` line 7:
```cmake
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum iOS deployment version" FORCE)
```

### Change Bundle ID

Edit `CMakeLists.txt` line 38:
```cmake
set(BUNDLE_ID "com.YourCompany.YourApp")
```

### Add Privacy Permission

Edit `packaging/Info-iOS.plist`, add:
```xml
<key>NSCameraUsageDescription</key>
<string>Your reason here</string>
```

### Change Screen Orientations

Edit `CMakeLists.txt` lines 105-106:
```cmake
IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait
IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
```

### Set Development Team

Edit `cmake/PamplejuceIOS.cmake` line 33:
```cmake
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "YOUR_TEAM_ID" CACHE STRING "Development team ID")
```

Or via command line:
```bash
cmake -Bbuild-ios -GXcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="YOUR_TEAM_ID"
```

## Targets

| Target | Description | Format |
|--------|-------------|--------|
| `Gambler2_Standalone` | Standalone iOS app | `.app` |
| `Gambler2_AUv3` | Audio Unit v3 plugin | `.appex` |

## Testing

### On Simulator
1. Build: `./build-ios.sh`
2. Open Xcode: `./open-xcode.sh`
3. Select simulator and run

### On Device
1. Build: `./build-ios.sh -d`
2. Open Xcode: `./open-xcode.sh`
3. Connect device, select it, and run
4. Trust developer certificate on device (first time only)

### AUv3 in Host App
1. Build (⌘B, not Run) AUv3 target once
2. Open GarageBand/AUM on iPad
3. Add instrument → Gambler2 (under ToneGarden)
4. To debug: Xcode → Debug → Attach to Process → Gambler2

## Troubleshooting

| Problem | Solution |
|---------|----------|
| No provisioning profile | Open in Xcode, select team in Signing & Capabilities |
| AudioPluginHost.app error | Expected for AUv3! Use Build (⌘B) not Run (⌘R) |
| AUv3 not showing | Build (not run) AUv3 target, restart host app, restart iPad |
| Build fails | Try `./build-ios.sh -c` to clean build |
| Xcode can't find project | Run `./build-ios.sh` first to generate project |
| Simulator not available | Install iOS Simulator in Xcode preferences |

## Useful Xcode Shortcuts

| Action | Shortcut |
|--------|----------|
| Build | ⌘B |
| Run | ⌘R |
| Stop | ⌘. |
| Clean | ⌘⇧K |
| Archive | ⌘⇧B (with device selected) |

## Distribution Checklist

- [ ] Build Release configuration: `./build-ios.sh -r -d`
- [ ] Test on multiple iOS versions
- [ ] Test on iPhone and iPad
- [ ] Test AUv3 in multiple host apps
- [ ] Set development team in Xcode
- [ ] Archive in Xcode (Product → Archive)
- [ ] Validate archive in Organizer
- [ ] Distribute to App Store or TestFlight

## Resources

- [iOS Build Guide](iOS-Build-Guide.md) - Comprehensive guide
- [CLion iOS Setup](CLion-iOS-Setup.md) - CLion-specific setup
- [JUCE CMake API](../JUCE/docs/CMake%20API.md) - Official JUCE docs
- [Apple Developer](https://developer.apple.com) - Apple documentation

## Support

For issues specific to:
- **JUCE**: [JUCE Forum](https://forum.juce.com)
- **CMake**: [CMake Documentation](https://cmake.org/documentation/)
- **iOS Development**: [Apple Developer Forums](https://developer.apple.com/forums/)

