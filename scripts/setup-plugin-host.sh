#!/bin/bash
# Build JUCE AudioPluginHost for debugging

set -e

echo "🔨 Building JUCE AudioPluginHost..."
echo ""
echo "This is a lightweight host specifically for plugin development."
echo "It supports full debugging without permission issues!"
echo ""

# Check if JUCE exists
if [ ! -d "JUCE/extras/AudioPluginHost" ]; then
    echo "❌ JUCE not found!"
    echo "Run: git submodule update --init --recursive"
    exit 1
fi

cd JUCE/extras/AudioPluginHost/Builds/MacOSX

# Check if Xcode project exists
if [ ! -f "AudioPluginHost.xcodeproj/project.pbxproj" ]; then
    echo "❌ Xcode project not found!"
    echo "You may need to run Projucer to generate it."
    exit 1
fi

echo "Building Debug configuration..."
xcodebuild -project AudioPluginHost.xcodeproj \
           -configuration Debug \
           -jobs 8 \
           build

echo ""
echo "✅ AudioPluginHost built successfully!"
echo ""
echo "📍 Location: JUCE/extras/AudioPluginHost/Builds/MacOSX/build/Debug/AudioPluginHost.app"
echo ""
echo "🚀 To use:"
echo "   1. Run: open JUCE/extras/AudioPluginHost/Builds/MacOSX/build/Debug/AudioPluginHost.app"
echo "   2. Options → Edit the list of available plug-ins"
echo "   3. Scan for your VST3: ~/Library/Audio/Plug-Ins/VST3/Gambler2.vst3"
echo "   4. Add it to the graph"
echo "   5. Debug with full breakpoint support!"
echo ""
echo "💡 Or use the VS Code configuration I'll create for you..."

