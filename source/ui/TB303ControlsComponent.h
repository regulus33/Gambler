#pragma once

#include "StateListenerComponent.h"
#include "BinaryData.h"
#include "../data/Params.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <random>

/**
 * TB303ControlsComponent
 *
 * Replaces the PianoRoll + MainControls area when the view is in "303 Synth mode".
 * Displays per-step 303 parameters (7 knobs) and global 303 parameters (5 knobs).
 *
 * Layout (622 × 241 px, two rows):
 *   Row 0 (~50px): "303" title label + 5 global knobs (Tune, Vol, Drive, DistMix, Reverb)
 *   Row 1 (~191px): 7 per-step knobs (Cutoff, Res, EnvMod, Decay, Accent, Wave, Slew)
 *
 * Per-step params are stored in stepModValue[0..6] via SequenceState.
 * Global params write directly to APVTS float parameters.
 */
class TB303ControlsComponent : public StateListenerComponent {
public:
    explicit TB303ControlsComponent(PluginProcessor& processor)
        : StateListenerComponent(processor)
    {
        // Load font
        interFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        // Title label
        titleLabel.setText("303 SYNTH", juce::dontSendNotification);
        titleLabel.setFont(juce::FontOptions(interFont).withHeight(13.f).withStyle("Bold"));
        titleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(titleLabel);

        // ---- Per-step knobs (row 1) ----
        static const char* perStepLabels[] = { "CUTOFF", "RESON", "ENVMOD", "DECAY", "ACCENT", "WAVE", "SLEW" };
        // Slot colours matching MidiDataPerStepSelectionComponent
        static const juce::uint32 slotColours[] = {
            0xFFFF6B6B,  // C-Red
            0xFFFF9F6B,  // Db-Orange
            0xFFFFD66B,  // D-Yellow
            0xFFFFE66B,  // Eb-Light yellow
            0xFFE6FF6B,  // E-Yellow-green
            0xFF6BFFB4,  // F-Light green
            0xFF6BFFD6   // Gb-Cyan-green
        };

        for (int i = 0; i < 7; ++i) {
            auto& knob = perStepKnobs[i];
            knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
            knob.setRange(0.0, 127.0, 1.0);
            knob.setValue(64.0, juce::dontSendNotification);
            knob.setColour(juce::Slider::rotarySliderFillColourId,
                           juce::Colour(slotColours[i]));
            knob.setColour(juce::Slider::rotarySliderOutlineColourId,
                           juce::Colour(slotColours[i]).withAlpha(0.3f));
            knob.setColour(juce::Slider::thumbColourId, juce::Colours::white);
            knob.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.8f));
            knob.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            knob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            addAndMakeVisible(knob);

            auto& lbl = perStepLabels_[i];
            lbl.setText(perStepLabels[i], juce::dontSendNotification);
            lbl.setFont(juce::FontOptions(interFont).withHeight(9.f));
            lbl.setColour(juce::Label::textColourId, juce::Colour(slotColours[i]).withAlpha(0.85f));
            lbl.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbl);

            const int slot = i;
            knob.onValueChange = [this, slot]() {
                auto& state = getState();
                int selectedStep = state.getSelectedStepForEditing();
                state.setStepModValue(selectedStep, slot, (int)perStepKnobs[slot].getValue());
            };
        }

        // ---- Global knobs (row 0) ----
        static const char* globalLabels[]    = { "TUNE", "VOL", "DRIVE", "DISTMIX", "REVERB" };
        static const char* globalParamIds[]  = {
            "synth_tuning", "synth_volume", "synth_dist_drive", "synth_dist_mix", "synth_reverb_wet"
        };
        static const double globalMins[]  = { -12.0, 0.0, 0.0, 0.0, 0.0 };
        static const double globalMaxs[]  = {  12.0, 1.0, 1.0, 1.0, 1.0 };
        static const double globalDefs[]  = {   0.0, 0.75, 0.0, 0.0, 0.0 };

        for (int i = 0; i < 5; ++i) {
            auto& knob = globalKnobs[i];
            knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 44, 14);
            knob.setRange(globalMins[i], globalMaxs[i], 0.0);
            knob.setValue(globalDefs[i], juce::dontSendNotification);
            knob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF6BD6FF));
            knob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF6BD6FF).withAlpha(0.3f));
            knob.setColour(juce::Slider::thumbColourId, juce::Colours::white);
            knob.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.8f));
            knob.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            knob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            addAndMakeVisible(knob);

            auto& lbl = globalLabels_[i];
            lbl.setText(globalLabels[i], juce::dontSendNotification);
            lbl.setFont(juce::FontOptions(interFont).withHeight(9.f));
            lbl.setColour(juce::Label::textColourId, juce::Colour(0xFF6BD6FF).withAlpha(0.85f));
            lbl.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbl);

            // Attach global knob to APVTS param
            globalAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                getProcessor().getAPVTS(), globalParamIds[i], knob);
        }

        // ---- Randomize button ----
        randomizeBtn.setButtonText("RND");
        randomizeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF484848));
        randomizeBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.8f));
        randomizeBtn.onClick = [this]() { randomizePerStepParams(); };
        addAndMakeVisible(randomizeBtn);

        // Subscribe to step mod values (all steps) + selected step changes
        listenToModulations(0, 63);
        listenToParameter(Params::Keys::ID::SEQ_SELECTED_STEP);
        // Global params
        listenToParameter(Params::Keys::ID::SYNTH_TUNING);
        listenToParameter(Params::Keys::ID::SYNTH_VOLUME);
        listenToParameter(Params::Keys::ID::SYNTH_DIST_DRIVE);
        listenToParameter(Params::Keys::ID::SYNTH_DIST_MIX);
        listenToParameter(Params::Keys::ID::SYNTH_REVERB_WET);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xFF1E1E1E));

        // Subtle separator between global row and per-step row
        int separatorY = globalRowHeight;
        g.setColour(juce::Colours::white.withAlpha(0.07f));
        g.drawHorizontalLine(separatorY, 0.f, (float)getWidth());
    }

    void resized() override {
        auto bounds = getLocalBounds();
        const int w = bounds.getWidth();

        // ---- Row 0: global controls ----
        auto globalRow = bounds.removeFromTop(globalRowHeight);
        globalRow.reduce(8, 4);

        // Title label on the left
        titleLabel.setBounds(globalRow.removeFromLeft(60));

        // 5 global knobs + labels — distribute remaining space
        const int numGlobal = 5;
        const int knobAreaW = globalRow.getWidth() / numGlobal;
        const int knobSize  = juce::jmin(knobAreaW - 4, globalRowHeight - 20);
        const int labelH    = 12;

        for (int i = 0; i < numGlobal; ++i) {
            auto cell = globalRow.removeFromLeft(knobAreaW);
            int knobX = cell.getX() + (cell.getWidth() - knobSize) / 2;
            int knobY = cell.getY();
            globalKnobs[i].setBounds(knobX, knobY, knobSize, knobSize);
            globalLabels_[i].setBounds(knobX, knobY + knobSize - 2, knobSize, labelH);
        }

        // Randomize button (top-right, small)
        randomizeBtn.setBounds(w - 48, 4, 44, 22);

        // ---- Row 1: per-step knobs ----
        auto stepRow = bounds; // remainder
        stepRow.reduce(4, 4);

        const int numStep = 7;
        const int cellW   = stepRow.getWidth() / numStep;
        const int knobH   = stepRow.getHeight() - 14;  // leave room for label
        const int kSize   = juce::jmin(cellW - 6, knobH);
        const int lH      = 12;

        for (int i = 0; i < numStep; ++i) {
            auto cell = stepRow.removeFromLeft(cellW);
            int kx = cell.getX() + (cell.getWidth() - kSize) / 2;
            int ky = cell.getY() + (cell.getHeight() - kSize - lH) / 2;
            perStepKnobs[i].setBounds(kx, ky, kSize, kSize);
            perStepLabels_[i].setBounds(kx - 4, ky + kSize, kSize + 8, lH);
        }
    }

protected:
    void onStateChanged() override {
        updateFromSequence();
    }

private:
    void updateFromSequence() {
        auto& state = getState();
        int selectedStep = state.getSelectedStepForEditing();
        if (selectedStep < 0) return;

        // Update per-step knobs from selected step's mod values
        perStepKnobs[0].setValue(state.getStep303Cutoff    (selectedStep), juce::dontSendNotification);
        perStepKnobs[1].setValue(state.getStep303Resonance (selectedStep), juce::dontSendNotification);
        perStepKnobs[2].setValue(state.getStep303EnvMod    (selectedStep), juce::dontSendNotification);
        perStepKnobs[3].setValue(state.getStep303Decay     (selectedStep), juce::dontSendNotification);
        perStepKnobs[4].setValue(state.getStep303Accent    (selectedStep), juce::dontSendNotification);
        perStepKnobs[5].setValue(state.getStep303Waveform  (selectedStep), juce::dontSendNotification);
        perStepKnobs[6].setValue(state.getStep303Slew      (selectedStep), juce::dontSendNotification);
        // Global knobs are handled by APVTS attachment automatically
    }

    void randomizePerStepParams() {
        // Musical sweet-spot ranges per param
        static const int mins[] = { 25,  0, 15, 15,  0,   0,  0 };
        static const int maxs[] = { 90, 80,100, 85, 55, 127, 35 };

        thread_local std::mt19937 rng(std::random_device{}());

        auto& state = getState();
        int selectedStep = state.getSelectedStepForEditing();
        if (selectedStep < 0) return;

        for (int i = 0; i < 7; ++i) {
            // Waveform (slot 5): snap to 0 or 127
            int val;
            if (i == 5) {
                val = (std::uniform_int_distribution<int>(0, 1)(rng) == 0) ? 0 : 127;
            } else {
                val = std::uniform_int_distribution<int>(mins[i], maxs[i])(rng);
            }
            state.setStepModValue(selectedStep, i, val);
        }
    }

    // Layout constant
    static constexpr int globalRowHeight = 52;

    juce::Typeface::Ptr interFont;

    juce::Label titleLabel;

    // Per-step knobs (7 slots = cutoff, res, envmod, decay, accent, wave, slew)
    std::array<juce::Slider, 7> perStepKnobs;
    std::array<juce::Label,  7> perStepLabels_;

    // Global knobs (5: tune, vol, drive, distmix, reverb)
    std::array<juce::Slider, 5> globalKnobs;
    std::array<juce::Label,  5> globalLabels_;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 5> globalAttachments;

    juce::TextButton randomizeBtn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TB303ControlsComponent)
};
