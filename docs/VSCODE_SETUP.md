# VS Code Setup Guide for Gambler

## Quick Start

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Cmd+Shift+R` | **Build Standalone & Run** |
| `Cmd+Shift+B` | **Build VST3 & Launch Bitwig** |
| `Cmd+B` | Build Standalone (no run) |
| `F5` | Run selected configuration |

---

## Build & Run Configurations

Press `Cmd+Shift+D` or click the Run icon to see all configurations:

### Standalone
- 🎯 **Build & Run Standalone (Debug)** - Build and launch standalone app
- 🎯 **Build & Run Standalone (Release)** - Build and launch release version

### VST3
- 🎹 **Build VST3 & Launch Bitwig (Debug)** - Build VST3 and open Bitwig
- 🎹 **Build VST3 & Launch Bitwig (Release)** - Build release VST3 and open Bitwig

### AU (Audio Unit)
- 🎵 **Build AU & Launch Logic Pro (Debug)** - Build AU and open Logic
- 🎵 **Build AU & Launch Logic Pro (Release)** - Build release AU and open Logic

---

## Typical Workflows

### Testing Standalone App
1. Press `Cmd+Shift+R` (or F5 with "Build & Run Standalone" selected)
2. App builds and launches automatically
3. Test your changes

### Testing VST3 in Bitwig
1. Press `Cmd+Shift+B` (or F5 with "Build VST3 & Launch Bitwig" selected)
2. VST3 builds and Bitwig launches
3. In Bitwig: Add "Gambler2" as a MIDI effect on a track
4. Test your plugin

### Just Building (No Launch)
- `Cmd+Shift+P` → "Tasks: Run Task"
- Select:
  - "Build: Standalone (Debug)"
  - "Build: VST3 (Debug)"
  - "Build: AU (Debug)"
  - etc.

---

## Plugin Locations

After building, plugins are automatically copied to:

- **VST3**: `~/Library/Audio/Plug-Ins/VST3/Gambler2.vst3`
- **AU**: `~/Library/Audio/Plug-Ins/Components/Gambler2.component`
- **Standalone**: `build/Gambler2_artefacts/Debug/Standalone/Gambler2.app`

---

## Debugging with Logging

Since debugger attachment is tricky with DAWs, use JUCE's logging:

```cpp
// In your code
DBG("Value: " << someValue);
DBG("Button clicked!");
```

View logs in **Console.app**:
1. Open Console.app
2. Filter for "Gambler" or "Bitwig"
3. See your debug messages in real-time

---

## Build Tasks Reference

All available tasks (access via `Cmd+Shift+P` → "Tasks: Run Task"):

### Configuration
- CMake: Configure
- CMake: Configure (Release)

### Build Only
- Build: Standalone (Debug/Release)
- Build: VST3 (Debug/Release)
- Build: AU (Debug/Release)
- Build: All (Debug/Release)

### Build & Run
- Build & Run: Standalone (Debug/Release)
- Build & Run: VST3 → Bitwig (Debug/Release)
- Build & Run: AU → Logic (Debug/Release)

### Clean
- Clean: Build Directory
- Clean: Rebuild All (Debug)

---

## Troubleshooting

### Red Underlines in Code
```bash
# Regenerate compile_commands.json
cmake -B build -G Ninja

# Restart clangd
Cmd+Shift+P → "clangd: Restart language server"
```

### Build Fails
```bash
# Clean and rebuild
rm -rf build
cmake -B build -G Ninja
cmake --build build --config Debug
```

### Plugin Not Showing in DAW
1. Check plugin was built: `ls ~/Library/Audio/Plug-Ins/VST3/Gambler2.vst3`
2. Rescan plugins in your DAW
3. Restart the DAW

---

## Helper Scripts

### `./debug-vst3.sh`
Builds VST3 and launches Bitwig (same as `Cmd+Shift+B`)

### `./sign-lldb.sh`
Signs LLDB for debugging (not needed for build & run workflow)

---

## Extensions Installed

- **CMake Tools** - Build system integration
- **clangd** - C++ IntelliSense and linting
- **CMake** - CMake syntax highlighting

---

## Tips

1. **Fast iteration**: Use `Cmd+Shift+R` for standalone testing (fastest)
2. **VST3 testing**: Use `Cmd+Shift+B`, then load plugin in Bitwig
3. **Release builds**: Use the Release configurations before distributing
4. **Logging**: Use `DBG()` macro for quick debugging instead of breakpoints
5. **Clean builds**: If things get weird, run "Clean: Rebuild All (Debug)"

---

## Next Steps

- Set up your preferred keyboard shortcuts in `.vscode/keybindings.json`
- Customize build tasks in `.vscode/tasks.json`
- Add more DAW configurations in `.vscode/launch.json`

Happy coding! 🎵

