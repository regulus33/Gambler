#pragma once

#include "../PluginProcessor.h"
#include "MainScreenComponent.h"
#include "SettingsScreenComponent.h"
#include "../backend/ClockMode.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * ApplicationComponent
 *
 * Top-level component that orchestrates different screens:
 * - MainScreen: The main sequencer interface
 * - SettingsScreen: The settings interface
 *
 * This component handles:
 * - Screen navigation
 * - Processor callbacks and async updates
 * - Settings state management
 * - Preset management
 */
class ApplicationComponent : public juce::Component,
                             private juce::AsyncUpdater {
public:
    explicit ApplicationComponent(PluginProcessor& processor)
        : processorRef(processor),
          mainScreen(processor)
    {
        // Add main screen (visible by default)
        addAndMakeVisible(mainScreen);

        // Setup navbar callback for settings button
        mainScreen.getNavBar().onSettingsClicked = [this]() {
            showSettingsScreen();
        };

        // Setup preset selector callbacks
        setupPresetSelector();

        // Setup async update callback for main screen
        mainScreen.onAsyncUpdateNeeded = [this]() {
            triggerAsyncUpdate();
        };

        // Wire up the processor's tick callback to trigger async UI updates
        processorRef.onSequenceTick = [this]() {
            triggerAsyncUpdate();
        };

        // Wire up the completion callbacks notification to trigger async UI updates
        // This ensures UI updates happen after queued commands have executed
        processorRef.onCompletionCallbacksPending = [this]() {
            triggerAsyncUpdate();
        };
    }

    ~ApplicationComponent() override {
        // Clear the callbacks before destruction to avoid dangling reference
        processorRef.onSequenceTick = nullptr;
        processorRef.onCompletionCallbacksPending = nullptr;
        cancelPendingUpdate();
    }

    void paint(juce::Graphics& g) override {
        // Background is painted by child screens
    }

    void resized() override {
        auto area = getLocalBounds();

        // Both screens take up the full area
        mainScreen.setBounds(area);
        if (settingsScreen != nullptr) {
            settingsScreen->setBounds(area);
        }
    }

    // Get main screen for external access (e.g., for keyboard shortcuts)
    MainScreenComponent& getMainScreen() { return mainScreen; }

private:
    void handleAsyncUpdate() override {
        // Execute any pending completion callbacks from queued commands
        // These callbacks contain UI update logic that should run after domain state changes
        int numCallbacks = processorRef.executeCompletionCallbacks();
        if (numCallbacks > 0) {
            DBG("Executed " << numCallbacks << " completion callbacks");
        }

        // Update the main screen from sequence state
        mainScreen.updateFromSequence();
    }

    void showSettingsScreen() {
        if (!settingsScreen) {
            settingsScreen = std::make_unique<SettingsScreenComponent>();

            // Setup navbar callback to go back to main screen
            settingsScreen->getNavBar().onSettingsClicked = [this]() {
                showMainScreen();
            };

            // Set the settings button to toggled state (showing X icon)
            settingsScreen->getNavBar().setSettingsToggled(true);

            // Wire up settings callbacks to processor's clock
            // Sync initial state from processor
            settingsScreen->setInternalClockEnabled(processorRef.getClock().getMode() == ClockMode::INTERNAL);
            settingsScreen->setTempo(processorRef.getClock().getBpm());

            settingsScreen->onInternalClockChanged = [this](bool enabled) {
                processorRef.queueCommand(ProcessorCommand([this, enabled]() {
                    if (enabled) {
                        processorRef.getClock().setMode(ClockMode::INTERNAL);
                        processorRef.getClock().start();
                    } else {
                        processorRef.getClock().setMode(ClockMode::DAW_SYNC);
                        processorRef.getClock().stop();
                    }
                    DBG("Internal clock " + juce::String(enabled ? "enabled" : "disabled"));
                }));
            };

            settingsScreen->onTempoChanged = [this](double bpm) {
                processorRef.queueCommand(ProcessorCommand([this, bpm]() {
                    processorRef.getClock().setBpm(bpm);
                    DBG("Tempo changed to: " + juce::String(bpm) + " BPM");
                }));
            };

            // Sync note length setting from processor
            settingsScreen->setNoteLength(processorRef.getUserPreferences().defaultNoteLength);
            settingsScreen->onNoteLengthChanged = [this](double length) {
                processorRef.queueCommand(ProcessorCommand([this, length]() {
                    processorRef.getUserPreferences().defaultNoteLength = length;
                    DBG("Note length changed to: " + juce::String(length * 100.0) + "%");
                }));
            };

            settingsScreen->onPlayClicked = [this]() {
                processorRef.queueCommand(ProcessorCommand([this]() {
                    // Reset sequence to beginning and start clock
                    processorRef.getSequence().reset();
                    processorRef.getClock().start();
                    DBG("Play clicked - sequence reset to index 0, clock started");
                }));
            };

            settingsScreen->onStopClicked = [this]() {
                processorRef.queueCommand(ProcessorCommand([this]() {
                    processorRef.getClock().stop();
                    DBG("Stop clicked - clock stopped");
                }));
            };

            // Sync preview notes setting from processor
            settingsScreen->setPreviewNotesEnabled(processorRef.getUserPreferences().previewNotesOnEdit);
            settingsScreen->onPreviewNotesChanged = [this](bool enabled) {
                processorRef.getUserPreferences().previewNotesOnEdit = enabled;
                DBG("Preview notes on edit: " + juce::String(enabled ? "enabled" : "disabled"));
            };

            // Sync follow step setting from processor
            settingsScreen->setFollowStepEnabled(processorRef.getUserPreferences().followStep);
            settingsScreen->onFollowStepChanged = [this](bool enabled) {
                processorRef.getUserPreferences().followStep = enabled;
                DBG("Follow step: " + juce::String(enabled ? "enabled" : "disabled"));
            };

            // Sync follow playback setting from processor
            settingsScreen->setFollowPlaybackEnabled(processorRef.getUserPreferences().followPlayback);
            settingsScreen->onFollowPlaybackChanged = [this](bool enabled) {
                processorRef.getUserPreferences().followPlayback = enabled;
                DBG("Follow playback: " + juce::String(enabled ? "enabled" : "disabled"));
            };

            addAndMakeVisible(*settingsScreen);
            settingsScreen->setBounds(getLocalBounds());
        }

        // Show settings screen, hide main screen
        mainScreen.setVisible(false);
        settingsScreen->setVisible(true);

        // Toggle the main screen's settings button to show X icon
        mainScreen.getNavBar().setSettingsToggled(true);
    }

    void showMainScreen() {
        // Show main screen, hide settings screen
        if (settingsScreen) {
            settingsScreen->setVisible(false);
        }
        mainScreen.setVisible(true);

        // Toggle the main screen's settings button back to gear icon
        mainScreen.getNavBar().setSettingsToggled(false);
    }

    void setupPresetSelector() {
        auto& presetSelector = mainScreen.getNavBar().getPresetSelector();

        // Set this component as the dialog parent for proper centering
        presetSelector.setDialogParent(this);

        // Refresh preset list from disk
        refreshPresetList();

        // Wire up preset selection callback
        presetSelector.onPresetSelected = [this](int index) {
            loadPreset(index);
        };

        // Wire up save preset callback
        presetSelector.onSavePreset = [this](const juce::String& name) {
            savePreset(name);
        };

        // Wire up delete preset callback
        presetSelector.onDeletePreset = [this](int index) {
            deletePreset(index);
        };
    }

    void refreshPresetList() {
        auto& presetManager = processorRef.getPresetManager();
        presetManager.refreshPresetList();

        auto& presetSelector = mainScreen.getNavBar().getPresetSelector();
        presetSelector.setPresets(presetManager.getPresetNames());

        // Select current preset if one is loaded
        const auto& currentName = presetManager.getCurrentPresetName();
        if (!currentName.isEmpty()) {
            presetSelector.selectPresetByName(currentName, false);
        }
    }

    void loadPreset(int index) {
        auto& presetManager = processorRef.getPresetManager();
        auto xml = presetManager.loadPresetByIndex(index);

        if (xml) {
            // Load the state
            processorRef.loadStateFromXml(*xml);

            // Update current preset name
            const auto& presets = presetManager.getPresets();
            if (index >= 0 && index < static_cast<int>(presets.size())) {
                presetManager.setCurrentPresetName(presets[static_cast<size_t>(index)].name);
            }

            // Trigger UI update
            triggerAsyncUpdate();

            DBG("Loaded preset at index " + juce::String(index));
        }
    }

    void savePreset(const juce::String& name) {
        auto& presetManager = processorRef.getPresetManager();

        // Get current state as XML
        auto xml = processorRef.getStateAsXml();

        if (xml && presetManager.savePreset(name, *xml)) {
            // Update current preset name
            presetManager.setCurrentPresetName(name);

            // Refresh the preset list
            refreshPresetList();

            // Select the newly saved preset
            auto& presetSelector = mainScreen.getNavBar().getPresetSelector();
            presetSelector.selectPresetByName(name, false);

            DBG("Saved preset: " + name);
        }
    }

    void deletePreset(int index) {
        auto& presetManager = processorRef.getPresetManager();
        auto& presetSelector = mainScreen.getNavBar().getPresetSelector();

        // Get the preset name before deleting (for checking if it's current)
        const auto& presets = presetManager.getPresets();
        juce::String deletedName;
        if (index >= 0 && index < static_cast<int>(presets.size())) {
            deletedName = presets[static_cast<size_t>(index)].name;
        }

        if (presetManager.deletePresetByIndex(index)) {
            // Refresh the preset list
            refreshPresetList();

            // If we deleted the current preset, clear the selection
            if (deletedName == presetManager.getCurrentPresetName()) {
                presetManager.setCurrentPresetName("");
                presetSelector.clearSelection();
            }

            DBG("Deleted preset at index " + juce::String(index));
        }
    }

    PluginProcessor& processorRef;
    MainScreenComponent mainScreen;
    std::unique_ptr<SettingsScreenComponent> settingsScreen;
};

