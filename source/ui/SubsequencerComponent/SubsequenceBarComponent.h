#pragma once

#include "SingleBarComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>
#include <memory>

/**
 * SubsequenceBarComponent - A vertical stack of 8 clickable rectangle components
 */
class SubsequenceBarComponent : public juce::Component {
public:
    SubsequenceBarComponent() {
        // Create and add all bar components
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            bars[i] = std::make_unique<SingleBarComponent>();
            addAndMakeVisible (bars[i].get());

            // Set MIDI control ID (SUBSEQ_BAR_0 through SUBSEQ_BAR_7)
            bars[i]->setMidiControlId(static_cast<MidiControlId>(
                static_cast<int>(MidiControlId::SUBSEQ_BAR_0) + i));

            // Setup click callback for each bar (select for editing)
            bars[i]->onClick = [this, i]() {
                if (onBarClicked)
                    onBarClicked (i);
            };

            // Setup double-click callback (set as active/current)
            bars[i]->onDoubleClick = [this, i]() {
                if (onBarDoubleClicked)
                    onBarDoubleClicked(i);
            };
        }
    }

    /**
     * Set the MIDI learn registry for all bars
     */
    void setRegistry(MidiLearnRegistry* reg) {
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            bars[i]->setRegistry(reg);
        }
    }

    void resized() override {
        using namespace SubsequenceControlsConstants;

        for (int i = 0; i < numBars; ++i) {
            int y = i * (barHeight + barSpacing);
            bars[i]->setBounds (0, y, static_cast<int>(barWidth), barHeight);
        }
    }

    /**
     * Set the color of a single bar
     * @param index Bar index (0-7)
     * @param color The new color
     */
    void setBarColor (int index, juce::Colour color) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            bars[index]->setColor (color);
        }
    }

    /**
     * Get the color of a single bar
     * @param index Bar index (0-7)
     */
    juce::Colour getBarColor (int index) const {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            return bars[index]->getColor();
        }
        return juce::Colour (SubsequenceControlsConstants::defaultGreyHex);
    }

    void setBarBorderStyle(int index, BarBorderStyle style) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            bars[index]->setBorderStyle(style);
        }
    }

    BarBorderStyle getBarBorderStyle(int index) const {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            return bars[index]->getBorderStyle();
        }
        return BarBorderStyle::none;
    }

    void setBarNoteLabel(int index, const juce::String& label) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            bars[index]->setNoteLabel(label);
        }
    }

    void setBarCurrent(int index, bool current) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            bars[index]->setCurrent(current);
        }
    }

    void setBarPlaceholder(int index, bool placeholder) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            bars[index]->setPlaceholder(placeholder);
        }
    }

    /**
     * Get direct access to a bar component
     * @param index Bar index (0-7)
     */
    SingleBarComponent* getBar (int index) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            return bars[index].get();
        }
        return nullptr;
    }

    // Callback for bar clicks (single = select, double = set active)
    std::function<void(int)> onBarClicked;
    std::function<void(int)> onBarDoubleClicked;

private:
    std::array<std::unique_ptr<SingleBarComponent>, SubsequenceControlsConstants::numBars> bars;
};
