#pragma once

#include "PresetSelectorConstants.h"
#include "PresetCycleButton.h"
#include "SavePresetButton.h"
#include "../StyledDropdown/StyledDropdown.h"
#include "../SavePresetDialogComponent/SavePresetDialogComponent.h"
#include "../../native/iOSNativeDialogs.h"
#include "../../util/PlatformUtils.h"
#include "../../backend/MidiLearnRegistry/MidiLearnRegistry.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * PresetSelectorComponent
 *
 * Combines:
 * - Left cycle button to go to the previous preset
 * - Preset dropdown selector
 * - Right cycle button to go to the next preset
 * - Save button (rightmost)
 *
 * Handles showing the save dialog as a popup overlay.
 */
class PresetSelectorComponent : public juce::Component {
public:
    PresetSelectorComponent() {
        addChildren();
        configureControls();
        wireCallbacks();
    }

    void resized() override {
        using namespace PresetSelectorConstants;

        auto area = getLocalBounds();

        cycleDownButton.setBounds(area.removeFromLeft(cycleButtonSize));
        area.removeFromLeft(cycleButtonGap);

        saveButton.setBounds(area.removeFromRight(saveButtonSize));
        area.removeFromRight(saveButtonGap);

        cycleUpButton.setBounds(area.removeFromRight(cycleButtonSize));
        area.removeFromRight(cycleButtonGap);

        int dropdownW = juce::jmin(area.getWidth(), dropdownWidth);
        dropdown.setBounds(area.removeFromLeft(dropdownW));
    }

    int getCalculatedWidth() const {
        using namespace PresetSelectorConstants;
        return cycleButtonSize + cycleButtonGap
             + dropdownWidth
             + cycleButtonGap + cycleButtonSize
             + saveButtonGap + saveButtonSize;
    }

    int getCalculatedHeight() const { return PresetSelectorConstants::height; }

    void setPresets(const juce::StringArray& presetNames) { dropdown.setItems(presetNames); }

    void selectPresetByIndex(int index, bool sendNotification = false) {
        dropdown.setSelectedIndex(index, sendNotification);
    }

    void selectPresetByName(const juce::String& name, bool sendNotification = false) {
        dropdown.setSelectedByName(name, sendNotification);
    }

    void clearSelection() { dropdown.clearSelection(); }

    std::function<void()> onCycleDown;
    std::function<void()> onCycleUp;
    std::function<void(int)> onPresetSelected;
    std::function<void(const juce::String&)> onSavePreset;
    std::function<void(int)> onDeletePreset;

    void setRegistry(MidiLearnRegistry* registry) {
        cycleDownButton.setRegistry(registry);
        cycleUpButton.setRegistry(registry);
    }

    void setDialogParent(juce::Component* parent) { dialogParent = parent; }

private:
    void addChildren() {
        addAndMakeVisible(cycleDownButton);
        addAndMakeVisible(dropdown);
        addAndMakeVisible(cycleUpButton);
        addAndMakeVisible(saveButton);
    }

    void configureControls() {
        cycleDownButton.setMidiControlId(MidiControlId::PRESET_CYCLE_DOWN);
        cycleUpButton.setMidiControlId(MidiControlId::PRESET_CYCLE_UP);

        dropdown.setShowDeleteButtons(true);
        dropdown.setPlaceholderText("Select Preset");
    }

    void wireCallbacks() {
        cycleDownButton.onClick = [this]() { if (onCycleDown) onCycleDown(); };
        cycleUpButton.onClick = [this]() { if (onCycleUp) onCycleUp(); };

        dropdown.onItemSelected = [this](int index) {
            if (onPresetSelected) onPresetSelected(index);
        };
        dropdown.onItemDeleteRequested = [this](int index) {
            if (onDeletePreset) onDeletePreset(index);
        };

        saveButton.onClick = [this]() { showSaveDialog(); };
    }

    void showSaveDialog() {
        if (PlatformUtils::isIOS()) {
            showSaveDialogIOS();
            return;
        }
        showSaveDialogDesktop();
    }

    void showSaveDialogDesktop() {
        juce::Component* parent = dialogParent ? dialogParent : getTopLevelComponent();
        if (!parent) return;

        if (!saveDialog) {
            saveDialog = std::make_unique<SavePresetDialogComponent>();
            saveDialog->onSave = [this](const juce::String& name) {
                if (onSavePreset) onSavePreset(name);
                hideSaveDialog();
            };
            saveDialog->onCancel = [this]() { hideSaveDialog(); };
        }

        saveDialog->clearInput();
        parent->addAndMakeVisible(*saveDialog);

        int dialogX = (parent->getWidth() - saveDialog->getWidth()) / 2;
        int dialogY = (parent->getHeight() - saveDialog->getHeight()) / 2;
        saveDialog->setBounds(dialogX, dialogY, saveDialog->getWidth(), saveDialog->getHeight());

        saveDialog->focusInput();
        saveDialog->toFront(true);
    }

    void showSaveDialogIOS() {
        iOSNativeDialogs::showTextInputAlert(
            getTopLevelComponent(),
            "Save Preset",
            "Preset name",
            "",
            [this](const juce::String& name) {
                auto trimmed = name.trim();
                if (trimmed.isNotEmpty() && onSavePreset)
                    onSavePreset(trimmed);
            });
    }

    void hideSaveDialog() {
        if (saveDialog) {
            if (auto* parent = saveDialog->getParentComponent())
                parent->removeChildComponent(saveDialog.get());
        }
    }

    PresetCycleButton cycleDownButton { PresetCycleButton::Direction::Left };
    StyledDropdown dropdown;
    PresetCycleButton cycleUpButton { PresetCycleButton::Direction::Right };
    SavePresetButton saveButton;
    std::unique_ptr<SavePresetDialogComponent> saveDialog;
    juce::Component* dialogParent = nullptr;
};
