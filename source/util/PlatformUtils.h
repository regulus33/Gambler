#pragma once

/**
 * PlatformUtils
 *
 * Centralized platform detection for behavioral differences between iOS and desktop.
 * Search usages of PlatformUtils::isIOS() to find all platform-specific code paths.
 */
struct PlatformUtils {
    static bool isIOS() {
#if JUCE_IOS
        return true;
#else
        return false;
#endif
    }
};
