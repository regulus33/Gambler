#pragma once

#include "SavePresetDialogConstants.h"
#include "BinaryData.h"
#include "../../backend/PresetManager.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * PresetNameInputComponent
 *
 * Text input field for preset name with character limit validation.
 */
class PresetNameInputComponent : public juce::TextEditor {
public:
    PresetNameInputComponent() {
        setMultiLine(false);
        setReturnKeyStartsNewLine(false);
        setScrollbarsShown(false);
        setCaretVisible(true);
        setPopupMenuEnabled(false);

        setColour(juce::TextEditor::textColourId, juce::Colours::white);
        setColour(juce::TextEditor::highlightColourId, juce::Colours::white.withAlpha(0.3f));
        setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::white);

        auto interFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
        juce::Font font(interFont);
        font.setHeight(SavePresetDialogConstants::inputFontSize);
        setFont(font);

        setInputFilter(new LengthLimitFilter(PresetManagerConstants::maxPresetNameLength), true);
    }

private:
    class LengthLimitFilter : public juce::TextEditor::InputFilter {
    public:
        explicit LengthLimitFilter(int maxLen) : maxLength(maxLen) {}

        juce::String filterNewText(juce::TextEditor& editor, const juce::String& newInput) override {
            int currentLength = editor.getText().length();
            int selectionLength = editor.getHighlightedRegion().getLength();
            int availableSpace = maxLength - currentLength + selectionLength;

            if (availableSpace <= 0)
                return {};

            return newInput.substring(0, availableSpace);
        }

    private:
        int maxLength;
    };
};
