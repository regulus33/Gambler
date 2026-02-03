#pragma once

#include "BinaryData.h"
#include "Icons.h"
#include "PresetDropdownComponent.h"
#include "SavePresetDialogComponent.h"
#include "../backend/PresetManager.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace PresetSelectorConstants {
    constexpr int height = 31;  // 24 * 1.3
    constexpr int dropdownWidth = 182;  // 140 * 1.3
    constexpr int saveButtonSize = 31;  // 24 * 1.3
    constexpr int gapBetweenDropdownAndButton = 10;  // 8 * 1.3
    constexpr int saveIconSize = 17;  // 13 * 1.3
}

/**
 * SavePresetButton
 * 
 * A simple icon button that displays the save icon
 */
class SavePresetButton : public juce::Component {
public:
    SavePresetButton() {
        saveIcon = Icons::createSaveIcon();
    }

    void paint(juce::Graphics& g) override {
        using namespace PresetSelectorConstants;
        using namespace PresetDropdownConstants;

        // Draw border with rounded corners
        g.setColour(borderColor);
        g.drawRoundedRectangle(getLocalBounds().toFloat(), borderRadius, borderWidth);

        // Draw save icon centered
        auto iconBounds = getLocalBounds().toFloat().reduced(
            (getWidth() - saveIconSize) / 2.0f,
            (getHeight() - saveIconSize) / 2.0f);

        Icons::drawIcon(g, saveIcon, iconBounds, juce::Colours::white);
    }

    void mouseDown(const juce::MouseEvent& /*event*/) override {
        if (onClick) {
            onClick();
        }
    }

    void mouseEnter(const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    std::function<void()> onClick;

private:
    juce::Path saveIcon;
};

/**
 * PresetSelectorComponent
 * 
 * A component that combines:
 * - Preset dropdown selector (left)
 * - Save button (right)
 * 
 * Handles showing the save dialog as a popup overlay.
 */
class PresetSelectorComponent : public juce::Component {
public:
    PresetSelectorComponent() {
        addAndMakeVisible(dropdown);
        addAndMakeVisible(saveButton);

        // Wire up dropdown selection
        dropdown.onPresetSelected = [this](int index) {
            if (onPresetSelected) {
                onPresetSelected(index);
            }
        };

        // Wire up dropdown delete
        dropdown.onPresetDeleteRequested = [this](int index) {
            if (onDeletePreset) {
                onDeletePreset(index);
            }
        };

        // Wire up save button
        saveButton.onClick = [this]() {
            showSaveDialog();
        };
    }

    void resized() override {
        using namespace PresetSelectorConstants;
        
        auto area = getLocalBounds();
        
        // Save button on the right
        saveButton.setBounds(area.removeFromRight(saveButtonSize));
        
        // Gap
        area.removeFromRight(gapBetweenDropdownAndButton);
        
        // Dropdown takes remaining space (or fixed width if area is larger)
        int dropdownW = juce::jmin(area.getWidth(), dropdownWidth);
        dropdown.setBounds(area.removeFromLeft(dropdownW));
    }

    /**
     * Get the calculated width of the component
     */
    int getCalculatedWidth() const {
        using namespace PresetSelectorConstants;
        return dropdownWidth + gapBetweenDropdownAndButton + saveButtonSize;
    }

    /**
     * Get the calculated height of the component
     */
    int getCalculatedHeight() const {
        return PresetSelectorConstants::height;
    }

    /**
     * Update the list of presets
     */
    void setPresets(const juce::StringArray& presetNames) {
        dropdown.setPresets(presetNames);
    }

    /**
     * Select a preset by index
     */
    void selectPresetByIndex(int index, bool sendNotification = false) {
        dropdown.selectPresetByIndex(index, sendNotification);
    }

    /**
     * Select a preset by name
     */
    void selectPresetByName(const juce::String& name, bool sendNotification = false) {
        dropdown.selectPresetByName(name, sendNotification);
    }

    /**
     * Clear the selection
     */
    void clearSelection() {
        dropdown.clearSelection();
    }

    /**
     * Callback when a preset is selected from the dropdown
     * Parameter: 0-based index of the selected preset
     */
    std::function<void(int)> onPresetSelected;

    /**
     * Callback when save is requested
     * Parameter: the preset name to save
     */
    std::function<void(const juce::String&)> onSavePreset;

    /**
     * Callback when delete is requested
     * Parameter: 0-based index of the preset to delete
     */
    std::function<void(int)> onDeletePreset;

    /**
     * Set the parent component for showing the save dialog
     * This should be the top-level component (e.g., ApplicationComponent)
     */
    void setDialogParent(juce::Component* parent) {
        dialogParent = parent;
    }

private:
    void showSaveDialog() {
        // Find the top-level component to add the dialog to
        juce::Component* parent = dialogParent;
        if (!parent) {
            parent = getTopLevelComponent();
        }
        
        if (!parent) {
            DBG("PresetSelectorComponent: No parent found for save dialog");
            return;
        }
        
        // Create dialog if needed
        if (!saveDialog) {
            saveDialog = std::make_unique<SavePresetDialogComponent>();
            
            saveDialog->onSave = [this, parent](const juce::String& name) {
                if (onSavePreset) {
                    onSavePreset(name);
                }
                hideSaveDialog();
            };
            
            saveDialog->onCancel = [this]() {
                hideSaveDialog();
            };
        }
        
        // Clear and focus input
        saveDialog->clearInput();
        
        // Add to parent and center
        parent->addAndMakeVisible(*saveDialog);
        
        // Center the dialog in the parent
        int dialogX = (parent->getWidth() - saveDialog->getWidth()) / 2;
        int dialogY = (parent->getHeight() - saveDialog->getHeight()) / 2;
        saveDialog->setBounds(dialogX, dialogY, saveDialog->getWidth(), saveDialog->getHeight());
        
        // Focus the input field
        saveDialog->focusInput();
        
        // Bring to front
        saveDialog->toFront(true);
    }

    void hideSaveDialog() {
        if (saveDialog) {
            if (auto* parent = saveDialog->getParentComponent()) {
                parent->removeChildComponent(saveDialog.get());
            }
        }
    }

    PresetDropdownComponent dropdown;
    SavePresetButton saveButton;
    std::unique_ptr<SavePresetDialogComponent> saveDialog;
    juce::Component* dialogParent = nullptr;
};

