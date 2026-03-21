#pragma once

#include "BinaryData.h"
#include "LogoComponent.h"
#include "PresetSelectorComponent.h"
#include "ToggleIconButton.h"
#include "ScaleLatchButton.h"
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
        auto iconArea = juce::Rectangle<float>(padding, padding, contentSize, contentSize);
        Icons::strokeIcon(g, path, iconArea, juce::Colours::white, 1.5f, true, 1.0f);
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
            onClicked();
        }
    }

    void mouseEnter(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    // Fired on click (no drag) — open file chooser
    std::function<void()> onClicked;

    // Fired on drag — should return a temp MIDI file path
    std::function<juce::File()> onDragStarted;

private:
    bool didDrag = false;
};

namespace NavBarConstants {
    constexpr int height = 45;
    constexpr int marginLeft = 22;
    constexpr int marginRight = 10;
    constexpr int buttonSize = ToggleIconButtonConstants::totalSize;  // 36px
    constexpr int buttonGap = 10;
    constexpr int presetSelectorGap = 20;
    // Settings icon scale factor - tweak this to adjust the settings gear icon size
    constexpr float settingsIconScale = 1.0f;
    // Transport button colors
    const juce::Colour inactiveColor = juce::Colours::white;
    const juce::Colour activePlayColor = juce::Colour(0xff4ade80);
    const juce::Colour activeStopColor = juce::Colour(0xfff87171);
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
          compactViewButton(Icons::createCompactViewIcon, Icons::createCompactViewIcon, true, true),
          swapRowButton(Icons::createSwapRowsIcon, Icons::createSwapRowsIcon, true, true)
    {
        // Add logo component
        addAndMakeVisible(logo);

        addAndMakeVisible(presetSelector);

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

        // Add compact view toggle button
        addAndMakeVisible(compactViewButton);
        compactViewButton.onClick = [this](bool toggled) {
            // Show/hide swap button based on compact state
            swapRowButton.setVisible(toggled);
            if (!toggled) {
                // Reset swap state when exiting compact mode
                swapRowButton.setToggled(false);
            }
            if (onCompactViewToggled) {
                onCompactViewToggled(toggled);
            }
        };

        // Add swap row button (hidden by default, shown in compact mode)
        addChildComponent(swapRowButton);
        swapRowButton.onClick = [this](bool toggled) {
            if (onSwapRowClicked) {
                onSwapRowClicked(toggled);
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
    }

    void resized() override {
        using namespace NavBarConstants;

        auto area = getLocalBounds().reduced(marginLeft, 0).withTrimmedRight(marginRight - marginLeft);

        // Helper to add a fixed-size item centered vertically
        auto addItem = [&](juce::FlexBox& fb, juce::Component& comp, float w, float h, float gap = 0.0f) {
            fb.items.add(juce::FlexItem(comp).withWidth(w).withHeight(h).withMargin({0, 0, 0, gap}));
        };

        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.alignItems = juce::FlexBox::AlignItems::center;

        // Left group: preset selector + logo
        addItem(fb, presetSelector, static_cast<float>(presetSelector.getCalculatedWidth()),
                static_cast<float>(presetSelector.getCalculatedHeight()));
        addItem(fb, logo, static_cast<float>(logo.getCalculatedWidth()), static_cast<float>(logo.getCalculatedHeight()), presetSelectorGap);

        // Flexible spacer pushes remaining items to the right
        fb.items.add(juce::FlexItem().withFlex(1.0f));

        // Right group: tool buttons, transport, settings
        float bs = static_cast<float>(buttonSize);
        float bg = static_cast<float>(buttonGap);
        float ss = static_cast<float>(SubdivisionSliderConstants::totalSize);

        addItem(fb, compactViewButton, bs, bs);
        if (swapRowButton.isVisible())
            addItem(fb, swapRowButton, bs, bs, bg);
        addItem(fb, subdivisionSlider, ss, ss, bg);
        addItem(fb, exportButton, bs, bs, bg);
        addItem(fb, scaleLatchButton, bs, bs, bg);
        addItem(fb, playPauseButton, bs, bs, bg);
        addItem(fb, stopButton, bs, bs, bg);
        addItem(fb, midiLearnButton, bs, bs, bg);
        addItem(fb, settingsButton, bs, bs, bg);

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

    // Access to preset selector for wiring up callbacks
    PresetSelectorComponent& getPresetSelector() { return presetSelector; }

    // Access to subdivision slider for MIDI learn wiring
    SubdivisionSlider& getSubdivisionSlider() { return subdivisionSlider; }

    // Set the MIDI learn registry for navbar buttons
    void setSettingsButtonRegistry(MidiLearnRegistry* registry) {
        settingsButton.setRegistry(registry);
        playPauseButton.setRegistry(registry);
        stopButton.setRegistry(registry);
        scaleLatchButton.setRegistry(registry);
    }

private:
    PluginProcessor& processorRef;
    LogoComponent logo;
    PresetSelectorComponent presetSelector;
    ToggleIconButton midiLearnButton;
    ToggleIconButton settingsButton;
    ToggleIconButton playPauseButton;
    ToggleIconButton stopButton;
    ScaleLatchButton scaleLatchButton;
    ToggleIconButton compactViewButton;
    ToggleIconButton swapRowButton;
    ExportButton exportButton;
    SubdivisionSlider subdivisionSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subdivisionAttachment;
};

