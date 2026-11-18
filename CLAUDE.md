# Gambler

Probability-driven MIDI step sequencer built with JUCE 8 (C++23). Ships as a standalone app and as AU / VST3 / CLAP (macOS) and AUv3 (iPad). The editor is a web page (`web/`) inside a JUCE web view. Open source, MIT.

`README.md` covers features, usage and building. `docs/ARCHITECTURE.md` explains threading, state and the gotchas; read it before changing the processor, clock or state code.

## Commands

```bash
git submodule update --init --recursive                 # JUCE, clap-juce-extensions, melatonin_inspector
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target Gambler2_Standalone         # also Gambler2_AU / _VST3 / _CLAP
cmake --build build --target Tests && ./build/Tests      # Catch2; all tests must pass
./build/Tests "[visual]"                                 # visual snapshot tests (legacy JUCE editor)
./build-ios.sh -s | -d [-r] [-c]                         # iPad simulator / device build → build-ios/Gambler2.xcodeproj
./scripts/pluginval.sh                                   # plugin validation of the VST3
open web/index.html                                      # UI in a browser against the mock engine
```

Targets are named `Gambler2_*` (`PROJECT_NAME` is `Gambler2`); the built products are called `Gambler` (`PRODUCT_NAME`).

## Layout

- `source/PluginProcessor.*`: controller; owns APVTS, InternalState, clock, MIDI I/O, presets.
- `source/backend/`: `Sequence`, `Step`, `SequenceState` (the API UI code should use), `Clock`, `DawSync`, `ScaleQuantizer`, MIDI learn.
- `source/bridge/UiBridge.*`: the only seam between engine and UI. Pushes `state`/`tick`/`noteOff`/`toast`, handles actions.
- `source/web/WebEditor.*`: plugin window hosting the web view; serves `web/` from BinaryData.
- `source/ui/`: legacy JUCE component editor (`GAMBLER_WEB_UI=0`). Still compiled; the visual tests render it.
- `web/`: `index.html`, `css/style.css`, `js/app.js` (UI), `js/bridge.js` (message contract), `js/mock-engine.js`.
- `tests/`: Catch2 tests; snapshot goldens in `tests/snapshots/`.
- `packaging/`: icons (`icon-ios.svg`, `icon-macos.svg` → `scripts/render-icons.sh`), iOS asset catalog and `Info-iOS.plist`, App Store kit in `packaging/appstore/` (`scripts/render-appstore-screenshots.sh`).

## Rules of the road

- **UI ⇄ engine changes go through the contract** at the top of `web/js/bridge.js`. When adding or changing an action or state field, update all three: `web/js/app.js`, `source/bridge/UiBridge.cpp`, and `web/js/mock-engine.js` (so the browser preview keeps working).
- **File names under `web/` must be unique**: BinaryData flattens directories (`cmake/Assets.cmake`). Anything in `assets/` is embedded in the binary too, so don't put unused files there.
- **State split**: small automatable values live in APVTS (`source/data/Params.h`); per-step data lives in InternalState. Go through `SequenceState` rather than touching either directly.
- **Audio thread**: no UI calls, no allocation-heavy work in `processBlock`/`Sequence::tick()`. Audio → UI uses the processor callbacks, which `UiBridge` turns into atomic flags.
- **Visual snapshots**: never run `UPDATE_SNAPSHOTS=1` without the user confirming the new render is correct.
- **Releases** go through `.github/workflows/release-ios.yml` (tag `vX.Y.Z` matching `VERSION` → build, sign, upload to App Store Connect on a `macos-26` runner; Xcode 26+ is required for uploads). Setup and renewal: `docs/RELEASING.md`. Never print, log or commit signing material; secrets live in the `app-store` environment.
- **Never change** `BUNDLE_ID`, `PRODUCT_NAME`, `PLUGIN_MANUFACTURER_CODE` (`Tngd`) or `PLUGIN_CODE` (`Gmbl`) in `CMakeLists.txt` after release: they identify the app on the App Store and the plugin in users' saved host sessions.
- **iOS Info.plist keys**: `PLIST_TO_MERGE` takes XML text, so `CMakeLists.txt` reads `packaging/Info-iOS.plist`. Permission texts and other keys JUCE manages go through `juce_add_plugin` options instead.
- **iOS** is iPad-only for now (`TARGETED_DEVICE_FAMILY "2"` in `CMakeLists.txt`); the web UI doesn't fit iPhone yet. Simulator screenshots from `xcrun simctl io` come out in portrait orientation; rotate them with `sips -r 270`.
- **Commits**: one-line, plain-English summary of the behaviour change (e.g. "Make chords on MIDI input change cleanly, with or without Latch"). No AI attribution or co-author trailers (`.claude/settings.json` turns them off).
