# Claude Code - Project Setup & Reference

This document contains important setup information and reference material for working with the Gambler2 JUCE plugin project in the VM environment.

## Project Overview

- **Project Name:** Gambler2
- **Type:** JUCE MIDI Effect Plugin
- **Build System:** CMake (based on Pamplejuce template)
- **Formats:** Standalone, AU, VST3, AUv3, CLAP
- **Platform:** Linux (Ubuntu 24.04 LTS)
- **Compiler:** GCC 13.3.0
- **CMake Version:** 3.28.3

## Initial VM Setup (Completed 2025-12-27)

### Dependencies Installed

All required Linux dependencies for building JUCE applications:

```bash
sudo apt-get update
sudo apt-get install -y \
  libasound2-dev \
  libjack-jackd2-dev \
  libxinerama-dev \
  libxrandr-dev \
  libxcursor-dev \
  libgl1-mesa-dev \
  libglu1-mesa-dev
```

### Submodules Initialized

All git submodules have been initialized and updated:

```bash
git submodule update --init --recursive
```

**Submodules:**
- JUCE (develop branch)
- cmake (sudara/cmake-includes, main branch)
- modules/clap-juce-extensions (main branch)
- modules/melatonin_inspector (main branch)

**Note:** The cmake submodule required manual update to latest main branch due to missing commit reference.

### VERSION File Fix

Fixed the VERSION file to remove trailing newline (required by Pamplejuce):
- Original: `0.0.1\n`
- Fixed: `0.0.1` (no newline)

## Build Instructions

### Initial Build

```bash
cd /workspaces/Gambler
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug -j$(nproc)
```

### Rebuild After Changes

```bash
cd /workspaces/Gambler/build
cmake --build . --config Debug -j$(nproc)
```

### Build Configurations

- **Debug:** Use `-DCMAKE_BUILD_TYPE=Debug` (includes debug symbols, no optimization)
- **Release:** Use `-DCMAKE_BUILD_TYPE=Release` (optimized, smaller binary)

## Build Artifacts

After a successful build, the following artifacts are created:

### Standalone Application
- **Location:** `/workspaces/Gambler/build/Gambler2_artefacts/Debug/Standalone/Gambler2`
- **Size:** ~98MB (Debug build with symbols)
- **Run:** `./build/Gambler2_artefacts/Debug/Standalone/Gambler2`

### VST3 Plugin
- **Build Location:** `/workspaces/Gambler/build/Gambler2_artefacts/Debug/VST3/Gambler2.vst3`
- **Installed Location:** `~/.vst3/Gambler2.vst3`

### CLAP Plugin
- **Location:** `/workspaces/Gambler/build/Gambler2_artefacts/Debug/CLAP/Gambler2.clap`

### Tests & Benchmarks
- **Tests:** `/workspaces/Gambler/build/Tests`
- **Benchmarks:** `/workspaces/Gambler/build/Benchmarks`

## Testing

### Run Tests
```bash
cd /workspaces/Gambler/build
./Tests
```

### Run Benchmarks
```bash
cd /workspaces/Gambler/build
./Benchmarks
```

## Project Structure

```
/workspaces/Gambler/
├── build/                      # Build directory (gitignored)
├── source/                     # Plugin source code
│   ├── backend/               # Backend logic (Step, Sequence, Clock, etc.)
│   ├── ui/                    # UI components
│   ├── data/                  # Data structures (Params)
│   ├── PluginEditor.cpp/h     # Main editor
│   └── PluginProcessor.cpp/h  # Main processor
├── tests/                      # Test files
├── benchmarks/                 # Benchmark files
├── assets/                     # Binary assets (fonts, images)
├── modules/                    # External modules
│   ├── clap-juce-extensions/
│   └── melatonin_inspector/
├── JUCE/                       # JUCE framework submodule
├── cmake/                      # CMake helper scripts
├── CMakeLists.txt             # Main CMake configuration
└── VERSION                     # Version file (no trailing newline!)
```

## Known Issues & Fixes

### Issue: CMake Version Error
**Error:** `Invalid version format: 0.0.1`
**Cause:** Trailing newline in VERSION file
**Fix:** Ensure VERSION file has no trailing newline: `printf "0.0.1" > VERSION`

### Issue: Submodule cmake Missing Commit
**Error:** `remote error: upload-pack: not our ref a98084d71914d492d8f0f90e4c1396531a9a8f7a`
**Fix:**
```bash
cd cmake
git fetch origin
git checkout origin/main
```

### Issue: CLAP Nested Submodules Not Initialized
**Error:** Missing CMakeLists.txt in clap-libs/clap and clap-libs/clap-helpers
**Fix:**
```bash
cd modules/clap-juce-extensions
git submodule update --init --recursive
```

## Compiler Warnings

The build produces some sign-conversion warnings in:
- `source/backend/StepNotes.h`
- `source/backend/StepModulations.h`

These are non-critical warnings about converting int to size_t for array indexing. Consider fixing by using `size_t` for index variables.

## Development Workflow

1. **Make code changes** in `source/` directory
2. **Rebuild** with `cmake --build . -j$(nproc)` from build directory
3. **Test** with `./Tests` or run standalone application
4. **For UI changes**, the melatonin_inspector is available for debugging

## Important Notes

- **VM State Persistence:** All dependencies and builds persist when VM state is saved
- **Standalone Mode Only:** No plugin host setup required for this development environment
- **Auto-Bump Version:** CMakeLists.txt has `PAMPLEJUCE_AUTO_BUMP_PATCH_LEVEL ON` which auto-increments patch version based on git commits
- **Component Snapshots:** Enabled via `TAKE_SNAPSHOTS_OF_COMPONENTS=1`, saves to `component_snapshots/`

## Quick Reference Commands

```bash
# Full rebuild from scratch
rm -rf build && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && cmake --build . -j$(nproc)

# Run standalone application
./build/Gambler2_artefacts/Debug/Standalone/Gambler2

# Run tests
./build/Tests

# Update submodules
git submodule update --remote --merge

# Check dependencies
ldd ./build/Gambler2_artefacts/Debug/Standalone/Gambler2

# Clean build artifacts
rm -rf build
```

## Additional Resources

- JUCE Documentation: https://docs.juce.com/
- Pamplejuce Template: https://github.com/sudara/pamplejuce
- CLAP Format: https://github.com/free-audio/clap
