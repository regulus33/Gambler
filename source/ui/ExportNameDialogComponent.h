#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * ExportNameDialogComponent
 *
 * A simple dialog shown in a CallOutBox that lets the user name their
 * export file before sharing. Cancel dismisses without exporting.
 */
class ExportNameDialogComponent : public juce::Component {
public:
    explicit ExportNameDialogComponent(std::function<void(const juce::String&)> onSave)
        : saveCallback_(std::move(onSave))
    {
        nameEditor_.setText("Gambler-Export", false);
        nameEditor_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF333333));
        nameEditor_.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        nameEditor_.setColour(juce::TextEditor::outlineColourId, juce::Colours::grey);
        nameEditor_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::white);
        nameEditor_.setJustification(juce::Justification::centredLeft);
        nameEditor_.setSelectAllWhenFocused(true);
        nameEditor_.onReturnKey = [this]() { doSave(); };
        addAndMakeVisible(nameEditor_);

        extensionLabel_.setText(".mid", juce::dontSendNotification);
        extensionLabel_.setColour(juce::Label::textColourId, juce::Colours::grey);
        extensionLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(extensionLabel_);

        saveButton_.setButtonText("Save");
        saveButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF4488FF));
        saveButton_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        saveButton_.onClick = [this]() { doSave(); };
        addAndMakeVisible(saveButton_);

        cancelButton_.setButtonText("Cancel");
        cancelButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF444444));
        cancelButton_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        cancelButton_.onClick = [this]() {
            if (auto* callout = findParentComponentOfClass<juce::CallOutBox>())
                callout->dismiss();
        };
        addAndMakeVisible(cancelButton_);

        setSize(240, 90);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(8);

        auto topRow = area.removeFromTop(30);
        extensionLabel_.setBounds(topRow.removeFromRight(36));
        nameEditor_.setBounds(topRow);

        area.removeFromTop(8);

        auto buttonRow = area.removeFromTop(30);
        int buttonWidth = (buttonRow.getWidth() - 8) / 2;
        cancelButton_.setBounds(buttonRow.removeFromLeft(buttonWidth));
        buttonRow.removeFromLeft(8);
        saveButton_.setBounds(buttonRow);
    }

    void parentHierarchyChanged() override {
        // Auto-focus the text editor when shown
        if (isShowing())
            nameEditor_.grabKeyboardFocus();
    }

private:
    void doSave() {
        auto name = nameEditor_.getText().trim();
        if (name.isEmpty())
            name = "Gambler-Export";

        if (auto* callout = findParentComponentOfClass<juce::CallOutBox>())
            callout->dismiss();

        if (saveCallback_)
            saveCallback_(name);
    }

    juce::TextEditor nameEditor_;
    juce::Label extensionLabel_;
    juce::TextButton saveButton_;
    juce::TextButton cancelButton_;
    std::function<void(const juce::String&)> saveCallback_;
};
