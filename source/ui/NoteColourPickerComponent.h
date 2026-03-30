#pragma once

#include "StateListenerComponent.h"
#include "MusicConstants.h"
#include "../data/Params.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace NoteColourPickerConstants {
    constexpr int whiteKeyWidth = 30;
    constexpr int whiteKeyHeight = 60;
    constexpr int blackKeyWidth = 22;
    constexpr int blackKeyHeight = 36;
    constexpr int keySpacing = 1;  // Gap between white keys (adjust to taste)
    constexpr float cornerRadius = 3.0f;
    constexpr float borderWidth = 1.5f;
}

/**
 * NoteColourPickerComponent - Mini piano keyboard for customizing note colours
 *
 * Displays 12 small colored rectangles arranged like a piano keyboard.
 * Clicking a key opens a juce::ColourSelector popup via CallOutBox.
 * When the user picks a color, it writes to the APVTS parameter.
 *
 * Inherits from StateListenerComponent to listen for note colour param changes
 * so it updates when presets are loaded.
 */
class NoteColourPickerComponent : public StateListenerComponent {
public:
    explicit NoteColourPickerComponent(PluginProcessor& processor)
        : StateListenerComponent(processor)
    {
        // Subscribe to InternalState changes (note colours live there, not APVTS)
        listenToInternalState();
        for (int i = 0; i < 12; ++i)
            keyColours_[i] = getState().getNoteColour(i);

        setInterceptsMouseClicks(true, false);
    }

    void paint(juce::Graphics& g) override {
        using namespace NoteColourPickerConstants;

        // Draw white keys first
        for (int i = 0; i < 12; ++i) {
            if (!isBlackKey(i)) {
                auto bounds = getKeyBounds(i);
                g.setColour(keyColours_[i]);
                g.fillRoundedRectangle(bounds, cornerRadius);
                g.setColour(juce::Colours::black);
                g.drawRoundedRectangle(bounds, cornerRadius, borderWidth);
            }
        }

        // Draw black keys on top
        for (int i = 0; i < 12; ++i) {
            if (isBlackKey(i)) {
                auto bounds = getKeyBounds(i);
                g.setColour(keyColours_[i]);
                g.fillRoundedRectangle(bounds, cornerRadius);
                g.setColour(juce::Colours::black);
                g.drawRoundedRectangle(bounds, cornerRadius, borderWidth);
            }
        }
    }

    void mouseDown(const juce::MouseEvent& event) override {
        // Check black keys first (they're on top)
        for (int i = 11; i >= 0; --i) {
            if (isBlackKey(i)) {
                auto bounds = getKeyBounds(i);
                if (bounds.contains(event.position)) {
                    showColourPicker(i);
                    return;
                }
            }
        }
        // Then white keys
        for (int i = 0; i < 12; ++i) {
            if (!isBlackKey(i)) {
                auto bounds = getKeyBounds(i);
                if (bounds.contains(event.position)) {
                    showColourPicker(i);
                    return;
                }
            }
        }
    }

    void onStateChanged() override {
        // Update colours from APVTS
        for (int i = 0; i < 12; ++i) {
            keyColours_[i] = getState().getNoteColour(i);
        }
        repaint();
    }

private:
    std::array<juce::Colour, 12> keyColours_;

    static bool isBlackKey(int noteIndex) {
        // Black keys: Db(1), Eb(3), Gb(6), Ab(8), Bb(10)
        return noteIndex == 1 || noteIndex == 3 || noteIndex == 6 ||
               noteIndex == 8 || noteIndex == 10;
    }

    /**
     * Get the white key index (0-6) for a given note
     * C=0, D=1, E=2, F=3, G=4, A=5, B=6
     */
    static int whiteKeyIndex(int noteIndex) {
        static const int mapping[] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
        return mapping[noteIndex];
    }

    juce::Rectangle<float> getKeyBounds(int noteIndex) const {
        using namespace NoteColourPickerConstants;

        if (isBlackKey(noteIndex)) {
            // Black key - positioned between white keys
            // Find the white key to the left
            int leftWhite = noteIndex - 1;
            int leftWhiteIdx = whiteKeyIndex(leftWhite);
            float whiteX = static_cast<float>(leftWhiteIdx * (whiteKeyWidth + keySpacing));
            float blackX = whiteX + static_cast<float>(whiteKeyWidth) - static_cast<float>(blackKeyWidth) / 2.0f;
            return {blackX, 0.0f, static_cast<float>(blackKeyWidth), static_cast<float>(blackKeyHeight)};
        } else {
            int wki = whiteKeyIndex(noteIndex);
            float x = static_cast<float>(wki * (whiteKeyWidth + keySpacing));
            return {x, 0.0f, static_cast<float>(whiteKeyWidth), static_cast<float>(whiteKeyHeight)};
        }
    }

    void showColourPicker(int noteIndex) {
        auto* selector = new juce::ColourSelector(
            juce::ColourSelector::showColourAtTop |
            juce::ColourSelector::showSliders |
            juce::ColourSelector::showColourspace);

        selector->setCurrentColour(keyColours_[noteIndex]);
        selector->setSize(300, 400);

        // Capture the note index and processor ref for the callback
        selector->addChangeListener(new ColourChangeListener(this, noteIndex));

        auto keyBounds = getKeyBounds(noteIndex);
        auto screenBounds = localAreaToGlobal(keyBounds.toNearestInt());

        juce::CallOutBox::launchAsynchronously(
            std::unique_ptr<juce::Component>(selector),
            screenBounds,
            getTopLevelComponent());
    }

    /**
     * Internal listener that forwards colour changes to the APVTS param
     */
    class ColourChangeListener : public juce::ChangeListener {
    public:
        ColourChangeListener(NoteColourPickerComponent* owner, int noteIndex)
            : owner_(owner), noteIndex_(noteIndex) {}

        void changeListenerCallback(juce::ChangeBroadcaster* source) override {
            if (auto* selector = dynamic_cast<juce::ColourSelector*>(source)) {
                auto newColour = selector->getCurrentColour();
                if (owner_ != nullptr) {
                    owner_->getState().setNoteColour(noteIndex_, newColour);
                }
            }
        }

    private:
        NoteColourPickerComponent* owner_;
        int noteIndex_;
    };

    // Store listeners so they don't get leaked
    // Note: The CallOutBox owns the selector, and the selector owns the listener
    // via addChangeListener, so we don't need to manually manage lifetime here.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteColourPickerComponent)
};
