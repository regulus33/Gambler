# iOS AUv3 File Export Research

## Problem
Exporting MIDI files from the iOS AUv3 plugin. `FileChooser::launchAsync` opens an invisible modal dialog that blocks all touch input. `ContentSharer::shareFilesScoped` hits a jassert: "Content sharing not available on this platform!"

## What We've Tried

1. **FileChooser::launchAsync** - Opens invisible modal in AUv3, blocks all UI input. Works on desktop. NOT viable for AUv3.

2. **Direct file save to Documents** - Files saved to `userDocumentsDirectory` are not visible in the Files app from AUv3 because the plugin runs in the host app's sandbox, not its own.

3. **ContentSharer::shareFilesScoped** - Hits "not available" assert because `JUCE_CONTENT_SHARING` is not defined in CMakeLists.txt. The native iOS implementation (`juce_ContentSharer_ios.cpp`) is gated behind this define and never gets compiled without it.

## Root Cause: Missing Compile Definition

In `JUCE/modules/juce_gui_basics/juce_gui_basics.cpp` lines 182-184:
```cpp
#if JUCE_CONTENT_SHARING
  #include "native/juce_ContentSharer_ios.cpp"
#endif
```

The Projucer auto-sets this for iOS targets, but our CMake build does not. **Fix: add `JUCE_CONTENT_SHARING=1` to `target_compile_definitions` in CMakeLists.txt.**

## Recommended Fix (Try First)

Add to CMakeLists.txt:
```cmake
target_compile_definitions(Gambler2 PUBLIC JUCE_CONTENT_SHARING=1)
```

Then the existing code in `ApplicationComponent.h` should work:
```cpp
activeShareSession = juce::ContentSharer::shareFilesScoped(
    files, callback, this);  // parent component required for AUv3
```

## Known ContentSharer Issues (If Fix Works But Has Bugs)

- **Callback never called**: Multiple reports on JUCE forum. Store `ScopedMessageBox` as member (we already do this).
- **Crash after sharing**: Ensure temp file stays valid until callback fires.
- **Hanging share sheet**: Reported in JUCE forum, may need JUCE update.

Sources:
- https://forum.juce.com/t/contentsharer-sharefilesscoped-callback-never-called/63856
- https://forum.juce.com/t/shares-successfully-but-then-crashes/66519
- https://forum.juce.com/t/ios-contentsharer-hanging/67306

## Fallback: Native Objective-C++ Bridge

If ContentSharer is too buggy, write a custom `.mm` file that presents `UIActivityViewController` directly:

1. Create `source/native/iOSShareBridge.h` (C++ interface)
2. Create `source/native/iOSShareBridge.mm` (ObjC++ implementation)
3. Get the UIViewController from the JUCE component peer and present the share sheet

The JUCE source at `juce_ContentSharer_ios.cpp` does exactly this — can be used as reference.

Sources:
- https://forum.juce.com/t/fix-content-sharing-from-auv3-ios/49652
- https://github.com/juce-framework/JUCE/pull/996

## AUv3 Constraints to Remember

- **No top-level windows**: AUv3 runs in app extension sandbox. Any native UI (share sheets, alerts) MUST be presented from the plugin's existing UIViewController, not a new window.
- **Pass parent component**: `shareFilesScoped(..., parentComponent)` — passing `nullptr` will assert-fail in AUv3.
- **Files app visibility**: `UIFileSharingEnabled` and `LSSupportsOpeningDocumentsInPlace` in Info.plist only apply to the **Standalone** app container. AUv3 plugins have a separate sandbox at `/var/mobile/Containers/Data/PluginKitPlugin/<UUID>/` — files there are NOT visible in Files.app. This is why the share sheet is the correct approach.
- **App Group containers**: `File::getContainerForSecurityApplicationGroupIdentifier()` can share data between standalone and AUv3, but this folder is also not visible in Files.app. Useful for presets, not for user-facing export.

Sources:
- https://forum.juce.com/t/ios-auv3-save-load-custom-presets/54140
- https://forum.juce.com/t/app-group-folder-access/23102
- https://forum.juce.com/t/ipados-file-sharing/59202

## Other Platform-Specific Issues Fixed in This Session

All caused by iOS AUv3 not supporting top-level native windows:

| Issue | Root Cause | Fix |
|-------|-----------|-----|
| PopupMenu not showing | JUCE screen detection returns zero bounds | CallOutBox on iOS, PopupMenu on desktop (`PlatformUtils::isIOS()`) |
| ComboBox not showing | Same as PopupMenu | `CallOutDropdown` component: native ComboBox on desktop, CallOutBox on iOS |
| CallOutBox color pickers not showing | `nullptr` parent | Pass `getTopLevelComponent()` as parent |
| FileChooser blocks all input | Invisible modal dialog | ContentSharer on iOS, FileChooser on desktop |
| JUCE 8.0.8 screen detection bug | `TemporaryWindow` gets null scene | Updated JUCE submodule to 8.0.12 |
