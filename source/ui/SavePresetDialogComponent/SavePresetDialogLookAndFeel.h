#pragma once

#include "SavePresetDialogConstants.h"
#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * SavePresetDialogLookAndFeel
 *
 * Custom look and feel for the save preset dialog components.
 */
class SavePresetDialogLookAndFeel : public juce::LookAndFeel_V4 {
public:
    SavePresetDialogLookAndFeel() {
        interFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
        interBoldFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Inter_28ptBold_ttf,
            BinaryData::Inter_28ptBold_ttfSize);
    }

    void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                               juce::TextEditor& editor) override {
        juce::ignoreUnused(editor);
        g.setColour(juce::Colours::white);
        g.drawRect(0, 0, width, height, SavePresetDialogConstants::borderWidth);
    }

    void fillTextEditorBackground(juce::Graphics& g, int /*width*/, int /*height*/,
                                  juce::TextEditor& editor) override {
        juce::ignoreUnused(editor);
        g.fillAll(juce::Colours::transparentBlack);
    }

    juce::CaretComponent* createCaretComponent(juce::Component* keyFocusOwner) override {
        auto* caret = new juce::CaretComponent(keyFocusOwner);
        caret->setColour(juce::CaretComponent::caretColourId, juce::Colours::white);
        return caret;
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& /*backgroundColourToUse*/,
                              bool isMouseOverButton, bool isButtonDown) override {
        auto bounds = button.getLocalBounds().toFloat();

        if (isButtonDown) {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.fillRect(bounds);
        } else if (isMouseOverButton) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRect(bounds);
        }

        g.setColour(juce::Colours::white);
        g.drawRect(bounds, static_cast<float>(SavePresetDialogConstants::borderWidth));
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool /*isMouseOverButton*/, bool /*isButtonDown*/) override {
        juce::Font font(interBoldFont);
        font.setHeight(SavePresetDialogConstants::buttonFontSize);
        g.setFont(font);
        g.setColour(juce::Colours::white);

        g.drawText(button.getButtonText(), button.getLocalBounds(),
                   juce::Justification::centred, true);
    }

    juce::Typeface::Ptr interFont;
    juce::Typeface::Ptr interBoldFont;
};
