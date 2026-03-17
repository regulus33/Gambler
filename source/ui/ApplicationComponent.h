#pragma once

#include "../PluginProcessor.h"
#include "../backend/MidiExporter.h"
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

        // Setup navbar callback for MIDI Learn button
        mainScreen.getNavBar().onMidiLearnClicked = [this](bool toggled) {
            if (toggled) {
                // Enter MIDI learn mode
                processorRef.getMidiLearnRegistry().enterLearnMode();
                DBG("MIDI Learn mode ENABLED");
                // Trigger UI refresh to show overlays
                mainScreen.repaint();
                // Explicitly repaint components with MIDI learn overlays
                repaintMidiLearnComponents();
            } else {
                // Exit MIDI learn mode
                processorRef.getMidiLearnRegistry().exitLearnMode();
                DBG("MIDI Learn mode DISABLED");
                // Trigger UI refresh to hide overlays
                mainScreen.repaint();
                // Explicitly repaint components with MIDI learn overlays
                repaintMidiLearnComponents();
            }
        };

        // Setup navbar callback for Play/Pause button (toggles play/pause)
        mainScreen.getNavBar().onPlayPauseClicked = [this]() {
            processorRef.togglePlayPause();
            // Update button state immediately
            updatePlayPauseButtonState();
        };

        // Setup navbar callback for Stop button
        mainScreen.getNavBar().onStopClicked = [this]() {
            processorRef.stopSequencer();
            // Update button state immediately
            updatePlayPauseButtonState();
        };

        // Setup export button callbacks (click = file chooser, drag = D&D to DAW)
        mainScreen.getNavBar().onExportClicked = [this]() {
            exportMidiFile();
        };
        mainScreen.getNavBar().onExportDragStarted = [this]() -> juce::File {
            auto midiFile = MidiExporter::createMidiFile(processorRef.getState());
            return MidiExporter::writeToTempFile(midiFile);
        };

        // Note: WriteScale button doesn't need a callback - the logic is handled
        // in MainScreenComponent::updateFromSequence() which checks button state directly

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

        // Wire up note-off callback to clear active note highlight
        processorRef.onNoteOff = [this]() {
            // Use MessageManager to safely update UI from audio thread
            juce::MessageManager::callAsync([this]() {
                mainScreen.getNoteRangeControls().clearActiveNote();
            });
        };

        // Wire up MIDI learn callback to trigger UI updates when mapping is created
        processorRef.onMidiLearned = [this]() {
            triggerAsyncUpdate();
        };

        // Wire up transport callbacks (triggered by MIDI or DAW automation via params)
        processorRef.onPlayPauseTriggered = [this]() {
            juce::MessageManager::callAsync([this]() {
                updatePlayPauseButtonState();
            });
        };
        processorRef.onStopTriggered = [this]() {
            juce::MessageManager::callAsync([this]() {
                updatePlayPauseButtonState();
            });
        };
    }

    ~ApplicationComponent() override {
        // Clear the callbacks before destruction to avoid dangling reference
        processorRef.onSequenceTick = nullptr;
        processorRef.onNoteOff = nullptr;
        processorRef.onMidiLearned = nullptr;
        processorRef.onPlayPauseTriggered = nullptr;
        processorRef.onStopTriggered = nullptr;
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
        // Apply any pending note writes from the audio thread (roll mode)
        processorRef.getState().applyPendingNoteWrites();

        // Update the main screen from sequence state
        mainScreen.updateFromSequence();

        // Update play/pause button state
        updatePlayPauseButtonState();

        // Repaint MIDI learn components to show updated mappings
        repaintMidiLearnComponents();
    }

    void repaintMidiLearnComponents() {
        // Repaint all components that have MIDI learn overlays
        mainScreen.getMainControls().getNoteControls().getOctaveSlider().repaint();
        mainScreen.getMainControls().getNoteControls().getNoteSlider().repaint();
        mainScreen.getMainControls().getProbabilityControls().getProbabilitySlider().repaint();
        mainScreen.getSequencerControls().repaint();
        mainScreen.getSequencerSteps().repaint();
        mainScreen.getPianoRoll().repaint();
        mainScreen.getNoteRangeControls().repaint();
    }

    void updatePlayPauseButtonState() {
        // Update play/pause button based on sequencer state
        // When playing: show pause icon (toggled = true) with green color
        // When stopped/paused: show play icon (toggled = false) with white color
        auto state = processorRef.getSequencerState();
        bool isPlaying = (state == PluginProcessor::SequencerState::Playing);
        bool isStopped = (state == PluginProcessor::SequencerState::Stopped);

        mainScreen.getNavBar().setPlayPauseState(isPlaying);
        mainScreen.getNavBar().setStopButtonActive(isStopped);

        if (settingsScreen) {
            settingsScreen->getNavBar().setPlayPauseState(isPlaying);
            settingsScreen->getNavBar().setStopButtonActive(isStopped);
        }
    }

    void showSettingsScreen() {
        if (!settingsScreen) {
            settingsScreen = std::make_unique<SettingsScreenComponent>(processorRef);

            // Setup navbar callback to go back to main screen
            settingsScreen->getNavBar().onSettingsClicked = [this]() {
                showMainScreen();
            };

            // Set the settings button to toggled state (showing X icon)
            settingsScreen->getNavBar().setSettingsToggled(true);

            // Setup MIDI Learn button callback for settings screen
            settingsScreen->getNavBar().onMidiLearnClicked = [this](bool toggled) {
                if (toggled) {
                    processorRef.getMidiLearnRegistry().enterLearnMode();
                    DBG("MIDI Learn mode ENABLED (from settings)");
                    if (settingsScreen) settingsScreen->repaint();
                } else {
                    processorRef.getMidiLearnRegistry().exitLearnMode();
                    DBG("MIDI Learn mode DISABLED (from settings)");
                    if (settingsScreen) settingsScreen->repaint();
                }
            };

            // Setup navbar callback for Play/Pause button (toggles play/pause)
            settingsScreen->getNavBar().onPlayPauseClicked = [this]() {
                processorRef.togglePlayPause();
                // Update button state immediately
                updatePlayPauseButtonState();
            };

            // Setup navbar callback for Stop button
            settingsScreen->getNavBar().onStopClicked = [this]() {
                processorRef.stopSequencer();
                // Update button state immediately
                updatePlayPauseButtonState();
            };

            // Setup export button callbacks for settings screen
            settingsScreen->getNavBar().onExportClicked = [this]() {
                exportMidiFile();
            };
            settingsScreen->getNavBar().onExportDragStarted = [this]() -> juce::File {
                auto midiFile = MidiExporter::createMidiFile(processorRef.getState());
                return MidiExporter::writeToTempFile(midiFile);
            };

            // Note: WriteScale button doesn't need a callback - the logic is handled
            // in MainScreenComponent::updateFromSequence() which checks button state directly

            // Wire up settings callbacks to processor's clock
            // Sync initial state from processor
            auto currentClockMode = processorRef.getClock().getMode();
            settingsScreen->setInternalClockEnabled(currentClockMode == ClockMode::INTERNAL);
            settingsScreen->setExternalMidiClockEnabled(currentClockMode == ClockMode::EXTERNAL_MIDI_CLOCK);
            settingsScreen->setTempo(processorRef.getClock().getBpm());

            settingsScreen->onInternalClockChanged = [this](bool enabled) {
                // Set CLOCK_MODE via APVTS - parameterChanged handles applying to Clock
                if (auto* param = processorRef.getAPVTS().getParameter(Params::Keys::ID::CLOCK_MODE)) {
                    // 0=INTERNAL, 1=DAW_SYNC
                    param->setValueNotifyingHost(param->convertTo0to1(enabled ? 0.0f : 1.0f));
                    DBG("Internal clock " + juce::String(enabled ? "enabled" : "disabled"));
                }
            };

            settingsScreen->onTempoChanged = [this](double bpm) {
                // Set CLOCK_BPM via APVTS - parameterChanged handles applying to Clock
                if (auto* param = processorRef.getAPVTS().getParameter(Params::Keys::ID::CLOCK_BPM)) {
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(bpm)));
                    DBG("Tempo changed to: " + juce::String(bpm) + " BPM");
                }
            };

            // Sync note length setting from APVTS-backed state
            float currentNoteLength = processorRef.getState().getNoteLength();
            settingsScreen->setNoteLength(currentNoteLength);
            settingsScreen->onNoteLengthChanged = [this](double length) {
                // Update via APVTS-backed state (thread-safe)
                processorRef.getState().setNoteLength(static_cast<float>(length));
            };

            // Sync preview notes setting from APVTS
            if (auto* param = dynamic_cast<juce::AudioParameterBool*>(
                processorRef.getAPVTS().getParameter(Params::Keys::ID::PREF_PREVIEW_NOTES))) {
                settingsScreen->setPreviewNotesEnabled(param->get());
            }
            settingsScreen->onPreviewNotesChanged = [this](bool enabled) {
                if (auto* param = processorRef.getAPVTS().getParameter(Params::Keys::ID::PREF_PREVIEW_NOTES)) {
                    param->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
                    DBG("Preview notes on edit: " + juce::String(enabled ? "enabled" : "disabled"));
                }
            };

            // Sync follow step setting from APVTS
            if (auto* param = dynamic_cast<juce::AudioParameterBool*>(
                processorRef.getAPVTS().getParameter(Params::Keys::ID::PREF_FOLLOW_STEP))) {
                settingsScreen->setFollowStepEnabled(param->get());
            }
            settingsScreen->onFollowStepChanged = [this](bool enabled) {
                if (auto* param = processorRef.getAPVTS().getParameter(Params::Keys::ID::PREF_FOLLOW_STEP)) {
                    param->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
                    DBG("Follow step: " + juce::String(enabled ? "enabled" : "disabled"));
                }
            };



            // MIDI Clock Output toggle callback (standalone only)
            // Note: MIDI clock output is NOT stored in APVTS - it's a runtime setting only
            // We'll keep track of it in a local variable for now
            // TODO: Consider if this should be persisted or remain runtime-only
            settingsScreen->setMidiClockOutputEnabled(false); // Default to disabled
            settingsScreen->onMidiClockOutputChanged = [this](bool enabled) {
                if (enabled) {
                    processorRef.openAllMidiClockOutputs();
                    DBG("MIDI clock output enabled - opened all devices");
                } else {
                    processorRef.closeAllMidiClockOutputs();
                    DBG("MIDI clock output disabled - closed all devices");
                }
            };

            // External MIDI Clock toggle callback
            settingsScreen->onExternalMidiClockChanged = [this](bool enabled) {
                // Set CLOCK_MODE via APVTS - parameterChanged handles applying to Clock
                if (auto* param = processorRef.getAPVTS().getParameter(Params::Keys::ID::CLOCK_MODE)) {
                    // 2=EXTERNAL_MIDI_CLOCK, 0=INTERNAL
                    param->setValueNotifyingHost(param->convertTo0to1(enabled ? 2.0f : 0.0f));
                    DBG("External MIDI clock " + juce::String(enabled ? "enabled" : "disabled"));
                }
            };

            // Sync MIDI output channel from APVTS
            if (auto* param = dynamic_cast<juce::AudioParameterInt*>(
                    processorRef.getAPVTS().getParameter(Params::Keys::ID::MIDI_OUTPUT_CHANNEL))) {
                settingsScreen->setMidiOutputChannel(param->get());
            }
            settingsScreen->onMidiOutputChannelChanged = [this](int channel) {
                if (auto* param = processorRef.getAPVTS().getParameter(Params::Keys::ID::MIDI_OUTPUT_CHANNEL)) {
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(channel)));
                    DBG("MIDI output channel changed to: " + juce::String(channel));
                }
            };

            addAndMakeVisible(*settingsScreen);
            settingsScreen->setBounds(getLocalBounds());
        }

        // Show settings screen, hide main screen
        mainScreen.setVisible(false);
        settingsScreen->setVisible(true);

        // Toggle the main screen's settings button to show X icon
        mainScreen.getNavBar().setSettingsToggled(true);

        // Sync MIDI Learn button state from main screen
        bool midiLearnActive = processorRef.getMidiLearnRegistry().isLearning();
        settingsScreen->getNavBar().setMidiLearnToggled(midiLearnActive);
    }

    void showMainScreen() {
        // Show main screen, hide settings screen
        if (settingsScreen) {
            settingsScreen->setVisible(false);
        }
        mainScreen.setVisible(true);

        // Toggle the main screen's settings button back to gear icon
        mainScreen.getNavBar().setSettingsToggled(false);

        // Sync MIDI Learn button state from registry
        bool midiLearnActive = processorRef.getMidiLearnRegistry().isLearning();
        mainScreen.getNavBar().setMidiLearnToggled(midiLearnActive);
    }

    void setupPresetSelector() {
        auto& presetSelector = mainScreen.getNavBar().getPresetSelector();
        auto& navBar = mainScreen.getNavBar();

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

        // Wire up preset cycle buttons (now owned by PresetSelectorComponent)
        presetSelector.onCycleUp = [this]() { cyclePresetUp(); };
        presetSelector.onCycleDown = [this]() { cyclePresetDown(); };

        // Wire up processor MIDI-triggered cycle callbacks
        processorRef.onPresetCycleUp = [this]() {
            juce::MessageManager::callAsync([this]() { cyclePresetUp(); });
        };
        processorRef.onPresetCycleDown = [this]() {
            juce::MessageManager::callAsync([this]() { cyclePresetDown(); });
        };
    }

    void cyclePresetUp() {
        auto& presetManager = processorRef.getPresetManager();
        const auto& presets = presetManager.getPresets();
        if (presets.empty()) return;

        int currentIndex = presetManager.getPresetIndex(presetManager.getCurrentPresetName());
        int nextIndex = (currentIndex + 1) % static_cast<int>(presets.size());
        loadPreset(nextIndex);

        auto& presetSelector = mainScreen.getNavBar().getPresetSelector();
        presetSelector.selectPresetByName(presets[static_cast<size_t>(nextIndex)].name, false);
    }

    void cyclePresetDown() {
        auto& presetManager = processorRef.getPresetManager();
        const auto& presets = presetManager.getPresets();
        if (presets.empty()) return;

        int currentIndex = presetManager.getPresetIndex(presetManager.getCurrentPresetName());
        int count = static_cast<int>(presets.size());
        int prevIndex = (currentIndex - 1 + count) % count;
        loadPreset(prevIndex);

        auto& presetSelector = mainScreen.getNavBar().getPresetSelector();
        presetSelector.selectPresetByName(presets[static_cast<size_t>(prevIndex)].name, false);
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

    void exportMidiFile() {
        auto midiFile = MidiExporter::createMidiFile(processorRef.getState());

        fileChooser = std::make_unique<juce::FileChooser>(
            "Save MIDI File",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                .getChildFile("Gambler-Export.mid"),
            "*.mid");

        fileChooser->launchAsync(
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [midiFile](const juce::FileChooser& chooser) {
                auto result = chooser.getResult();
                if (result != juce::File()) {
                    MidiExporter::writeToFile(midiFile, result);
                }
            });
    }

    PluginProcessor& processorRef;
    MainScreenComponent mainScreen;
    std::unique_ptr<SettingsScreenComponent> settingsScreen;
    std::unique_ptr<juce::FileChooser> fileChooser;
};

