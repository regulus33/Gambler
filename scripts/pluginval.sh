#!/usr/bin/env bash
# Download (if needed) and run pluginval against the built VST3 plugin.
#
# Usage:
#   ./scripts/pluginval.sh [--strictness-level N]
#
# Defaults to strictness level 5. Pass a different level to override:
#   ./scripts/pluginval.sh --strictness-level 10

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PLUGINVAL_DIR="$PROJECT_ROOT/build/_pluginval"
PLUGINVAL_BIN="$PLUGINVAL_DIR/pluginval"
PLUGINVAL_VERSION="v1.0.3"

# Determine platform
OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Linux)
        PLUGINVAL_URL="https://github.com/Tracktion/pluginval/releases/download/${PLUGINVAL_VERSION}/pluginval_Linux.zip"
        ;;
    Darwin)
        PLUGINVAL_URL="https://github.com/Tracktion/pluginval/releases/download/${PLUGINVAL_VERSION}/pluginval_macOS.zip"
        PLUGINVAL_BIN="$PLUGINVAL_DIR/pluginval.app/Contents/MacOS/pluginval"
        ;;
    MINGW*|MSYS*|CYGWIN*)
        PLUGINVAL_URL="https://github.com/Tracktion/pluginval/releases/download/${PLUGINVAL_VERSION}/pluginval_Windows.zip"
        PLUGINVAL_BIN="$PLUGINVAL_DIR/pluginval.exe"
        ;;
    *)
        echo "Unsupported OS: $OS" >&2
        exit 1
        ;;
esac

# Download pluginval if not present
if [ ! -f "$PLUGINVAL_BIN" ]; then
    echo "Downloading pluginval ${PLUGINVAL_VERSION}..."
    mkdir -p "$PLUGINVAL_DIR"
    curl -fsSL "$PLUGINVAL_URL" -o "$PLUGINVAL_DIR/pluginval.zip"
    unzip -o "$PLUGINVAL_DIR/pluginval.zip" -d "$PLUGINVAL_DIR"
    rm "$PLUGINVAL_DIR/pluginval.zip"
    chmod +x "$PLUGINVAL_BIN" 2>/dev/null || true
    echo "pluginval installed to $PLUGINVAL_BIN"
fi

# Find the VST3 plugin
VST3_PATH="$PROJECT_ROOT/build/Gambler2_artefacts/VST3/Gambler2.vst3"
if [ ! -e "$VST3_PATH" ]; then
    # Try Debug subfolder
    VST3_PATH="$PROJECT_ROOT/build/Gambler2_artefacts/Debug/VST3/Gambler2.vst3"
fi

if [ ! -e "$VST3_PATH" ]; then
    echo "ERROR: Could not find Gambler2.vst3. Build the plugin first:" >&2
    echo "  cmake -B build && cmake --build build" >&2
    exit 1
fi

# Default strictness level
STRICTNESS=5
EXTRA_ARGS=()

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --strictness-level)
            STRICTNESS="$2"
            shift 2
            ;;
        *)
            EXTRA_ARGS+=("$1")
            shift
            ;;
    esac
done

echo "Running pluginval on: $VST3_PATH"
echo "Strictness level: $STRICTNESS"

# On headless Linux, wrap with xvfb-run so editor tests can create windows
RUNNER=()
if [ "$OS" = "Linux" ] && [ -z "${DISPLAY:-}" ] && command -v xvfb-run &>/dev/null; then
    echo "No DISPLAY detected — using xvfb-run"
    RUNNER=(xvfb-run -a)
fi

echo "---"

"${RUNNER[@]+"${RUNNER[@]}"}" "$PLUGINVAL_BIN" \
    --validate-in-process \
    --strictness-level "$STRICTNESS" \
    --validate "$VST3_PATH" \
    "${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"}"
