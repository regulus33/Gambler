#pragma once

#include "BinaryData.h"
#include "../backend/PresetManager.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace SavePresetDialogConstants {
    constexpr int dialogWidth = 300;
    constexpr int dialogHeight = 150;
    constexpr int padding = 20;
    constexpr int titleHeight = 24;
    constexpr int inputHeight = 30;
    constexpr int buttonHeight = 28;
    constexpr int buttonWidth = 80;
    constexpr int buttonSpacing = 12;
    constexpr int elementSpacing = 16;
    constexpr float titleFontSize = 14.0f;
    constexpr float inputFontSize = 12.0f;
    constexpr float buttonFontSize = 12.0f;
    constexpr juce::uint32 bgColourHex = 0xFF1E1E1E;
    constexpr int borderWidth = 1;
}

/**
 * SavePresetDialogLookAndFeel
 * 
 * Custom look and feel for the save preset dialog components
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
                                juce::TextEditor& editor) override
    {
        juce::ignoreUnused(editor);
        g.setColour(juce::Colours::white);
        g.drawRect(0, 0, width, height, SavePresetDialogConstants::borderWidth);
    }

    void fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                   juce::TextEditor& editor) override
    {
        juce::ignoreUnused(editor);
        // Transparent background
        g.fillAll(juce::Colours::transparentBlack);
    }

    juce::CaretComponent* createCaretComponent(juce::Component* keyFocusOwner) override
    {
        auto* caret = new juce::CaretComponent(keyFocusOwner);
        caret->setColour(juce::CaretComponent::caretColourId, juce::Colours::white);
        return caret;
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                               const juce::Colour& /*backgroundColourToUse*/,
                               bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        
        // Transparent fill with white border
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
                        bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
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

/**
 * PresetNameInputComponent
 * 
 * Text input field for preset name with character limit validation
 */
class PresetNameInputComponent : public juce::TextEditor {
public:
    PresetNameInputComponent() {
        setMultiLine(false);
        setReturnKeyStartsNewLine(false);
        setScrollbarsShown(false);
        setCaretVisible(true);
        setPopupMenuEnabled(false);
        
        // Set text color
        setColour(juce::TextEditor::textColourId, juce::Colours::white);
        setColour(juce::TextEditor::highlightColourId, juce::Colours::white.withAlpha(0.3f));
        setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::white);
        
        // Set font
        auto interFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
        juce::Font font(interFont);
        font.setHeight(SavePresetDialogConstants::inputFontSize);
        setFont(font);
        
        // Set input filter for max length
        setInputFilter(new LengthLimitFilter(PresetManagerConstants::maxPresetNameLength), true);
    }

private:
    /**
     * Input filter that limits text length
     */
    class LengthLimitFilter : public juce::TextEditor::InputFilter {
    public:
        explicit LengthLimitFilter(int maxLen) : maxLength(maxLen) {}
        
        juce::String filterNewText(juce::TextEditor& editor, const juce::String& newInput) override {
            int currentLength = editor.getText().length();
            int selectionLength = editor.getHighlightedRegion().getLength();
            int availableSpace = maxLength - currentLength + selectionLength;
            
            if (availableSpace <= 0) {
                return {};
            }
            
            return newInput.substring(0, availableSpace);
        }
        
    private:
        int maxLength;
    };
};

/**
 * SavePresetDialogComponent
 * 
 * A dialog for saving presets with:
 * - "Save Preset" title in bold Inter 14px
 * - Text input with white border, transparent background
 * - Save and Cancel buttons with white border, transparent fill
 * 
 * This component is designed to be shown as a popup/overlay.
 */
class SavePresetDialogComponent : public juce::Component {
public:
    SavePresetDialogComponent() {
        using namespace SavePresetDialogConstants;
        
        // Load fonts
        interBoldFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Inter_28ptBold_ttf,
            BinaryData::Inter_28ptBold_ttfSize);
        
        // Setup input field
        addAndMakeVisible(nameInput);
        nameInput.setLookAndFeel(&lookAndFeel);
        
        // Setup Save button
        addAndMakeVisible(saveButton);
        saveButton.setButtonText("Save");
        saveButton.setLookAndFeel(&lookAndFeel);
        saveButton.onClick = [this]() {
            if (onSave) {
                juce::String name = nameInput.getText().trim();
                if (!name.isEmpty()) {
                    onSave(name);
                }
            }
        };
        
        // Setup Cancel button
        addAndMakeVisible(cancelButton);
        cancelButton.setButtonText("Cancel");
        cancelButton.setLookAndFeel(&lookAndFeel);
        cancelButton.onClick = [this]() {
            if (onCancel) {
                onCancel();
            }
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
        
        // Background
        g.fillAll(juce::Colour(bgColourHex));
        
        // Border
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRect(getLocalBounds(), borderWidth);
        
        // Title
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
        
        // Input field
        nameInput.setBounds(padding, y, getWidth() - padding * 2, inputHeight);
        y += inputHeight + elementSpacing;
        
        // Buttons (right-aligned)
        int buttonsWidth = buttonWidth * 2 + buttonSpacing;
        int buttonsX = getWidth() - padding - buttonsWidth;
        
        cancelButton.setBounds(buttonsX, y, buttonWidth, buttonHeight);
        saveButton.setBounds(buttonsX + buttonWidth + buttonSpacing, y, buttonWidth, buttonHeight);
    }

    /**
     * Clear the input field
     */
    void clearInput() {
        nameInput.clear();
    }

    /**
     * Set initial text in the input field
     */
    void setInitialText(const juce::String& text) {
        nameInput.setText(text, false);
        nameInput.selectAll();
    }

    /**
     * Focus the input field
     */
    void focusInput() {
        nameInput.grabKeyboardFocus();
    }

    /**
     * Callback when Save is clicked
     * Parameter: the entered preset name
     */
    std::function<void(const juce::String&)> onSave;

    /**
     * Callback when Cancel is clicked
     */
    std::function<void()> onCancel;

private:
    juce::Typeface::Ptr interBoldFont;
    SavePresetDialogLookAndFeel lookAndFeel;
    
    PresetNameInputComponent nameInput;
    juce::TextButton saveButton;
    juce::TextButton cancelButton;
};

