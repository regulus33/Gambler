#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * ClickFlash
 *
 * Lightweight click feedback animation state.
 * Triggers a brief gold flash that fades out over ~300ms.
 *
 * Usage:
 *   - Call trigger() on mouseDown
 *   - Call tick() from timerCallback (~60Hz) — returns true while animating
 *   - Call paint(g, bounds) from paint()
 */
class ClickFlash {
public:
    void trigger() {
        alpha_ = 1.0f;
        active_ = true;
    }

    /** Advance the animation by one frame. Returns true if still active. */
    bool tick() {
        if (!active_) return false;
        alpha_ -= fadeStep_;
        if (alpha_ <= 0.0f) {
            alpha_ = 0.0f;
            active_ = false;
            return false;
        }
        return true;
    }

    /** Paint the flash overlay as a soft rounded-rect glow. */
    void paint(juce::Graphics& g, juce::Rectangle<int> bounds) const {
        if (!active_ && alpha_ <= 0.0f) return;
        g.setColour(colour_.withAlpha(alpha_ * maxAlpha_));
        float r = static_cast<float>(juce::jmin(bounds.getWidth(), bounds.getHeight())) * 0.2f;
        g.fillRoundedRectangle(bounds.toFloat(), r);
    }

    bool isActive() const { return active_; }

    void setColour(juce::Colour c) { colour_ = c; }

private:
    static constexpr float fadeStep_ = 1.0f / (0.3f * 60.0f); // 300ms at 60fps
    static constexpr float maxAlpha_ = 0.35f;
    juce::Colour colour_ { 0xFFFFD700 }; // gold
    float alpha_ = 0.0f;
    bool active_ = false;
};
