#pragma once

/**
 * HapticFeedback
 *
 * Triggers trackpad haptic feedback on macOS (Force Touch).
 * No-op on iOS and non-Apple platforms.
 */
struct HapticFeedback {
    /** Fire a single detent-style tick — suitable for rotary notch crossings. */
    static void tick();
};
