#pragma once

#include "BinaryData.h"
#include "ToggleIconButton.h"
#include "ScaleLatchButton.h"
#include "DestructiveToggleButton.h"
#include "sliders/SubdivisionSlider.h"
#include "../data/Params.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

#include "../PluginProcessor.h"

/**
 * ExportButton
 *
 * A button that supports both click-to-save and drag-to-export.
 * - Click (mouseUp without drag): fires onClicked callback (opens file chooser)
 * - Drag (mouseDrag past threshold): fires onDragStarted to get a temp file,
 *   then initiates external file drag-and-drop into a DAW.
 *
 * Renders the export icon using the same style as ToggleIconButton.
 */
class ExportButton : public juce::Component {
public:
    ExportButton() {}

    void paint(juce::Graphics& g) override {
        using namespace ToggleIconButtonConstants;
        auto path = Icons::createExportIcon();
        // Derive padding and content from actual bounds so the icon scales with the component
        float boundsSize = static_cast<float>(juce::jmin(getWidth(), getHeight()));
        float paddingRatio = static_cast<float>(padding) / static_cast<float>(totalSize);
        float actualPadding = boundsSize * paddingRatio;
        float actualContent = boundsSize - actualPadding * 2.0f;
        auto iconArea = juce::Rectangle<float>(actualPadding, actualPadding, actualContent, actualContent);
        Icons::strokeIcon(g, path, iconArea, juce::Colours::white, 1.5f, true, 1.0f);

        // Draw click flash overlay
        clickFlash_.paint(g, getLocalBounds());
    }

    void mouseDown(const juce::MouseEvent&) override {
        didDrag = false;
    }

    void mouseDrag(const juce::MouseEvent& event) override {
        if (!didDrag && event.getDistanceFromDragStart() > 8) {
            didDrag = true;
            if (onDragStarted) {
                auto tempFile = onDragStarted();
                if (tempFile != juce::File()) {
                    juce::DragAndDropContainer::performExternalDragDropOfFiles(
                        {tempFile.getFullPathName()}, false, this);
                }
            }
        }
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (!didDrag && onClicked) {
            clickFlash_.trigger();
            onClicked();
        }
    }

    void mouseEnter(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void parentHierarchyChanged() override {
        if (getTopLevelComponent() != nullptr)
            clickFlash_.attachTo(this);
    }

    // Fired on click (no drag) — open file chooser
    std::function<void()> onClicked;

    // Fired on drag — should return a temp MIDI file path
    std::function<juce::File()> onDragStarted;

private:
    bool didDrag = false;
    ClickFlash clickFlash_;
};

namespace NavBarConstants {
    constexpr int height = 45;
    constexpr int marginLeft = 8;
    constexpr int marginRight = 35;

    // Per-component scale factors — tweak individually to resize each control
    constexpr float compactTopScale    = 0.8f;   // compact top row toggle button
    constexpr float compactBottomScale = 0.8f;   // compact bottom row toggle button
    constexpr float subdivisionScale   = 0.8f;   // subdivision slider
    constexpr float exportScale        = 0.8f;   // export / drag-to-DAW button
    constexpr float scaleLatchScale    = 0.8f;   // scale latch button
    constexpr float playPauseScale     = 0.8f;   // play / pause button
    constexpr float stopScale          = 0.8f;   // stop button
    constexpr float midiLearnScale     = 0.8f;   // MIDI learn button
    constexpr float bakeScaleScale     = 0.8f;   // bake scale to steps button
    constexpr float bakeRangeScale     = 0.8f;   // bake range to steps button
    constexpr float settingsScale      = 0.8f;   // settings gear button

    // Base sizes (before scaling)
    constexpr float baseButtonSize     = static_cast<float>(ToggleIconButtonConstants::totalSize);
    constexpr float baseSubdivSize     = static_cast<float>(SubdivisionSliderConstants::totalSize);

    // Spacing — tweak these to adjust gaps
    constexpr int buttonGap = 6;                   // gap between icon buttons

    // Settings icon scale factor - tweak this to adjust the settings gear icon size
    constexpr float settingsIconScale = 1.0f;
    // Transport button colors
    const juce::Colour inactiveColor = juce::Colours::white;
    const juce::Colour activePlayColor = juce::Colour(0xff4ade80);
    const juce::Colour activeStopColor = juce::Colour(0xfff87171);

    // Compact mode pulse glow color (gold)
    const juce::Colour compactPulseColor = juce::Colour(0xFFFFD700);
}

/**
 * NavBarComponent
 *
 * A navigation bar component that displays:
 * - Logo (left side, centered vertically)
 * - Preset selector (right of logo)
 * - Version label (right side, left of settings button)
 * - Settings button (right side)
 *
 * This component is displayed at the top of all screens.
 */
class NavBarComponent : public juce::Component {
public:
    explicit NavBarComponent(PluginProcessor& processor)
        : processorRef(processor),
          settingsButton(Icons::createSettingsIcon, Icons::createCloseIcon, true, false, NavBarConstants::settingsIconScale, Icons::closeIconScale),
          midiLearnButton(Icons::createMidiLearnIcon, Icons::createMidiLearnIcon, false, true),
          playPauseButton(Icons::createPlayIcon, Icons::createPauseIcon, false, false),
          stopButton(Icons::createStopIcon, Icons::createStopIcon, false, false),
          scaleLatchButton(processor),
          bakeScaleButton(processor, Params::Keys::ID::PREF_DESTRUCTIVE_SCALE,
                          MidiControlId::DESTRUCTIVE_SCALE, Icons::createBakeScaleIcon),
          bakeRangeButton(processor, Params::Keys::ID::PREF_DESTRUCTIVE_RANGE,
                          MidiControlId::DESTRUCTIVE_RANGE, Icons::createBakeRangeIcon),
          compactTopButton(Icons::createCompactTopIcon, Icons::createCompactTopIcon, true, true),
          compactBottomButton(Icons::createCompactViewIcon, Icons::createCompactViewIcon, true, true)
    {
        // Add MIDI learn button
        addAndMakeVisible(midiLearnButton);
        midiLearnButton.onClick = [this](bool toggled) {
            if (onMidiLearnClicked) {
                onMidiLearnClicked(toggled);
            }
        };

        // Add settings button
        addAndMakeVisible(settingsButton);
        settingsButton.setMidiControlId(MidiControlId::SETTINGS_TOGGLE);
        settingsButton.onClick = [this](bool toggled) {
            if (onSettingsClicked) {
                onSettingsClicked();
            }
        };

        // Add play/pause button
        addAndMakeVisible(playPauseButton);
        playPauseButton.setMidiControlId(MidiControlId::PLAY_PAUSE);
        playPauseButton.onClick = [this](bool toggled) {
            if (onPlayPauseClicked) {
                onPlayPauseClicked();
            }
        };

        // Add stop button
        addAndMakeVisible(stopButton);
        stopButton.setMidiControlId(MidiControlId::STOP);
        stopButton.onClick = [this](bool toggled) {
            if (onStopClicked) {
                onStopClicked();
            }
        };

        // Add scale latch button
        addAndMakeVisible(scaleLatchButton);
        // Note: No callback needed - syncs with APVTS parameter

        // Add destructive toggle buttons (bake scale/range to steps)
        addAndMakeVisible(bakeScaleButton);
        addAndMakeVisible(bakeRangeButton);

        // Add export button (supports click-to-save and drag-to-DAW)
        addAndMakeVisible(exportButton);
        exportButton.onClicked = [this]() {
            if (onExportClicked) {
                onExportClicked();
            }
        };
        exportButton.onDragStarted = [this]() -> juce::File {
            if (onExportDragStarted) {
                return onExportDragStarted();
            }
            return {};
        };

        // Compact top button: toggles Default ↔ Compact Top Row
        addAndMakeVisible(compactTopButton);
        compactTopButton.onClick = [this](bool /*toggled*/) {
            if (compactTopButton.getToggled()) {
                // Entering compact top mode — untoggle bottom if active
                compactBottomButton.setToggled(false);
                compactBottomButton.stopPulse();
                compactTopButton.startPulse(NavBarConstants::compactPulseColor);
                if (onCompactViewToggled) onCompactViewToggled(true);
                if (onSwapRowClicked) onSwapRowClicked(true);
            } else {
                // Returning to default
                compactTopButton.stopPulse();
                if (onCompactViewToggled) onCompactViewToggled(false);
            }
        };

        // Compact bottom button: toggles Default ↔ Compact Bottom Row
        addAndMakeVisible(compactBottomButton);
        compactBottomButton.onClick = [this](bool /*toggled*/) {
            if (compactBottomButton.getToggled()) {
                // Entering compact bottom mode — untoggle top if active
                compactTopButton.setToggled(false);
                compactTopButton.stopPulse();
                compactBottomButton.startPulse(NavBarConstants::compactPulseColor);
                if (onCompactViewToggled) onCompactViewToggled(true);
                if (onSwapRowClicked) onSwapRowClicked(false);
            } else {
                // Returning to default
                compactBottomButton.stopPulse();
                if (onCompactViewToggled) onCompactViewToggled(false);
            }
        };

        // Add subdivision slider with APVTS attachment
        addAndMakeVisible(subdivisionSlider);
        subdivisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.getAPVTS(), Params::Keys::ID::CLOCK_SUBDIVISION,
            subdivisionSlider.getSlider());

        // Set MIDI learn button colors and stroke width
        // White when OFF (state 1), grey when ON/active (state 2)
        midiLearnButton.setColor1(NavBarConstants::inactiveColor);
        midiLearnButton.setColor2(juce::Colours::grey);

        // Set transport button colors
        // Play/Pause: white when stopped/paused (state 1), green when playing (state 2)
        playPauseButton.setColor1(NavBarConstants::inactiveColor);
        playPauseButton.setColor2(NavBarConstants::activePlayColor);

        // Stop: white normally (we'll update this dynamically based on state)
        stopButton.setColor1(NavBarConstants::inactiveColor);
        stopButton.setColor2(NavBarConstants::inactiveColor);

        // Disable click flash on transport buttons (they have their own color feedback)
        playPauseButton.setFlashEnabled(false);
        stopButton.setFlashEnabled(false);
    }

    void resized() override {
        using namespace NavBarConstants;

        auto area = getLocalBounds();

        // Helper to add a fixed-size item centered vertically
        auto addItem = [&](juce::FlexBox& fb, juce::Component& comp, float w, float h, float gap = 0.0f) {
            fb.items.add(juce::FlexItem(comp).withWidth(w).withHeight(h).withMargin({0, 0, 0, gap}));
        };

        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.alignItems = juce::FlexBox::AlignItems::center;
        fb.justifyContent = juce::FlexBox::JustifyContent::spaceAround;

        auto sz = [](float scale) { return baseButtonSize * scale; };
        float ss = baseSubdivSize * subdivisionScale;

        addItem(fb, compactTopButton, sz(compactTopScale), sz(compactTopScale));
        addItem(fb, compactBottomButton, sz(compactBottomScale), sz(compactBottomScale));
        addItem(fb, subdivisionSlider, ss, ss);
        addItem(fb, exportButton, sz(exportScale), sz(exportScale));
        addItem(fb, scaleLatchButton, sz(scaleLatchScale), sz(scaleLatchScale));
        addItem(fb, bakeScaleButton, sz(bakeScaleScale), sz(bakeScaleScale));
        addItem(fb, bakeRangeButton, sz(bakeRangeScale), sz(bakeRangeScale));
        addItem(fb, playPauseButton, sz(playPauseScale), sz(playPauseScale));
        addItem(fb, stopButton, sz(stopScale), sz(stopScale));
        addItem(fb, midiLearnButton, sz(midiLearnScale), sz(midiLearnScale));
        addItem(fb, settingsButton, sz(settingsScale), sz(settingsScale));

        fb.performLayout(area.toFloat());
    }

    // Callback when settings button is clicked
    std::function<void()> onSettingsClicked;

    // Callback when MIDI learn button is clicked
    // Parameter: true = MIDI learn mode ON, false = MIDI learn mode OFF
    std::function<void(bool)> onMidiLearnClicked;

    // Callback when play/pause button is clicked
    std::function<void()> onPlayPauseClicked;

    // Callback when stop button is clicked
    std::function<void()> onStopClicked;

    // Callback when export button is clicked (opens file chooser)
    std::function<void()> onExportClicked;

    // Callback when export button is dragged (returns temp MIDI file for D&D)
    std::function<juce::File()> onExportDragStarted;

    // Callback when compact view button is toggled
    // Parameter: true = compact mode ON, false = compact mode OFF
    std::function<void(bool)> onCompactViewToggled;

    // Callback when swap row button is clicked in compact mode
    // Parameter: true = showing top row, false = showing bottom row
    std::function<void(bool)> onSwapRowClicked;

    // Programmatically enter compact bottom row mode (same as clicking compactBottomButton)
    void enterCompactBottomMode() {
        if (!compactBottomButton.getToggled()) {
            compactBottomButton.setToggled(true);
            compactTopButton.setToggled(false);
            compactTopButton.stopPulse();
            compactBottomButton.startPulse(NavBarConstants::compactPulseColor);
            if (onCompactViewToggled) onCompactViewToggled(true);
            if (onSwapRowClicked) onSwapRowClicked(false);
        }
    }

    juce::Rectangle<int> getExportButtonScreenBounds() const {
        return exportButton.localAreaToGlobal(exportButton.getLocalBounds());
    }

    juce::Component* getExportButton() { return &exportButton; }

    // Public method to set settings button toggle state (for ApplicationComponent)
    void setSettingsToggled(bool toggled) {
        settingsButton.setToggled(toggled);
    }

    // Public method to set MIDI learn button toggle state (for ApplicationComponent)
    void setMidiLearnToggled(bool toggled) {
        midiLearnButton.setToggled(toggled);
    }

    // Public method to set play/pause button state based on sequencer state
    // When playing: shows pause icon (toggled = true) with green color
    // When stopped/paused: shows play icon (toggled = false) with white color
    void setPlayPauseState(bool isPlaying) {
        playPauseButton.setToggled(isPlaying);
    }

    // Public method to set stop button color based on sequencer state
    // When stopped: shows red color
    // When playing/paused: shows white color
    void setStopButtonActive(bool isStopped) {
        if (isStopped) {
            stopButton.setColor1(NavBarConstants::activeStopColor);
        } else {
            stopButton.setColor1(NavBarConstants::inactiveColor);
        }
    }

    // Access to subdivision slider for MIDI learn wiring
    SubdivisionSlider& getSubdivisionSlider() { return subdivisionSlider; }

    // Set the MIDI learn registry for navbar buttons
    void setSettingsButtonRegistry(MidiLearnRegistry* registry) {
        settingsButton.setRegistry(registry);
        playPauseButton.setRegistry(registry);
        stopButton.setRegistry(registry);
        scaleLatchButton.setRegistry(registry);
        bakeScaleButton.setRegistry(registry);
        bakeRangeButton.setRegistry(registry);
    }

private:
    PluginProcessor& processorRef;
    ToggleIconButton midiLearnButton;
    ToggleIconButton settingsButton;
    ToggleIconButton playPauseButton;
    ToggleIconButton stopButton;
    ScaleLatchButton scaleLatchButton;
    DestructiveToggleButton bakeScaleButton;
    DestructiveToggleButton bakeRangeButton;
    ToggleIconButton compactTopButton;
    ToggleIconButton compactBottomButton;
    ExportButton exportButton;
    SubdivisionSlider subdivisionSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subdivisionAttachment;
};
