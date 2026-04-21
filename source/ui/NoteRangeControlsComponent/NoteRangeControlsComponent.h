#pragma once

#include "NoteRangeControlsConstants.h"
#include "NoteRangeSliderLookAndFeel.h"
#include "NoteRangeGauge.h"
#include "MidiLearnOverlay.h"
#include "BinaryData.h"
#include "../MidiLearnOverlayPainter.h"
#include "../StateListenerComponent.h"
#include "../../backend/MidiMapping/MidiMapping.h"
#include "../../backend/Scale.h"
#include "../../data/Params.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * NoteRangeControlsComponent
 *
 * Panel displayed to the right of the sequencer steps/controls.
 * Contains controls for note range settings using a window-based approach:
 * - Window Size (bottom slider): How many notes are in the valid range (1-128)
 * - Window Offset (top slider): Starting position of the window (0-127)
 *
 * Example: size=45, offset=54 → valid notes are [54, 98].
 *
 * Listens to the scale window APVTS params and the global scale notes, and
 * updates the gauge on changes.
 */
class NoteRangeControlsComponent : public StateListenerComponent {
public:
    explicit NoteRangeControlsComponent(PluginProcessor& processor)
        : StateListenerComponent(processor) {
        interBoldFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Inter_24ptBold_ttf,
            BinaryData::Inter_24ptBold_ttfSize);

        setupSliders();
        setupAttachments();
        setupGauge();
        setupLearnOverlays();

        listenToScaleRange();
        listenToGlobalScaleNotes();
    }

    ~NoteRangeControlsComponent() override {
        windowWidthSlider.setLookAndFeel(nullptr);
        windowOffsetSlider.setLookAndFeel(nullptr);
    }

    void setWindow(int offset, int size) {
        rangeGauge.setRawParams(offset, size);
        int minNote = Scale::computeMinFromWindow(offset, size);
        int maxNote = Scale::computeMaxFromWindow(offset, size);
        rangeGauge.setRange(minNote, maxNote);
    }

    void setActiveNote(int noteNumber) { rangeGauge.setActiveNote(noteNumber); }
    void clearActiveNote() { rangeGauge.clearActiveNote(); }

    void setRegistry(MidiLearnRegistry* registry) { registry_ = registry; }

    void setMidiLearnSelectedCallback(std::function<void()> callback) {
        onMidiLearnSelected = std::move(callback);
    }

    std::function<void()> onMidiLearnSelected;
    std::function<void()> onRangeChanged;

    void paint(juce::Graphics& g) override {
        using namespace NoteRangeControlsConstants;

        bool isLearning = registry_ && registry_->isLearning();
        offsetOverlay_.setVisible(isLearning);
        widthOverlay_.setVisible(isLearning);

        paintBorder(g);
        if (isLearning) paintLearnOverlays(g);
        paintLabel(g);
    }

    void resized() override {
        using namespace NoteRangeControlsConstants;

        int widthSliderX = sliderPadding + contentPaddingLeft;
        int widthSliderY = getHeight() - sliderPadding - sliderDiameter;
        windowWidthSlider.setBounds(widthSliderX, widthSliderY, sliderDiameter, sliderDiameter);

        int offsetSliderX = sliderPadding + contentPaddingLeft;
        int offsetSliderY = widthSliderY - sliderVerticalSpacing - sliderDiameter - extraSliderGap;
        windowOffsetSlider.setBounds(offsetSliderX, offsetSliderY, sliderDiameter, sliderDiameter);

        offsetOverlay_.setBounds(offsetSliderX, offsetSliderY, sliderDiameter, sliderDiameter);
        widthOverlay_.setBounds(widthSliderX, widthSliderY, sliderDiameter, sliderDiameter);

        int halfWidth = getWidth() / 2;
        int gaugeX = halfWidth + rangeBarHorizontalMargin + rangeBarXOffset;
        int gaugeY = rangeBarVerticalMargin;
        int gaugeWidth = halfWidth - (rangeBarHorizontalMargin * 2);
        int gaugeHeight = getHeight() - (rangeBarVerticalMargin * 2);
        rangeGauge.setBounds(gaugeX, gaugeY, gaugeWidth, gaugeHeight);
    }

protected:
    void onStateChanged() override {
        auto& state = getState();
        int offset = state.getScaleWindowOffset();
        int size = state.getScaleWindowSize();

        rangeGauge.setRawParams(offset, size);

        int minNote = Scale::computeMinFromWindow(offset, size);
        int maxNote = Scale::computeMaxFromWindow(offset, size);
        rangeGauge.setRange(minNote, maxNote);

        std::array<bool, 12> scaleNotes{};
        for (int i = 0; i < 12; ++i)
            scaleNotes[i] = state.getGlobalScaleNote(i);
        rangeGauge.setScaleNotes(scaleNotes);

        rangeGauge.repaint();

        if (onRangeChanged) onRangeChanged();
    }

private:
    void setupSliders() {
        windowWidthSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        windowWidthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        windowWidthSlider.setLookAndFeel(&sliderLookAndFeel);
        addAndMakeVisible(windowWidthSlider);

        windowOffsetSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        windowOffsetSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        windowOffsetSlider.setLookAndFeel(&sliderLookAndFeel);
        addAndMakeVisible(windowOffsetSlider);
    }

    void setupAttachments() {
        windowOffsetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            getProcessor().getAPVTS(), Params::Keys::ID::SCALE_WINDOW_OFFSET, windowOffsetSlider);
        windowWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            getProcessor().getAPVTS(), Params::Keys::ID::SCALE_WINDOW_SIZE, windowWidthSlider);
    }

    void setupGauge() {
        addAndMakeVisible(rangeGauge);
        rangeGauge.onOffsetDragged = [this](int newRawOffset) {
            if (auto* param = getProcessor().getAPVTS().getParameter(Params::Keys::ID::SCALE_WINDOW_OFFSET))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(newRawOffset)));
        };
    }

    void setupLearnOverlays() {
        addAndMakeVisible(offsetOverlay_);
        offsetOverlay_.setVisible(false);
        offsetOverlay_.setInterceptsMouseClicks(true, false);
        offsetOverlay_.onMouseDown = [this]() {
            if (registry_ && registry_->isLearning()) {
                registry_->setLearningControl(MidiControlId::NOTE_RANGE_MIN);
                if (onMidiLearnSelected) onMidiLearnSelected();
            }
        };

        addAndMakeVisible(widthOverlay_);
        widthOverlay_.setVisible(false);
        widthOverlay_.setInterceptsMouseClicks(true, false);
        widthOverlay_.onMouseDown = [this]() {
            if (registry_ && registry_->isLearning()) {
                registry_->setLearningControl(MidiControlId::NOTE_RANGE_MAX);
                if (onMidiLearnSelected) onMidiLearnSelected();
            }
        };
    }

    void paintBorder(juce::Graphics& g) {
        using namespace NoteRangeControlsConstants;
        g.setColour(juce::Colours::white);
        float inset = borderWidth * 0.5f;
        juce::Rectangle<float> borderBounds = getLocalBounds().toFloat().reduced(inset);
        g.drawRoundedRectangle(borderBounds, borderRadius, borderWidth);
    }

    void paintLearnOverlays(juce::Graphics& g) {
        MidiLearnOverlayPainter::paint(g, windowOffsetSlider.getBounds(),
                                       registry_, MidiControlId::NOTE_RANGE_MIN, 0.0f);
        MidiLearnOverlayPainter::paint(g, windowWidthSlider.getBounds(),
                                       registry_, MidiControlId::NOTE_RANGE_MAX, 0.0f);
    }

    void paintLabel(juce::Graphics& g) {
        using namespace NoteRangeControlsConstants;

        float labelAreaTop = static_cast<float>(sliderPadding);
        float labelAreaBottom = static_cast<float>(getHeight() - sliderPadding - sliderDiameter
                                  - sliderVerticalSpacing - sliderDiameter - labelGapFromSlider);
        float labelAreaHeight = labelAreaBottom - labelAreaTop;
        float labelAreaCentreX = static_cast<float>(sliderPadding + contentPaddingLeft) + sliderDiameter / 2.0f;
        float labelAreaCentreY = labelAreaTop + labelAreaHeight / 2.0f;

        g.setColour(juce::Colours::white);

        juce::Font labelFont;
        if (interBoldFont != nullptr)
            labelFont = juce::Font(interBoldFont);
        labelFont.setHeight(labelFontSize);
        g.setFont(labelFont);

        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi,
                                                       labelAreaCentreX,
                                                       labelAreaCentreY));

        juce::Rectangle<float> textBounds(labelAreaCentreX - labelAreaHeight / 2.0f,
                                          labelAreaCentreY - labelFontSize / 2.0f,
                                          labelAreaHeight,
                                          labelFontSize);
        g.drawText("Note Range", textBounds, juce::Justification::centred, false);
        g.restoreState();
    }

    juce::Typeface::Ptr interBoldFont;
    NoteRangeSliderLookAndFeel sliderLookAndFeel;
    juce::Slider windowWidthSlider;
    juce::Slider windowOffsetSlider;
    NoteRangeGauge rangeGauge;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> windowOffsetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> windowWidthAttachment;

    MidiLearnRegistry* registry_ = nullptr;
    MidiLearnOverlay offsetOverlay_;
    MidiLearnOverlay widthOverlay_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteRangeControlsComponent)
};
