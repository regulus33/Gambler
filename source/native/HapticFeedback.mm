#include "HapticFeedback.h"

#include <atomic>

namespace {
    std::atomic<int>& counter() {
        static std::atomic<int> c{0};
        return c;
    }
}

int HapticFeedback::tickCount() { return counter().load(std::memory_order_relaxed); }
void HapticFeedback::resetTickCount() { counter().store(0, std::memory_order_relaxed); }

#if JUCE_MAC

#import <AppKit/AppKit.h>

void HapticFeedback::tick()
{
    counter().fetch_add(1, std::memory_order_relaxed);
    // LevelChange is more noticeable than Alignment; Alignment is so subtle
    // that many users never feel it even on Force Touch hardware.
    [[NSHapticFeedbackManager defaultPerformer]
        performFeedbackPattern:NSHapticFeedbackPatternLevelChange
        performanceTime:NSHapticFeedbackPerformanceTimeNow];
}

#elif JUCE_IOS

#import <UIKit/UIKit.h>

void HapticFeedback::tick()
{
    counter().fetch_add(1, std::memory_order_relaxed);
    static UIImpactFeedbackGenerator* generator = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        generator = [[UIImpactFeedbackGenerator alloc]
                        initWithStyle:UIImpactFeedbackStyleLight];
        [generator prepare];
    });
    [generator impactOccurred];
}

#endif
