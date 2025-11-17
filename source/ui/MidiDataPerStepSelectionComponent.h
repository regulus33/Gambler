#pragma once

#include "VerticalBarSlider.h"
#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>

/**
 * MidiDataPerStepSelectionComponent
 *
 * Displays MIDI CC parameter selection for each visible step.
 * Size: 408 x 199 pixels (matches PianoRollComponent)
 *
 * Layout using juce::Grid:
 * - Row 0 (2/10 height): 2 columns (9/10 width empty space, 1/10 width button)
 * - Row 1 (2/10 height): 7 columns (dropdowns / ComboBoxes)
 * - Row 2 (6/10 height): 7 columns (vertical bar sliders)
 * - 4px spacing between all grid items
 */
class MidiDataPerStepSelectionComponent : public juce::Component {
public:
    MidiDataPerStepSelectionComponent()
        : interFont (juce::Typeface::createSystemTypefaceFor (BinaryData::InterVariableFont_ttf,
                                                              BinaryData::InterVariableFont_ttfSize)),
          comboLookAndFeel (interFont)
    {
        // Row 0: Empty space + dummy button
        spacer = std::make_unique<juce::Component>();
        addAndMakeVisible(*spacer);

        dummyButton = std::make_unique<DummyButton>();
        addAndMakeVisible(*dummyButton);

        // This button enters/leaves "CC lane toggle" mode for the 7 lanes below.
        dummyButton->setToggleCallback ([this] (bool isOn)
        {
            isInLaneToggleMode = isOn;
        });


        // Define the 7 StepModulation slot colours using the first seven
        // chromatic colours from PianoRollComponent (C through F/Gb).
        const juce::Colour slotColours[7] = {
            juce::Colour (0xFFFF6B6B), // C  - Red
            juce::Colour (0xFFFF9F6B), // Db - Orange
            juce::Colour (0xFFFFD66B), // D  - Yellow
            juce::Colour (0xFFFFE66B), // Eb - Light yellow
            juce::Colour (0xFFE6FF6B), // E  - Yellow-green
            juce::Colour (0xFF6BFFB4), // F  - Light green
            juce::Colour (0xFF6BFFD6)  // Gb - Cyan-green
        };

        // Row 1: 7 dropdown components (ComboBoxes)
        for (int i = 0; i < 7; ++i) {
            auto slotColour = slotColours[i];

            auto dummy = std::make_unique<DummyComponent>(comboLookAndFeel);
            dummy->setColour(juce::ComboBox::backgroundColourId, slotColour);
            dummy->setColour(juce::ComboBox::textColourId, juce::Colours::black);
            dummy->setColour(juce::ComboBox::arrowColourId, juce::Colours::black);

            const int laneIndex = i;

            dummy->setToggleModeHandlers (
                [this]() { return isInLaneToggleMode; },
                [this, laneIndex]() { toggleLaneEnabled (laneIndex); });

            dummyDropdowns.push_back(std::move(dummy));
            addAndMakeVisible(*dummyDropdowns.back());
        }

        // Row 2: 7 vertical bar sliders (colour-coded to match corresponding dropdowns)
        for (int i = 0; i < 7; ++i) {
            auto slotColour = slotColours[i];

            auto slider = std::make_unique<VerticalBarSlider>();
            slider->setBackgroundColour(slotColour);

            // Use a more saturated and slightly darker version of the slot colour
            // for the adjustable area so it reads as inkier, not just lighter.
            const float hue        = slotColour.getHue();
            const float saturation = juce::jmin (1.0f, slotColour.getSaturation() * 1.35f);
            const float brightness = juce::jmax (0.0f, slotColour.getBrightness() * 0.8f);
            auto barColour         = juce::Colour::fromHSV (hue, saturation, brightness,
                                                            slotColour.getFloatAlpha());
            slider->setBarColour (barColour);

            const int laneIndex = i;

            slider->setToggleModeHandlers (
                [this]() { return isInLaneToggleMode; },
                [this, laneIndex]() { toggleLaneEnabled (laneIndex); });

            sliders.push_back(std::move(slider));
            addAndMakeVisible(*sliders.back());

            sliders.back()->onValueChanged = [i](float value) {
                DBG("Slider " + juce::String(i) + " value: " + juce::String(value));
            };

        }
        laneEnabled.assign (sliders.size(), true);

    }


    // Enable/disable individual CC lanes visually
    void setLaneEnabled (int laneIndex, bool shouldBeEnabled)
    {
        if (laneIndex < 0 || laneIndex >= static_cast<int> (laneEnabled.size()))
            return;

        laneEnabled[static_cast<size_t> (laneIndex)] = shouldBeEnabled;
        const float alpha = shouldBeEnabled ? 1.0f : 0.3f;

        if (laneIndex < static_cast<int> (dummyDropdowns.size()))
            dummyDropdowns[static_cast<size_t> (laneIndex)]->setAlpha (alpha);
        if (laneIndex < static_cast<int> (sliders.size()))
            sliders[static_cast<size_t> (laneIndex)]->setAlpha (alpha);
    }

    void toggleLaneEnabled (int laneIndex)
    {
        if (laneIndex < 0 || laneIndex >= static_cast<int> (laneEnabled.size()))
            return;

        const bool newState = ! laneEnabled[static_cast<size_t> (laneIndex)];
        setLaneEnabled (laneIndex, newState);
    }

    bool isLaneEnabled (int laneIndex) const noexcept
    {
        if (laneIndex < 0 || laneIndex >= static_cast<int> (laneEnabled.size()))
            return false;

        return laneEnabled[static_cast<size_t> (laneIndex)];
    }

    // Bulk access to all 7 lane enabled flags (for wiring to domain / persistence)
    std::vector<bool> getLaneEnabledStates () const
    {
        return laneEnabled;
    }

    void setLaneEnabledStates (const std::vector<bool>& states)
    {
        const auto count = juce::jmin (laneEnabled.size(), states.size());
        for (size_t i = 0; i < count; ++i)
            setLaneEnabled ((int) i, static_cast<bool> (states[i]));
    }


    void paint (juce::Graphics& g) override {
        juce::ignoreUnused (g);
        // Transparent background so the parent component shows through.
        // (We deliberately don't fill here.)
    }

    void resized() override {
        using Track = juce::Grid::TrackInfo;
        using Fr = juce::Grid::Fr;

        juce::Grid grid;

        // Define rows: 2/10, 2/10, 6/10 of total height
        grid.templateRows = {
            Track(Fr(2)),  // Row 0: 2/10 height
            Track(Fr(2)),  // Row 1: 2/10 height
            Track(Fr(6))   // Row 2: 6/10 height
        };

        // Define 7 equal columns (for the 7 steps)
        grid.templateColumns = {
            Track(Fr(1)),
            Track(Fr(1)),
            Track(Fr(1)),
            Track(Fr(1)),
            Track(Fr(1)),
            Track(Fr(1)),
            Track(Fr(1))
        };

        // Set spacing
        grid.columnGap = juce::Grid::Px(4);
        grid.rowGap = juce::Grid::Px(4);

        // Add items to grid
        // Row 0: Spacer spans first 6 columns (9/10 width), Button in last column (1/10 width)
        grid.items.add(juce::GridItem(*spacer).withArea(1, 1, 2, 7));      // Row 1, Col 1-6 (span 6 cols)
        grid.items.add(juce::GridItem(*dummyButton).withArea(1, 7));       // Row 1, Col 7

        // Row 1: 7 dummy dropdowns
        for (int i = 0; i < 7; ++i) {
            grid.items.add(juce::GridItem(*dummyDropdowns[i]).withArea(2, i + 1));  // Row 2, Col 1-7
        }

        // Row 2: 7 sliders
        for (int i = 0; i < 7; ++i) {
            grid.items.add(juce::GridItem(*sliders[i]).withArea(3, i + 1));  // Row 3, Col 1-7
        }

        grid.performLayout(getLocalBounds());
    }

private:
    // LookAndFeel for smaller font and chevron in dropdowns
    class SmallComboBoxLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        explicit SmallComboBoxLookAndFeel (juce::Typeface::Ptr typefaceToUse)
            : interTypeface (std::move (typefaceToUse)) {}

        juce::Font getComboBoxFont(juce::ComboBox& box) override {
            auto baseFont  = juce::LookAndFeel_V4::getComboBoxFont (box);
            auto baseSize  = baseFont.getHeight();
            auto targetSize = baseSize * 0.75f; // keep the 75% scaling

            if (interTypeface != nullptr) {
                juce::Font font (interTypeface);
                font.setHeight (targetSize);
                return font;
            }

            baseFont.setHeight (targetSize);
            return baseFont;
        }

        void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                          int buttonX, int buttonY, int buttonW, int buttonH,
                          juce::ComboBox& box) override
        {
            juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH, box);

            auto bounds = juce::Rectangle<float>(0.0f, 0.0f,
                                                 static_cast<float>(width),
                                                 static_cast<float>(height));

            // Background comes from the ComboBox's background colour (per-slot colour)
            auto background = box.findColour(juce::ComboBox::backgroundColourId, true);
            g.setColour(background);
            g.fillRoundedRectangle(bounds, 3.0f);

            // Draw a much smaller downward chevron (about 1/3 of previous size)
            const float arrowScale = 0.25f;
            auto arrowArea = bounds.removeFromRight(bounds.getHeight()).reduced(4.0f);

            auto size = juce::jmin(arrowArea.getWidth(), arrowArea.getHeight()) * 0.5f * arrowScale;
            auto cx   = arrowArea.getCentreX();
            auto cy   = arrowArea.getCentreY();

            juce::Path p;
            p.startNewSubPath(cx - size, cy - size * 0.5f);
            p.lineTo(cx + size, cy - size * 0.5f);
            p.lineTo(cx,        cy + size * 0.5f);
            p.closeSubPath();

            // Use the ComboBox's configured arrow colour (black for our slots)
            g.setColour(box.findColour(juce::ComboBox::arrowColourId, true));
            g.fillPath(p);
        }
    private:
        juce::Typeface::Ptr interTypeface;
    };

    /**
     * DummyComponent - MIDI CC selection dropdown (juce::ComboBox)
     */
    class DummyComponent : public juce::ComboBox {
    public:
        explicit DummyComponent (SmallComboBoxLookAndFeel& lf)
        {
            setLookAndFeel (&lf);
            setJustificationType (juce::Justification::centred);
            setTextWhenNothingSelected ("CC");

            for (int cc = 0; cc < 128; ++cc)
                addItem ("CC " + juce::String (cc), cc + 1);

            setSelectedId (1, juce::dontSendNotification);
        }

        ~DummyComponent() override
        {
            setLookAndFeel (nullptr);
        }

        // Configure optional toggle-mode behaviour (used by MidiDataPerStepSelectionComponent)
        void setToggleModeHandlers (std::function<bool()> isToggleMode,
                                    std::function<void()> onToggle)
        {
            isInToggleMode = std::move (isToggleMode);
            onToggleLane   = std::move (onToggle);
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (isInToggleMode && isInToggleMode())
            {
                if (onToggleLane)
                    onToggleLane();

                return;
            }

            juce::ComboBox::mouseDown (e);
        }

    private:
        std::function<bool()> isInToggleMode;
        std::function<void()> onToggleLane;
    };

    /**
     * DummyButton - now a toggle button for "lane disable" edit mode
     */
    class DummyButton : public juce::Component {
    public:
        DummyButton() = default;

        void setToggleCallback (std::function<void (bool)> callback)
        {
            onToggleChanged = std::move (callback);
        }

        bool getToggleState() const noexcept
        {
            return isOn;
        }

        void setToggleState (bool shouldBeOn)
        {
            if (isOn == shouldBeOn)
                return;

            isOn = shouldBeOn;
            repaint();

            if (onToggleChanged)
                onToggleChanged (isOn);
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (e.mouseWasDraggedSinceMouseDown())
                return;

            setToggleState (! isOn);
        }

        void paint (juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();

            auto background = isOn ? juce::Colour (0xff484848)
                                   : juce::Colour (0xff2c2c2c);
            g.setColour (background);
            g.fillRoundedRectangle (bounds, 3.0f);

            g.setColour (juce::Colours::white.withAlpha (isOn ? 1.0f : 0.6f));
            g.drawRoundedRectangle (bounds.reduced (1.0f), 3.0f, 2.0f);

            g.setColour (juce::Colours::white);
            g.setFont (juce::Font (12.0f, juce::Font::bold));
            g.drawText ("CC lanes", getLocalBounds(), juce::Justification::centred);
        }

    private:
        bool isOn { false };
        std::function<void (bool)> onToggleChanged;
    };

    juce::Typeface::Ptr interFont;
    SmallComboBoxLookAndFeel comboLookAndFeel;

    std::unique_ptr<juce::Component> spacer;
    std::unique_ptr<DummyButton> dummyButton;
    std::vector<std::unique_ptr<DummyComponent>> dummyDropdowns;
    std::vector<std::unique_ptr<VerticalBarSlider>> sliders;

    bool isInLaneToggleMode { false };
    std::vector<bool> laneEnabled;
};

