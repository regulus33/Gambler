#!/bin/bash
# Quick script to open the iOS Xcode project
# If the project doesn't exist, it will configure it first

BUILD_DIR="build-ios"
CMAKE_EXECUTABLE="/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake"

if [ ! -d "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/Gambler2.xcodeproj/project.pbxproj" ]; then
    echo "Xcode project not found. Configuring..."
    "$CMAKE_EXECUTABLE" -B"$BUILD_DIR" -GXcode \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
        -DCMAKE_OSX_SYSROOT=iphonesimulator
    echo "Configuration complete!"
fi

echo "Opening Xcode project..."
open "$BUILD_DIR/Gambler2.xcodeproj"

