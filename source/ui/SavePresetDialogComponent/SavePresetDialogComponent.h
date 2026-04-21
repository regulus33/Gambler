#pragma once

#include "SavePresetDialogConstants.h"
#include "SavePresetDialogLookAndFeel.h"
#include "PresetNameInputComponent.h"
#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * SavePresetDialogComponent
 *
 * A dialog for saving presets with:
 * - "Save Preset" title in bold Inter 14px
 * - Text input with white border, transparent background
 * - Save and Cancel buttons with white border, transparent fill
 *
 * Designed to be shown as a popup/overlay.
 */
class SavePresetDialogComponent : public juce::Component {
public:
    SavePresetDialogComponent() {
        using namespace SavePresetDialogConstants;

        interBoldFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Inter_28ptBold_ttf,
            BinaryData::Inter_28ptBold_ttfSize);

        addAndMakeVisible(nameInput);
        nameInput.setLookAndFeel(&lookAndFeel);

        addAndMakeVisible(saveButton);
        saveButton.setButtonText("Save");
        saveButton.setLookAndFeel(&lookAndFeel);
        saveButton.onClick = [this]() {
            if (onSave) {
                juce::String name = nameInput.getText().trim();
                if (!name.isEmpty())
                    onSave(name);
            }
        };

        addAndMakeVisible(cancelButton);
        cancelButton.setButtonText("Cancel");
        cancelButton.setLookAndFeel(&lookAndFeel);
        cancelButton.onClick = [this]() {
            if (onCancel) onCancel();
        };

        setSize(dialogWidth, dialogHeight);
    }

    ~SavePresetDialogComponent() override {
        nameInput.setLookAndFeel(nullptr);
        saveButton.setLookAndFeel(nullptr);
        cancelButton.setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override {
        using namespace SavePresetDialogConstants;

        g.fillAll(juce::Colour(bgColourHex));

        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRect(getLocalBounds(), borderWidth);

        juce::Font titleFont(interBoldFont);
        titleFont.setHeight(titleFontSize);
        g.setFont(titleFont);
        g.setColour(juce::Colours::white);

        auto titleBounds = juce::Rectangle<int>(padding, padding, getWidth() - padding * 2, titleHeight);
        g.drawText("Save Preset", titleBounds, juce::Justification::centredLeft, true);
    }

    void resized() override {
        using namespace SavePresetDialogConstants;

        int y = padding + titleHeight + elementSpacing;

        nameInput.setBounds(padding, y, getWidth() - padding * 2, inputHeight);
        y += inputHeight + elementSpacing;

        int buttonsWidth = buttonWidth * 2 + buttonSpacing;
        int buttonsX = getWidth() - padding - buttonsWidth;

        cancelButton.setBounds(buttonsX, y, buttonWidth, buttonHeight);
        saveButton.setBounds(buttonsX + buttonWidth + buttonSpacing, y, buttonWidth, buttonHeight);
    }

    void clearInput() { nameInput.clear(); }

    void setInitialText(const juce::String& text) {
        nameInput.setText(text, false);
        nameInput.selectAll();
    }

    void focusInput() { nameInput.grabKeyboardFocus(); }

    std::function<void(const juce::String&)> onSave;
    std::function<void()> onCancel;

private:
    juce::Typeface::Ptr interBoldFont;
    SavePresetDialogLookAndFeel lookAndFeel;

    PresetNameInputComponent nameInput;
    juce::TextButton saveButton;
    juce::TextButton cancelButton;
};
