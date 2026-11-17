cmake_minimum_required(VERSION 3.25)

# This tells cmake we have goodies in the /cmake folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Enable auto-incrementing patch level based on git commits
set(PAMPLEJUCE_AUTO_BUMP_PATCH_LEVEL OFF)

include (PamplejuceVersion)

# Modern concise way to add dependencies to your project
include (CPM)

# Configures universal binaries and decides which version of macOS to support
include(PamplejuceMacOS)

# iOS-specific configuration (deployment target, architectures, signing)
include(PamplejuceIOS)

# Couple tweaks that IMO should be JUCE defaults
include(JUCEDefaults)

# Change me!
# This is the internal name of the project and the name of JUCE's shared code target
# Note: This cannot have spaces (it may be 2024, but you can't have it all!)
# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs display)
set(PROJECT_NAME "Gambler2")

# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs will display)
# You can also just have it be the same thing as PROJECT_NAME
# You may want to append the major version on the end of this (and PROJECT_NAME) ala:
#   set(PROJECT_NAME "MyPlugin_v${MAJOR_VERSION}")
# Doing so enables major versions to show up in IDEs and DAWs as separate plugins
# allowing you to change parameters and behavior without breaking existing user projects
set(PRODUCT_NAME "Gambler2")

# Change me! Used for the MacOS bundle name and Installers
set(COMPANY_NAME "ToneGarden")

# Change me! Used for the MacOS bundle identifier (and signing)
set(BUNDLE_ID "com.ToneGarden.Gambler2")

# Change me! Set the plugin formats you want built
# Valid choices: AAX Unity VST VST3 AU AUv3 Standalone
set(FORMATS Standalone AU VST3 AUv3)

# Force JUCE to work with sccache
# instruct MSVC to embed debug info (/Z7) instead of emitting a .pdb
set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded CACHE STRING "" FORCE)

# For simplicity, the name of the CMake project is also the name of the target
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

# JUCE is setup as a submodule in the /JUCE folder
# Locally, you must run `git submodule update --init --recursive` once
# and later `git submodule update --remote --merge` to keep it up to date
# On Github Actions, this is done as a part of actions/checkout
add_subdirectory(JUCE)

# Add CLAP format
add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)

# Add any other modules you want modules here, before the juce_add_plugin call
# juce_add_module(modules/my_module)

# This adds the melatonin inspector module
add_subdirectory (modules/melatonin_inspector)

# See `docs/CMake API.md` in the JUCE repo for all config options
juce_add_plugin("${PROJECT_NAME}"
    # Icons for the standalone app (macOS/Windows)
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"

    # iOS: our own asset catalog with a single opaque 1024px icon (App Store rejects
    # icons with an alpha channel, which JUCE's generated ones have).
    # Regenerate it from packaging/icon-ios.svg with scripts/render-icons.sh
    CUSTOM_XCASSETS_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/packaging/ios/Images.xcassets"

    # Change me!
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"

    # On MacOS, plugin is copied to /Users/yourname/Library/Audio/Plug-Ins/
    COPY_PLUGIN_AFTER_BUILD TRUE

    # Change me!
    # A four-character plugin id
    # First character MUST be uppercase for AU formats
    PLUGIN_MANUFACTURER_CODE Pamp

    # Change me!
    # A unique four-character plugin id
    # Note: this must have at least one upper-case character
    PLUGIN_CODE P002
    FORMATS "${FORMATS}"

    # Pure MIDI effect configuration
    IS_SYNTH FALSE
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE

    # The editor is a web view (see /web and source/web/WebEditor)
    NEEDS_WEB_BROWSER TRUE

    # The name of your final executable
    # This is how it's listed in the DAW
    # This can be different from PROJECT_NAME and can have spaces!
    # You might want to use v${MAJOR_VERSION} here once you go to v2...
    PRODUCT_NAME "${PRODUCT_NAME}"

    # iOS-specific configuration
    # iPad-only for now (2 = iPad, "1,2" = iPhone + iPad): the web UI doesn't fit iPhone screens yet
    TARGETED_DEVICE_FAMILY "2"

    # Screen orientations - support all orientations for flexibility
    IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight

    # Merge custom iOS plist entries
    PLIST_TO_MERGE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/Info-iOS.plist")

# This lets us use our code in both the JUCE targets and our Test target
# Without running into ODR violations
add_library(SharedCode INTERFACE
        source/data/Params.h
        source/backend/ModulationType.h
        source/backend/Scale.h
        source/backend/ClockMode.h
        source/backend/InternalClock.h
        source/backend/Clock.h
        source/backend/DawSync.h
        source/backend/InternalState.h
        source/backend/InternalState.cpp
        source/ui/PianoRollComponent/PianoRollComponent.h
)

clap_juce_extensions_plugin(TARGET "${PROJECT_NAME}"
    CLAP_ID "${BUNDLE_ID}"
    CLAP_FEATURES note-effect midi-effect)
# Enable fast math, C++20 and a few other target defaults
include(SharedCodeDefaults)


# Manually list all .h and .cpp files for the plugin
# If you are like me, you'll use globs for your sanity.
# Just ensure you employ CONFIGURE_DEPENDS so the build system picks up changes
# If you want to appease the CMake gods and avoid globs, manually add files like so:
# set(SourceFiles Source/PluginEditor.h Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
if(APPLE)
    file(GLOB_RECURSE ObjCFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.mm")
    list(APPEND SourceFiles ${ObjCFiles})
endif()
target_sources(SharedCode INTERFACE ${SourceFiles})

# Adds a BinaryData target for embedding assets into the binary
include(Assets)

# MacOS only: Cleans up folder and target organization on Xcode.
include(XcodePrettify)

# This is where you can set preprocessor definitions for JUCE and your plugin
target_compile_definitions(SharedCode
    INTERFACE
    SHOW_INSPECTOR=0  # Set to 1 to enable the Melatonin Inspector button for UI debugging
    SHOW_MIDI_KEYBOARD=0  # Set to 1 to show the virtual MIDI keyboard strip in standalone
    TAKE_SNAPSHOTS_OF_COMPONENTS=0  # Set to 1 enable hitting s snapshots every component in tree...
    SNAPSHOT_DESTINATION="${CMAKE_SOURCE_DIR}/component_snapshots"
    GAMBLER_WEB_UI=1  # 1 = web view editor (/web), 0 = the original JUCE component editor
    JUCE_WEB_BROWSER=1  # needs `NEEDS_WEB_BROWSER TRUE` in the `juce_add_plugin` call
    JUCE_USE_CURL=0     # If you set this to 1, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_CONTENT_SHARING=1

    # Uncomment if you are paying for a an Indie/Pro license or releasing under GPLv3
    # JUCE_DISPLAY_SPLASH_SCREEN=0

    # lets the app known if we're Debug or Release
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"

    # JucePlugin_Name is for some reason doesn't use the nicer PRODUCT_NAME
    PRODUCT_NAME_WITHOUT_VERSION="Gambler2"

    # Repository root path for dev features (e.g., XML snapshot saving)
    REPO_ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Disable jasserts on iOS — audio-related asserts will be fixed later
if(IOS)
    target_compile_definitions(SharedCode INTERFACE JUCE_DISABLE_ASSERTIONS=1)
endif()

# Link to any other modules you added (with juce_add_module) here!
# Usually JUCE modules must have PRIVATE visibility
# See https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module
# However, with Pamplejuce, you'll link modules to SharedCode with INTERFACE visibility
# This allows the JUCE plugin targets and the Tests target to link against it
target_link_libraries(SharedCode
    INTERFACE
    Assets
    melatonin_inspector
    juce_audio_utils
    juce_audio_processors
    juce_gui_basics
    juce_gui_extra
    juce_animation
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Link the JUCE plugin targets our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

# IPP support, comment out to disable
include(PamplejuceIPP)

# Everything related to the tests target
include(Tests)

# A separate target for Benchmarks (keeps the Tests target fast)
include(Benchmarks)

# Output some config for CI (like our PRODUCT_NAME)
include(GitHubENV)
