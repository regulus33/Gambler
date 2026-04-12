#include "HapticFeedback.h"

#if JUCE_MAC

#import <AppKit/AppKit.h>

void HapticFeedback::tick()
{
    [[NSHapticFeedbackManager defaultPerformer]
        performFeedbackPattern:NSHapticFeedbackPatternAlignment
        performanceTime:NSHapticFeedbackPerformanceTimeNow];
}

#endif
