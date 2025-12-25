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

            addAndMakeVisible(*settingsScreen);
            settingsScreen->setBounds(getLocalBounds());
        }

        // Show settings screen, hide main screen
        mainScreen.setVisible(false);
        settingsScreen->setVisible(true);
    }

    void showMainScreen() {
        // Show main screen, hide settings screen
        if (settingsScreen) {
            settingsScreen->setVisible(false);
        }
        mainScreen.setVisible(true);
    }

    PluginProcessor& processorRef;
    MainScreenComponent mainScreen;
    std::unique_ptr<SettingsScreenComponent> settingsScreen;
};

