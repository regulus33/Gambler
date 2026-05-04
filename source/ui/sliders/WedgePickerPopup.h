#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * WedgePickerPopup
 *
 * Circular pie-wedge picker for choosing one of N labelled options.
 * Designed to be embedded in a juce::CallOutBox. Single tap on a wedge
 * commits the selection and dismisses the CallOutBox.
 *
 * Out-of-scope wedges render dimmed and are non-interactive.
 */
class WedgePickerPopup : public juce::Component {
public:
    struct Wedge {
        juce::String label;
        juce::Colour colour;
        bool enabled;
        int payload; // returned via onSelected when tapped
    };

    WedgePickerPopup(juce::Array<Wedge> wedges,
                     int selectedPayload,
                     std::function<void(int)> onSelected,
                     int diameter = 240)
        : wedges_(std::move(wedges)),
          selectedPayload_(selectedPayload),
          onSelected_(std::move(onSelected)) {
        setSize(diameter, diameter);
    }

    void paint(juce::Graphics& g) override {
        if (wedges_.isEmpty()) return;

        const float pad = 4.0f;
        auto bounds = getLocalBounds().toFloat().reduced(pad);
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float outerR = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const float innerR = outerR * 0.30f;

        const int N = wedges_.size();
        const float wedgeAngle = juce::MathConstants<float>::twoPi / (float) N;

        for (int i = 0; i < N; ++i) {
            const float fromA = ((float) i - 0.5f) * wedgeAngle;
            const float toA = fromA + wedgeAngle;
            const bool isSelected = (wedges_[i].payload == selectedPayload_);
            const bool isHovered = (i == hoverIdx_) && wedges_[i].enabled;

            juce::Path wedge;
            wedge.addPieSegment(cx - outerR, cy - outerR, outerR * 2.0f, outerR * 2.0f,
                                fromA, toA, innerR / outerR);

            auto fill = wedges_[i].colour;
            if (!wedges_[i].enabled)
                fill = fill.withMultipliedAlpha(0.18f).withMultipliedSaturation(0.4f);
            else if (isSelected)
                fill = fill.brighter(0.35f);
            else if (isHovered)
                fill = fill.brighter(0.18f);

            g.setColour(fill);
            g.fillPath(wedge);

            g.setColour(juce::Colour(0xFF1A1A1A).withAlpha(0.5f));
            g.strokePath(wedge, juce::PathStrokeType(1.5f));

            if (isSelected && wedges_[i].enabled) {
                // Dark inner ring + bright outer ring — reads against any wedge colour
                g.setColour(juce::Colour(0xFF111111).withAlpha(0.85f));
                g.strokePath(wedge, juce::PathStrokeType(6.0f));
                g.setColour(juce::Colours::white);
                g.strokePath(wedge, juce::PathStrokeType(3.0f));
            }

            const float midA = (fromA + toA) * 0.5f;
            const float midR = (innerR + outerR) * 0.5f;
            const float lx = cx + midR * std::sin(midA);
            const float ly = cy - midR * std::cos(midA);

            juce::Font f(juce::FontOptions(15.0f).withStyle("Bold"));
            g.setFont(f);
            g.setColour(juce::Colours::black.withAlpha(wedges_[i].enabled ? 0.85f : 0.35f));
            g.drawText(wedges_[i].label,
                juce::Rectangle<float>(40.0f, 22.0f).withCentre({lx, ly}),
                juce::Justification::centred);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        const int idx = wedgeAt(e.position);
        if (idx < 0 || !wedges_[idx].enabled) return;
        if (onSelected_) onSelected_(wedges_[idx].payload);
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
            box->dismiss();
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const int idx = wedgeAt(e.position);
        if (idx != hoverIdx_) {
            hoverIdx_ = idx;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override {
        if (hoverIdx_ != -1) {
            hoverIdx_ = -1;
            repaint();
        }
    }

private:
    int wedgeAt(juce::Point<float> p) const {
        if (wedges_.isEmpty()) return -1;
        const float pad = 4.0f;
        auto bounds = getLocalBounds().toFloat().reduced(pad);
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float outerR = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const float innerR = outerR * 0.30f;

        const float dx = p.x - cx;
        const float dy = p.y - cy;
        const float r  = std::sqrt(dx * dx + dy * dy);
        if (r < innerR || r > outerR) return -1;

        const int N = wedges_.size();
        const float wedgeAngle = juce::MathConstants<float>::twoPi / (float) N;
        // JUCE pie convention: 0 = up, increasing clockwise → atan2(dx, -dy)
        float a = std::atan2(dx, -dy) + wedgeAngle * 0.5f;
        while (a < 0) a += juce::MathConstants<float>::twoPi;
        while (a >= juce::MathConstants<float>::twoPi) a -= juce::MathConstants<float>::twoPi;
        return juce::jlimit(0, N - 1, (int) (a / wedgeAngle));
    }

    juce::Array<Wedge> wedges_;
    int selectedPayload_;
    std::function<void(int)> onSelected_;
    int hoverIdx_ = -1;
};
