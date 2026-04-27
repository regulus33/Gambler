#include "HapticFeedback.h"

#if !JUCE_MAC && !JUCE_IOS

// Stub for non-Apple builds. The .mm file owns the counter on Apple targets.
#include <atomic>

namespace {
    std::atomic<int>& counter() {
        static std::atomic<int> c{0};
        return c;
    }
}

int HapticFeedback::tickCount() { return counter().load(std::memory_order_relaxed); }
void HapticFeedback::resetTickCount() { counter().store(0, std::memory_order_relaxed); }

void HapticFeedback::tick()
{
    counter().fetch_add(1, std::memory_order_relaxed);
}

#endif
