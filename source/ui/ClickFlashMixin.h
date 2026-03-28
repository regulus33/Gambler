#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>

/**
 * ClickFlash
 *
 * Self-contained click feedback animation using VBlankAttachment.
 * Triggers a brief gold flash that fades out over ~300ms, driven by
 * the display's vertical blank signal for smooth, framerate-independent animation.
 *
 * Usage:
 *   - Call attachTo(component) once (e.g. in constructor or parentHierarchyChanged)
 *   - Call trigger() on mouseDown
 *   - Call paint(g, bounds) from paint()
 *   - No timer management needed — VBlankAttachment handles animation internally
 */
class ClickFlash {
public:
    /** Attach to the owning component. Must be called before trigger(). */
    void attachTo(juce::Component* owner) { owner_ = owner; }

    void trigger() {
        alpha_ = 1.0f;
        active_ = true;
        lastTimestamp_ = 0.0;
        ensureVBlank();
    }

    /** Paint the flash overlay as a soft rounded-rect glow. */
    void paint(juce::Graphics& g, juce::Rectangle<int> bounds) const {
        if (alpha_ <= 0.0f) return;
        g.setColour(colour_.withAlpha(alpha_ * maxAlpha_));
        float r = static_cast<float>(juce::jmin(bounds.getWidth(), bounds.getHeight())) * 0.2f;
        g.fillRoundedRectangle(bounds.toFloat(), r);
    }

    bool isActive() const { return active_; }

    void setColour(juce::Colour c) { colour_ = c; }

private:
    void ensureVBlank() {
        if (!vBlank_ && owner_) {
            vBlank_ = std::make_unique<juce::VBlankAttachment>(
                owner_, [this](double ts) { onVBlank(ts); });
        }
    }

    void onVBlank(double timestampSec) {
        if (!active_) return;

        double delta = (lastTimestamp_ > 0.0)
            ? (timestampSec - lastTimestamp_)
            : (1.0 / 60.0);
        lastTimestamp_ = timestampSec;
        float dt = static_cast<float>(juce::jlimit(0.001, 0.1, delta));

        alpha_ -= dt / fadeDuration_;
        if (alpha_ <= 0.0f) {
            alpha_ = 0.0f;
            active_ = false;
        }
        if (owner_) owner_->repaint();
    }

    static constexpr float fadeDuration_ = 0.3f; // seconds
    static constexpr float maxAlpha_ = 0.35f;
    juce::Colour colour_ { 0xFFFFD700 }; // gold
    float alpha_ = 0.0f;
    bool active_ = false;
    juce::Component* owner_ = nullptr;
    double lastTimestamp_ = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vBlank_;
};
