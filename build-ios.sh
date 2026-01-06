#!/bin/bash
# iOS Build Script for Gambler
# This script configures and builds the iOS targets (Standalone and AUv3)

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build-ios"
CMAKE_EXECUTABLE="/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake"

# Parse command line arguments
BUILD_TYPE="Debug"
DEVICE_TYPE="device"  # simulator or device
CLEAN_BUILD=false

print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -r, --release          Build in Release mode (default: Debug)"
    echo "  -d, --device           Build for device (default: simulator)"
    echo "  -s, --simulator        Build for simulator"
    echo "  -c, --clean            Clean build directory before building"
    echo "  -h, --help             Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                     # Build Debug for simulator"
    echo "  $0 -r -d               # Build Release for device"
    echo "  $0 -c -s               # Clean build Debug for simulator"
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -d|--device)
            DEVICE_TYPE="device"
            shift
            ;;
        -s|--simulator)
            DEVICE_TYPE="simulator"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            print_usage
            exit 1
            ;;
    esac
done

echo -e "${GREEN}=== Gambler iOS Build ===${NC}"
echo "Build Type: $BUILD_TYPE"
echo "Device Type: $DEVICE_TYPE"
echo ""

# Clean if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"

# Set SDK based on device type
if [ "$DEVICE_TYPE" = "simulator" ]; then
    SDK="iphonesimulator"
    DESTINATION="generic/platform=iOS Simulator"
else
    SDK="iphoneos"
    DESTINATION="generic/platform=iOS"
fi

echo -e "${GREEN}Step 1: Configuring CMake for iOS...${NC}"
"$CMAKE_EXECUTABLE" -B"$BUILD_DIR" -GXcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DCMAKE_OSX_SYSROOT=$SDK \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo ""
echo -e "${GREEN}Step 2: Building Standalone app...${NC}"
"$CMAKE_EXECUTABLE" --build "$BUILD_DIR" \
    --config "$BUILD_TYPE" \
    --target Gambler2_Standalone \
    -- -sdk $SDK -destination "$DESTINATION" -allowProvisioningUpdates

echo ""
echo -e "${GREEN}Step 3: Building AUv3 plugin...${NC}"
"$CMAKE_EXECUTABLE" --build "$BUILD_DIR" \
    --config "$BUILD_TYPE" \
    --target Gambler2_AUv3 \
    -- -sdk $SDK -destination "$DESTINATION" -allowProvisioningUpdates

echo ""
echo -e "${GREEN}=== Build Complete! ===${NC}"
echo ""
echo "Build artifacts location:"
echo "  $BUILD_DIR/Gambler2_artefacts/$BUILD_TYPE/"
echo ""
echo "To open in Xcode:"
echo "  open $BUILD_DIR/Gambler2.xcodeproj"
echo ""
echo "To archive for App Store (device builds only):"
echo "  1. Open the Xcode project"
echo "  2. Select 'Any iOS Device' as the destination"
echo "  3. Product -> Archive"
echo "  4. Use the Organizer to distribute"

open ${BUILD_DIR}/Gambler2.xcodeproj
