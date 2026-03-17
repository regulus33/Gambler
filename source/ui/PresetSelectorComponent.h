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
    constexpr int cycleButtonSize = 20;   // Width and height of the cycle arrow hit area
    constexpr float cycleIconScale = 0.75f; // Scale factor applied to the index arrow icon
    constexpr int cycleButtonGap = 6;     // Gap between cycle buttons and the dropdown
}

/**
 * PresetCycleButton
 *
 * A small icon button that draws a scaled-down index left or right arrow icon.
 * Used to cycle through presets inside PresetSelectorComponent.
 */
class PresetCycleButton : public juce::Component {
public:
    enum class Direction { Left, Right };

    explicit PresetCycleButton(Direction dir) : direction(dir) {
        icon = (direction == Direction::Left)
            ? Icons::createIndexLeftIcon()
            : Icons::createIndexRightIcon();
    }

    void paint(juce::Graphics& g) override {
        using namespace PresetSelectorConstants;
        Icons::drawIcon(g, icon, getLocalBounds().toFloat(),
                        juce::Colours::white, true, cycleIconScale);
    }

    void mouseEnter(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (onClick) onClick();
    }

    std::function<void()> onClick;

private:
    Direction direction;
    juce::Path icon;
};

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
 * - Left cycle button (◀) to go to the previous preset
 * - Preset dropdown selector
 * - Right cycle button (▶) to go to the next preset
 * - Save button (rightmost)
 *
 * Handles showing the save dialog as a popup overlay.
 */
class PresetSelectorComponent : public juce::Component {
public:
    PresetSelectorComponent() {
        addAndMakeVisible(cycleDownButton);
        addAndMakeVisible(dropdown);
        addAndMakeVisible(cycleUpButton);
        addAndMakeVisible(saveButton);

        // Wire up cycle buttons
        cycleDownButton.onClick = [this]() {
            if (onCycleDown) onCycleDown();
        };
        cycleUpButton.onClick = [this]() {
            if (onCycleUp) onCycleUp();
        };

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

        // Left cycle button
        cycleDownButton.setBounds(area.removeFromLeft(cycleButtonSize));
        area.removeFromLeft(cycleButtonGap);

        // Save button on the right
        saveButton.setBounds(area.removeFromRight(saveButtonSize));
        area.removeFromRight(gapBetweenDropdownAndButton);

        // Right cycle button (between dropdown and save button)
        cycleUpButton.setBounds(area.removeFromRight(cycleButtonSize));
        area.removeFromRight(cycleButtonGap);

        // Dropdown takes remaining space (or fixed width if area is larger)
        int dropdownW = juce::jmin(area.getWidth(), dropdownWidth);
        dropdown.setBounds(area.removeFromLeft(dropdownW));
    }

    /**
     * Get the calculated width of the component
     */
    int getCalculatedWidth() const {
        using namespace PresetSelectorConstants;
        return cycleButtonSize + cycleButtonGap
             + dropdownWidth
             + cycleButtonGap + cycleButtonSize
             + gapBetweenDropdownAndButton + saveButtonSize;
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
     * Callback to cycle to the previous preset (left arrow)
     */
    std::function<void()> onCycleDown;

    /**
     * Callback to cycle to the next preset (right arrow)
     */
    std::function<void()> onCycleUp;

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

    PresetCycleButton cycleDownButton { PresetCycleButton::Direction::Left };
    PresetDropdownComponent dropdown;
    PresetCycleButton cycleUpButton { PresetCycleButton::Direction::Right };
    SavePresetButton saveButton;
    std::unique_ptr<SavePresetDialogComponent> saveDialog;
    juce::Component* dialogParent = nullptr;
};

