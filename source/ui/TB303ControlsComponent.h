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
 * Two tabs:
 *   STEP tab: 7 per-step knobs (Cutoff, Res, EnvMod, Decay, Accent, Wave, Slew)
 *   GLOBAL tab: 5 global knobs (Tune, Vol, Drive, DistMix, Reverb)
 *
 * Per-step params are stored in stepModValue[0..6] via SequenceState.
 * Global params write directly to APVTS float parameters.
 *
 * Roll behaviour: when the Roll button is held, each step tick randomises that
 * step's per-step 303 params (handled in PluginProcessor::processBlock).
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

        // ---- Tab buttons ----
        tabStepBtn.setButtonText("STEP");
        tabGlobalBtn.setButtonText("GLOBAL");

        for (auto* btn : { &tabStepBtn, &tabGlobalBtn }) {
            btn->setClickingTogglesState(false);
            btn->setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.55f));
            addAndMakeVisible(*btn);
        }

        tabStepBtn  .onClick = [this]() { showTab(0); };
        tabGlobalBtn.onClick = [this]() { showTab(1); };

        // ---- Per-step knobs (7 slots: cutoff, res, envmod, decay, accent, wave, slew) ----
        static const char* perStepLabels[] = { "CUTOFF", "RESON", "ENVMOD", "DECAY", "ACCENT", "WAVE", "SLEW" };
        static const juce::uint32 slotColours[] = {
            0xFFFF6B6B,  // red
            0xFFFF9F6B,  // orange
            0xFFFFD66B,  // yellow
            0xFFFFE66B,  // light yellow
            0xFFE6FF6B,  // yellow-green
            0xFF6BFFB4,  // light green
            0xFF6BFFD6   // cyan-green
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

        // ---- Global knobs (5: tune, vol, drive, distmix, reverb) ----
        static const char* globalLabels[]   = { "TUNE", "VOL", "DRIVE", "DISTMIX", "REVERB" };
        static const char* globalParamIds[] = {
            "synth_tuning", "synth_volume", "synth_dist_drive", "synth_dist_mix", "synth_reverb_wet"
        };
        static const double globalMins[] = { -12.0, 0.0, 0.0, 0.0, 0.0 };
        static const double globalMaxs[] = {  12.0, 1.0, 1.0, 1.0, 1.0 };
        static const double globalDefs[] = {   0.0, 0.75, 0.0, 0.0, 0.0 };

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
            // Start hidden — shown when switching to GLOBAL tab
            addChildComponent(knob);

            auto& lbl = globalLabels_[i];
            lbl.setText(globalLabels[i], juce::dontSendNotification);
            lbl.setFont(juce::FontOptions(interFont).withHeight(9.f));
            lbl.setColour(juce::Label::textColourId, juce::Colour(0xFF6BD6FF).withAlpha(0.85f));
            lbl.setJustificationType(juce::Justification::centred);
            addChildComponent(lbl);

            globalAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                getProcessor().getAPVTS(), globalParamIds[i], knob);
        }

        // Subscribe to step mod values (all steps) + selected step changes + global params
        listenToModulations(0, 63);
        listenToParameter(Params::Keys::ID::SEQ_SELECTED_STEP);
        listenToParameter(Params::Keys::ID::SYNTH_TUNING);
        listenToParameter(Params::Keys::ID::SYNTH_VOLUME);
        listenToParameter(Params::Keys::ID::SYNTH_DIST_DRIVE);
        listenToParameter(Params::Keys::ID::SYNTH_DIST_MIX);
        listenToParameter(Params::Keys::ID::SYNTH_REVERB_WET);

        // Initialise tab colours (start on STEP tab)
        updateTabColours();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xFF1E1E1E));

        // Tab strip background
        g.setColour(juce::Colour(0xFF252525));
        g.fillRect(0, 0, getWidth(), tabStripH);

        // Active tab underline
        g.setColour(juce::Colour(0xFF6BD6FF));
        auto& activeBtn = (activeTab == 0) ? tabStepBtn : tabGlobalBtn;
        auto ab = activeBtn.getBounds();
        g.fillRect(ab.getX(), ab.getBottom() - 2, ab.getWidth(), 2);

        // Separator between strip and content
        g.setColour(juce::Colours::white.withAlpha(0.07f));
        g.drawHorizontalLine(tabStripH, 0.f, (float)getWidth());
    }

    void resized() override {
        auto bounds = getLocalBounds();
        if (bounds.isEmpty()) return;

        // Tab strip
        auto tabStrip = bounds.removeFromTop(tabStripH);
        int tabY = tabStrip.getY() + (tabStrip.getHeight() - tabBtnH) / 2;
        tabStepBtn  .setBounds(8,                  tabY, tabBtnW, tabBtnH);
        tabGlobalBtn.setBounds(8 + tabBtnW + 4,    tabY, tabBtnW, tabBtnH);

        // Content area
        auto content = bounds.reduced(4, 4);

        if (activeTab == 0) {
            // Seven per-step knobs spread across full width
            const int n       = 7;
            const int cellW   = content.getWidth() / n;
            const int labelH  = 12;
            const int kSize   = juce::jmin(cellW - 6, content.getHeight() - labelH - 4);
            auto row = content;
            for (int i = 0; i < n; ++i) {
                auto cell = row.removeFromLeft(cellW);
                int kx = cell.getX() + (cell.getWidth() - kSize) / 2;
                int ky = cell.getY() + (cell.getHeight() - kSize - labelH) / 2;
                perStepKnobs[i] .setBounds(kx,     ky,          kSize,       kSize);
                perStepLabels_[i].setBounds(kx - 4, ky + kSize,  kSize + 8,   labelH);
            }
        } else {
            // Five global knobs spread across full width
            const int n       = 5;
            const int cellW   = content.getWidth() / n;
            const int labelH  = 12;
            const int kSize   = juce::jmin(cellW - 6, content.getHeight() - labelH - 4);
            auto row = content;
            for (int i = 0; i < n; ++i) {
                auto cell = row.removeFromLeft(cellW);
                int kx = cell.getX() + (cell.getWidth() - kSize) / 2;
                int ky = cell.getY() + (cell.getHeight() - kSize - labelH) / 2;
                globalKnobs[i]  .setBounds(kx,     ky,          kSize,       kSize);
                globalLabels_[i].setBounds(kx - 4, ky + kSize,  kSize + 8,   labelH);
            }
        }
    }

protected:
    void onStateChanged() override {
        updateFromSequence();
    }

private:
    // Switch the visible tab
    void showTab(int tabIndex) {
        activeTab = tabIndex;
        const bool showStep = (tabIndex == 0);

        for (auto& k : perStepKnobs)  k.setVisible(showStep);
        for (auto& l : perStepLabels_) l.setVisible(showStep);
        for (auto& k : globalKnobs)   k.setVisible(!showStep);
        for (auto& l : globalLabels_) l.setVisible(!showStep);

        updateTabColours();
        resized();
        repaint();
    }

    void updateTabColours() {
        tabStepBtn.setColour(juce::TextButton::buttonColourId,
            activeTab == 0 ? juce::Colour(0xFF383838) : juce::Colour(0xFF252525));
        tabGlobalBtn.setColour(juce::TextButton::buttonColourId,
            activeTab == 1 ? juce::Colour(0xFF383838) : juce::Colour(0xFF252525));
    }

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
        // Global knobs are kept in sync by APVTS attachment automatically
    }

    // Layout constants
    static constexpr int tabStripH = 28;
    static constexpr int tabBtnW   = 60;
    static constexpr int tabBtnH   = 20;

    int activeTab = 0;

    juce::Typeface::Ptr interFont;

    juce::TextButton tabStepBtn;
    juce::TextButton tabGlobalBtn;

    // Per-step knobs (7 slots: cutoff, res, envmod, decay, accent, wave, slew)
    std::array<juce::Slider, 7> perStepKnobs;
    std::array<juce::Label,  7> perStepLabels_;

    // Global knobs (5: tune, vol, drive, distmix, reverb)
    std::array<juce::Slider, 5> globalKnobs;
    std::array<juce::Label,  5> globalLabels_;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 5> globalAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TB303ControlsComponent)
};
