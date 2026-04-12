#include "HapticFeedback.h"

#if ! JUCE_MAC

// Stub for non-macOS builds — haptic feedback is only available on macOS.
void HapticFeedback::tick()
{
}

#endif
