#pragma once

// juce_core defines JUCE_MAC / JUCE_IOS based on __APPLE__ + TargetConditionals.
// Without this include the platform macros are undefined in our .mm / .cpp,
// the platform-specific bodies never compile, and tick() ends up a no-op even
// on hardware that supports haptics.
#include <juce_core/juce_core.h>

/**
 * HapticFeedback
 *
 * Triggers a single detent-style haptic tick. Currently used for rotary
 * notch crossings (NotchedSlider) — when the rotary moves past a discrete
 * step the call site fires `tick()`.
 *
 * Platform behaviour:
 *   - macOS: NSHapticFeedbackPerformer (LevelChange pattern). Requires a
 *            Force Touch trackpad and "Force Click and haptic feedback"
 *            enabled in System Settings → Trackpad.
 *   - iOS:   UIImpactFeedbackGenerator (light style).
 *   - Other: no-op.
 *
 * Tests can read `tickCount()` to verify call sites are reached without
 * needing real hardware.
 */
struct HapticFeedback {
    static void tick();

    /** Number of times tick() has been called this process (test hook). */
    static int tickCount();
    /** Reset the tick counter (test hook). */
    static void resetTickCount();
};
