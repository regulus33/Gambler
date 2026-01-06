# iOS-specific configuration for JUCE projects
# This file handles iOS deployment target, architectures, and build settings

# Set iOS deployment target
# iOS 12.0 is a good minimum for modern features while maintaining compatibility
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum iOS deployment version" FORCE)
    message(STATUS "Building for iOS with deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    
    # Set iOS architectures
    # For device builds, use arm64
    # For simulator builds, use x86_64 and arm64 (for M1+ Macs)
    if (NOT DEFINED CMAKE_OSX_ARCHITECTURES)
        if (CMAKE_OSX_SYSROOT MATCHES ".*simulator.*")
            set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "iOS Simulator architectures" FORCE)
            message(STATUS "Building for iOS Simulator: ${CMAKE_OSX_ARCHITECTURES}")
        else()
            set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "iOS Device architecture" FORCE)
            message(STATUS "Building for iOS Device: ${CMAKE_OSX_ARCHITECTURES}")
        endif()
    endif()
    
    # Disable Xcode scheme generation for modules by default
    set(CMAKE_XCODE_GENERATE_SCHEME OFF)
    
    # Enable bitcode (optional, can be disabled if causing issues)
    # Note: Apple deprecated bitcode for iOS apps in Xcode 14
    # set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
    
    # Set code signing settings
    # These can be overridden via command line or in Xcode
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer" CACHE STRING "Code signing identity")
    set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "TFM56WJ6SZ" CACHE STRING "Development team ID")
    
    # Allow Xcode to automatically manage signing
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic" CACHE STRING "Code signing style")
    
    # Set targeted device family (1 = iPhone, 2 = iPad, 1,2 = Universal)
    set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" CACHE STRING "Targeted device family")
    
    # Skip install for iOS builds (we'll use Xcode's archive instead)
    set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)
    
    message(STATUS "iOS configuration complete")
endif()

