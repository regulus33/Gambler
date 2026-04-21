#pragma once

#include "StateListenerComponent.h"
#include "NoteColourPickerComponent.h"
#include "StyledDropdown/StyledDropdown.h"
#include "../data/Params.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>

namespace GlobalScaleQuantizerConstants {
    // Key colours
    const juce::Colour selectedKeyColour = juce::Colour(0xFFFF6B6B);
    const juce::Colour unselectedWhiteKeyColour = juce::Colours::white;
    const juce::Colour unselectedBlackKeyColour = juce::Colours::white;

    // Key dimensions (reuse from NoteColourPickerConstants)
    constexpr int whiteKeyWidth = NoteColourPickerConstants::whiteKeyWidth;
    constexpr int whiteKeyHeight = NoteColourPickerConstants::whiteKeyHeight;
    constexpr int blackKeyWidth = NoteColourPickerConstants::blackKeyWidth;
    constexpr int blackKeyHeight = NoteColourPickerConstants::blackKeyHeight;
    constexpr int keySpacing = NoteColourPickerConstants::keySpacing;
    constexpr float cornerRadius = NoteColourPickerConstants::cornerRadius;
    constexpr float borderWidth = NoteColourPickerConstants::borderWidth;

    // Layout
    constexpr int keyboardWidth = 7 * (whiteKeyWidth + keySpacing) - keySpacing; // 216px
    constexpr int dropdownHeight = 28;
    constexpr int dropdownRowSpacing = 8;
    constexpr int keyboardBottomMargin = 12;
    constexpr int labelWidth = 50;
    constexpr int labelDropdownGap = 6;

    // Total height: keyboard + margin + 2 dropdown rows + spacing between them
    constexpr int totalHeight = whiteKeyHeight + keyboardBottomMargin
        + dropdownHeight + dropdownRowSpacing + dropdownHeight;
}

class GlobalScaleQuantizerComponent : public StateListenerComponent {
public:
    explicit GlobalScaleQuantizerComponent(PluginProcessor& processor)
        : StateListenerComponent(processor)
    {
        using namespace GlobalScaleQuantizerConstants;

        // Listen to global scale note params
        for (int i = 0; i < 12; ++i) {
            listenToParameter(Params::Keys::ID::globalScaleNote(i));
            keyStates_[i] = getState().getGlobalScaleNote(i);
        }

        // Scale type dropdown
        scaleLabel_.setText("Scale", juce::dontSendNotification);
        scaleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        scaleLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(scaleLabel_);

        scaleTypeDropdown_.setItems({
            "Major", "Natural Minor", "Harmonic Minor", "Melodic Minor",
            "Pentatonic Major", "Pentatonic Minor", "Blues",
            "Dorian", "Mixolydian", "Lydian", "Phrygian", "Locrian",
            "Whole Tone", "Chromatic", "Custom"
        });
        scaleTypeDropdown_.setSelectedId(14, juce::dontSendNotification);
        scaleTypeDropdown_.onChange = [this] {
            if (!suppressDropdownUpdate_)
                onScaleOrKeyChanged();
        };
        addAndMakeVisible(scaleTypeDropdown_);

        // Key dropdown
        keyLabel_.setText("Key", juce::dontSendNotification);
        keyLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        keyLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(keyLabel_);

        keyDropdown_.setItems({
            "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
        });
        keyDropdown_.setSelectedId(1, juce::dontSendNotification);
        keyDropdown_.onChange = [this] {
            if (!suppressDropdownUpdate_)
                onScaleOrKeyChanged();
        };
        addAndMakeVisible(keyDropdown_);

        setInterceptsMouseClicks(true, true);
    }

    void paint(juce::Graphics& g) override {
        using namespace GlobalScaleQuantizerConstants;

        auto keyboardArea = getKeyboardArea();

        // Draw white keys first
        for (int i = 0; i < 12; ++i) {
            if (!isBlackKey(i)) {
                auto bounds = getKeyBounds(i, keyboardArea);
                g.setColour(keyStates_[i] ? selectedKeyColour : unselectedWhiteKeyColour);
                g.fillRoundedRectangle(bounds, cornerRadius);
                g.setColour(juce::Colours::black);
                g.drawRoundedRectangle(bounds, cornerRadius, borderWidth);
            }
        }

        // Draw black keys on top
        for (int i = 0; i < 12; ++i) {
            if (isBlackKey(i)) {
                auto bounds = getKeyBounds(i, keyboardArea);
                g.setColour(keyStates_[i] ? selectedKeyColour : unselectedBlackKeyColour);
                g.fillRoundedRectangle(bounds, cornerRadius);
                g.setColour(juce::Colours::black);
                g.drawRoundedRectangle(bounds, cornerRadius, borderWidth);
            }
        }
    }

    void mouseDown(const juce::MouseEvent& event) override {
        auto keyboardArea = getKeyboardArea();

        // Check black keys first (they're on top)
        for (int i = 11; i >= 0; --i) {
            if (isBlackKey(i)) {
                auto bounds = getKeyBounds(i, keyboardArea);
                if (bounds.contains(event.position)) {
                    toggleKey(i);
                    return;
                }
            }
        }
        // Then white keys
        for (int i = 0; i < 12; ++i) {
            if (!isBlackKey(i)) {
                auto bounds = getKeyBounds(i, keyboardArea);
                if (bounds.contains(event.position)) {
                    toggleKey(i);
                    return;
                }
            }
        }
    }

    void onStateChanged() override {
        // Read global scale to sync keyboard display
        for (int i = 0; i < 12; ++i)
            keyStates_[i] = getState().getGlobalScaleNote(i);
        repaint();
    }

    void resized() override {
        using namespace GlobalScaleQuantizerConstants;

        // Dropdown rows sit below the keyboard, right-aligned to keyboard width
        int y = whiteKeyHeight + keyboardBottomMargin;
        int dropdownWidth = keyboardWidth - labelWidth - labelDropdownGap;

        // Scale row
        scaleLabel_.setBounds(0, y, labelWidth, dropdownHeight);
        scaleTypeDropdown_.setBounds(labelWidth + labelDropdownGap, y, dropdownWidth, dropdownHeight);

        // Key row
        y += dropdownHeight + dropdownRowSpacing;
        keyLabel_.setBounds(0, y, labelWidth, dropdownHeight);
        keyDropdown_.setBounds(labelWidth + labelDropdownGap, y, dropdownWidth, dropdownHeight);
    }

private:
    juce::Label scaleLabel_;
    StyledDropdown scaleTypeDropdown_;
    juce::Label keyLabel_;
    StyledDropdown keyDropdown_;
    std::array<bool, 12> keyStates_ {};
    bool suppressDropdownUpdate_ = false;

    // Scale interval definitions
    static const std::vector<std::vector<int>>& getScaleIntervals() {
        static const std::vector<std::vector<int>> intervals = {
            {0, 2, 4, 5, 7, 9, 11},       // Major
            {0, 2, 3, 5, 7, 8, 10},        // Natural Minor
            {0, 2, 3, 5, 7, 8, 11},        // Harmonic Minor
            {0, 2, 3, 5, 7, 9, 11},        // Melodic Minor
            {0, 2, 4, 7, 9},               // Pentatonic Major
            {0, 3, 5, 7, 10},              // Pentatonic Minor
            {0, 3, 5, 6, 7, 10},           // Blues
            {0, 2, 3, 5, 7, 9, 10},        // Dorian
            {0, 2, 4, 5, 7, 9, 10},        // Mixolydian
            {0, 2, 4, 6, 7, 9, 11},        // Lydian
            {0, 1, 3, 5, 7, 8, 10},        // Phrygian
            {0, 1, 3, 5, 6, 8, 10},        // Locrian
            {0, 2, 4, 6, 8, 10},           // Whole Tone
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, // Chromatic
        };
        return intervals;
    }

    static std::array<bool, 12> computeScaleNotes(int scaleIndex, int keyIndex) {
        std::array<bool, 12> notes {};
        const auto& intervals = getScaleIntervals();
        if (scaleIndex < 0 || scaleIndex >= static_cast<int>(intervals.size()))
            return notes;

        for (int interval : intervals[static_cast<size_t>(scaleIndex)])
            notes[static_cast<size_t>((interval + keyIndex) % 12)] = true;

        return notes;
    }

    void onScaleOrKeyChanged() {
        int scaleId = scaleTypeDropdown_.getSelectedId();
        if (scaleId == 15) return; // Custom — don't recompute

        int scaleIndex = scaleId - 1; // 0-based
        int keyIndex = keyDropdown_.getSelectedId() - 1; // 0-based

        keyStates_ = computeScaleNotes(scaleIndex, keyIndex);
        applyScale();
        repaint();
    }

    void toggleKey(int noteIndex) {
        keyStates_[static_cast<size_t>(noteIndex)] = !keyStates_[static_cast<size_t>(noteIndex)];
        applyScale();

        // Set dropdown to Custom
        suppressDropdownUpdate_ = true;
        scaleTypeDropdown_.setSelectedId(15, juce::dontSendNotification);
        suppressDropdownUpdate_ = false;

        repaint();
    }

    void applyScale() {
        std::array<int, 12> notes {};
        for (int i = 0; i < 12; ++i)
            notes[static_cast<size_t>(i)] = keyStates_[static_cast<size_t>(i)] ? 1 : 0;

        getState().setGlobalScaleNotes(notes);
    }

    static bool isBlackKey(int noteIndex) {
        return noteIndex == 1 || noteIndex == 3 || noteIndex == 6 ||
               noteIndex == 8 || noteIndex == 10;
    }

    static int whiteKeyIndex(int noteIndex) {
        static const int mapping[] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
        return mapping[noteIndex];
    }

    juce::Rectangle<float> getKeyboardArea() const {
        return getLocalBounds().toFloat();
    }

    juce::Rectangle<float> getKeyBounds(int noteIndex, const juce::Rectangle<float>& keyboardArea) const {
        using namespace GlobalScaleQuantizerConstants;

        if (isBlackKey(noteIndex)) {
            int leftWhite = noteIndex - 1;
            int leftWhiteIdx = whiteKeyIndex(leftWhite);
            float whiteX = keyboardArea.getX() + static_cast<float>(leftWhiteIdx * (whiteKeyWidth + keySpacing));
            float blackX = whiteX + static_cast<float>(whiteKeyWidth) - static_cast<float>(blackKeyWidth) / 2.0f;
            return {blackX, keyboardArea.getY(), static_cast<float>(blackKeyWidth), static_cast<float>(blackKeyHeight)};
        } else {
            int wki = whiteKeyIndex(noteIndex);
            float x = keyboardArea.getX() + static_cast<float>(wki * (whiteKeyWidth + keySpacing));
            return {x, keyboardArea.getY(), static_cast<float>(whiteKeyWidth), static_cast<float>(whiteKeyHeight)};
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalScaleQuantizerComponent)
};
