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
 * Top-level component that orchestrates the entire UI:
 * - Owns the single NavBarComponent (shared across screens)
 * - Owns MainScreenComponent and SettingsScreenComponent
 * - Centralizes all processor-facing callback wiring
 * - Handles screen navigation, preset management, settings state
 * - Listens to APVTS params that require UI-specific handling
 */
class ApplicationComponent : public juce::Component,
                             private juce::AsyncUpdater,
                             private juce::AudioProcessorValueTreeState::Listener {
public:
    explicit ApplicationComponent(PluginProcessor& processor)
        : processorRef(processor),
          navBar(processor),
          mainScreen(processor)
    {
        // Add navbar (single instance, always visible above screens)
        addAndMakeVisible(navBar);

        // Add main screen (visible by default)
        addAndMakeVisible(mainScreen);

        // ========================================
        // NavBar callbacks (wired once, shared across screens)
        // ========================================

        navBar.onSettingsClicked = [this]() {
            if (settingsScreen && settingsScreen->isVisible())
                showMainScreen();
            else
                showSettingsScreen();
        };

        navBar.onMidiLearnClicked = [this](bool toggled) {
            if (toggled) {
                processorRef.getMidiLearnRegistry().enterLearnMode();
                DBG("MIDI Learn mode ENABLED");
            } else {
                processorRef.getMidiLearnRegistry().exitLearnMode();
                DBG("MIDI Learn mode DISABLED");
            }
            mainScreen.repaint();
            repaintMidiLearnComponents();
        };

        navBar.onPlayPauseClicked = [this]() {
            processorRef.togglePlayPause();
            updatePlayPauseButtonState();
        };

        navBar.onStopClicked = [this]() {
            processorRef.stopSequencer();
            updatePlayPauseButtonState();
        };

        navBar.onExportClicked = [this]() {
            exportMidiFile();
        };
        navBar.onExportDragStarted = [this]() -> juce::File {
            auto midiFile = MidiExporter::createMidiFile(processorRef.getState());
            return MidiExporter::writeToTempFile(midiFile);
        };

        navBar.onCompactViewToggled = [this](bool compact) {
            mainScreen.setCompactMode(compact);
        };

        navBar.onSwapRowClicked = [this](bool showingTop) {
            mainScreen.setShowingTopRow(showingTop);
        };

        // ========================================
        // Preset selector
        // ========================================
        setupPresetSelector();

        // ========================================
        // MainScreen child component callbacks
        // (previously scattered across MainScreenComponent)
        // ========================================

        // Piano roll: scale changes
        mainScreen.getPianoRoll().onNoteChanged = [this]() {
            Scale newScale = mainScreen.getPianoRoll().getScale();
            auto& state = processorRef.getState();
            state.setGlobalScaleNotes(newScale.getScaleNotes());
            DBG("Piano roll scale changed - updated global scale");

            int selectedStep = state.getSelectedStepForEditing();
            int selectedNote = state.getCurrentNoteForStep(selectedStep);
            updateNoteControlOptions(selectedNote);

            triggerAsyncUpdate();
        };

        // Note controls: pitch class changes
        mainScreen.getMainControls().getNoteControls().onNoteChanged = [this](int pitchClass) {
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int currentNote = state.getCurrentNoteForStep(selectedStep);
            int currentOctave = currentNote / 12;
            int newNote = currentOctave * 12 + pitchClass;

            state.updateCurrentNoteForStep(selectedStep, newNote);
            DBG("Note changed to pitch class " << pitchClass << " -> MIDI note " << newNote);

            if (auto* param = dynamic_cast<juce::AudioParameterBool*>(
                processorRef.getAPVTS().getParameter(Params::Keys::ID::PREF_PREVIEW_NOTES))) {
                if (param->get()) {
                    processorRef.triggerCurrentStepMidi(newNote);
                }
            }
        };

        // Note controls: octave changes
        mainScreen.getMainControls().getNoteControls().onOctaveChanged = [this](int octaveIndex) {
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int currentNote = state.getCurrentNoteForStep(selectedStep);
            int pitchClass = currentNote % 12;
            int newNote = octaveIndex * 12 + pitchClass;

            state.updateCurrentNoteForStep(selectedStep, newNote);
            DBG("Octave changed to " << octaveIndex << " -> MIDI note " << newNote);

            if (auto* param = dynamic_cast<juce::AudioParameterBool*>(
                processorRef.getAPVTS().getParameter(Params::Keys::ID::PREF_PREVIEW_NOTES))) {
                if (param->get()) {
                    processorRef.triggerCurrentStepMidi(newNote);
                }
            }
        };

        // Sequence grid: click to navigate
        mainScreen.getSequenceOverview().getSequenceGrid().onSquareClicked = [this](int stepIndex) {
            int rowIndex = stepIndex / 8;
            int windowStart = rowIndex * 8;
            mainScreen.setSequencerWindow(windowStart);

            auto& state = processorRef.getState();
            state.setSelectedStepForEditing(stepIndex);

            int currentStep = state.getCurrentIndex();
            mainScreen.getSequenceOverview().getSequenceGrid().updateFromState(state, currentStep);
            mainScreen.setActiveStepAndNote(stepIndex);

            DBG("Sequence grid clicked: step " + juce::String(stepIndex) +
                " -> navigating to row " + juce::String(rowIndex));
        };

        // Sequence grid: double-click to jump
        mainScreen.getSequenceOverview().getSequenceGrid().onSquareDoubleClicked = [this](int stepIndex) {
            auto& state = processorRef.getState();
            if (stepIndex < state.getLength()) {
                state.setCurrentIndex(stepIndex);
                DBG("Sequence grid double-clicked: forcing jump to step " + juce::String(stepIndex));
            }
        };

        // Sequence grid: drag to change length
        mainScreen.getSequenceOverview().getSequenceGrid().onLengthDragged = [this](int newLength) {
            auto& state = processorRef.getState();
            state.setLength(newLength);
            DBG("Sequence grid dragged: new length = " + juce::String(newLength));

            int currentStep = state.getCurrentIndex();
            mainScreen.getSequenceOverview().getSequenceGrid().updateFromState(state, currentStep);
        };

        // Subsequence controls
        setupSubsequenceCallbacks();

        // Sequencer step clicks
        mainScreen.getSequencerSteps().onStepClicked = [this](int stepIndex) {
            processorRef.triggerStepMidi(stepIndex);
            mainScreen.updateSubsequenceBar();
            DBG("Step clicked: " + juce::String(stepIndex));
        };

        // MIDI learn registry wiring for all components
        setupMidiLearnRegistries();

        // Note range callbacks
        setupNoteRangeCallbacks();

        // ========================================
        // MainScreen layout callbacks
        // ========================================

        mainScreen.onIdealHeightChanged = [this](int newHeight) {
            if (onDesignHeightChanged) onDesignHeightChanged(newHeight);
        };

        // ========================================
        // Processor callbacks
        // ========================================

        processorRef.onSequenceTick = [this]() {
            triggerAsyncUpdate();
        };

        processorRef.onNoteOff = [this]() {
            juce::MessageManager::callAsync([this]() {
                mainScreen.getNoteRangeControls().clearActiveNote();
            });
        };

        processorRef.onMidiLearned = [this]() {
            triggerAsyncUpdate();
        };

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

        // ========================================
        // APVTS parameter listeners (UI-specific actions needing window position)
        // ========================================
        processorRef.getAPVTS().addParameterListener(Params::Keys::ID::INDEX_LEFT, this);
        processorRef.getAPVTS().addParameterListener(Params::Keys::ID::INDEX_RIGHT, this);
        processorRef.getAPVTS().addParameterListener(Params::Keys::ID::CLEAR_WINDOW, this);
    }

    ~ApplicationComponent() override {
        // Remove APVTS listeners
        processorRef.getAPVTS().removeParameterListener(Params::Keys::ID::INDEX_LEFT, this);
        processorRef.getAPVTS().removeParameterListener(Params::Keys::ID::INDEX_RIGHT, this);
        processorRef.getAPVTS().removeParameterListener(Params::Keys::ID::CLEAR_WINDOW, this);

        // Clear processor callbacks
        processorRef.onSequenceTick = nullptr;
        processorRef.onNoteOff = nullptr;
        processorRef.onMidiLearned = nullptr;
        processorRef.onPlayPauseTriggered = nullptr;
        processorRef.onStopTriggered = nullptr;
        cancelPendingUpdate();
    }

    void paint(juce::Graphics&) override {}

    void resized() override {
        auto area = getLocalBounds();

        // NavBar spans the full width at the top
        navBar.setBounds(area.removeFromTop(NavBarConstants::height)
                            .withTrimmedLeft(MainScreenLayout::marginLeft + MainScreenLayout::pianoRollWidth + MainScreenLayout::topRowGap));

        // Screens take up the full area (they reserve the navbar height internally for layout alignment)
        auto fullArea = getLocalBounds();
        mainScreen.setBounds(fullArea);
        if (settingsScreen != nullptr) {
            settingsScreen->setBounds(fullArea);
        }
    }

    // Public accessors
    MainScreenComponent& getMainScreen() { return mainScreen; }
    NavBarComponent& getNavBar() { return navBar; }

    // Callback when the design height changes (compact mode toggle)
    std::function<void(int)> onDesignHeightChanged;

private:
    // ========================================
    // AsyncUpdater
    // ========================================
    void handleAsyncUpdate() override {
        processorRef.getState().applyPendingNoteWrites();
        mainScreen.updateFromSequence();

        // Update note/octave slider options (moved from MainScreenComponent)
        auto& state = processorRef.getState();
        int selectedStep = state.getSelectedStepForEditing();
        int selectedNote = state.getCurrentNoteForStep(selectedStep);
        updateNoteControlOptions(selectedNote);

        updatePlayPauseButtonState();
        repaintMidiLearnComponents();
    }

    // ========================================
    // APVTS Listener (INDEX_LEFT, INDEX_RIGHT, CLEAR_WINDOW)
    // ========================================
    void parameterChanged(const juce::String& parameterID, float newValue) override {
        bool isOn = newValue > 0.5f;
        if (!isOn) return;

        juce::MessageManager::callAsync([this, parameterID]() {
            auto& state = processorRef.getState();

            if (parameterID == Params::Keys::ID::INDEX_LEFT) {
                mainScreen.setSequencerWindow(state.getViewWindowStart() - 8);
                DBG("Page left, window now at: " + juce::String(state.getViewWindowStart()));
            }
            else if (parameterID == Params::Keys::ID::INDEX_RIGHT) {
                mainScreen.setSequencerWindow(state.getViewWindowStart() + 8);
                DBG("Page right, window now at: " + juce::String(state.getViewWindowStart()));
            }
            else if (parameterID == Params::Keys::ID::CLEAR_WINDOW) {
                int windowStart = state.getViewWindowStart();
                for (int i = 0; i < 8; i++) {
                    int stepIdx = windowStart + i;
                    for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                        state.setStepNote(stepIdx, n, -1);
                    }
                }
                mainScreen.getSequencerSteps().repaint();
                DBG("Cleared window starting at " + juce::String(windowStart));
            }
        });
    }

    // ========================================
    // UI Helpers
    // ========================================

    void repaintMidiLearnComponents() {
        mainScreen.getMainControls().getNoteControls().getOctaveSlider().repaint();
        mainScreen.getMainControls().getNoteControls().getNoteSlider().repaint();
        mainScreen.getMainControls().getProbabilityControls().getProbabilitySlider().repaint();
        mainScreen.getSequencerControls().repaint();
        mainScreen.getSequencerSteps().repaint();
        mainScreen.getPianoRoll().repaint();
        mainScreen.getNoteRangeControls().repaint();
    }

    void updatePlayPauseButtonState() {
        auto state = processorRef.getSequencerState();
        bool isPlaying = (state == PluginProcessor::SequencerState::Playing);
        bool isStopped = (state == PluginProcessor::SequencerState::Stopped);

        navBar.setPlayPauseState(isPlaying);
        navBar.setStopButtonActive(isStopped);
    }

    /**
     * Recompute and push filtered note and octave options to the NoteControlsComponent.
     * (Moved from MainScreenComponent)
     */
    void updateNoteControlOptions(int currentMidiNote) {
        auto& state = processorRef.getState();
        int rangeMin = state.getScaleRangeMin();
        int rangeMax = state.getScaleRangeMax();

        // Note slider: pitch classes reachable in range AND in scale
        juce::Array<juce::String> noteNames;
        juce::Array<juce::Colour> noteColours;
        juce::Array<int>          noteIndices;

        for (int pc = 0; pc < 12; ++pc) {
            if (!state.getGlobalScaleNote(pc))
                continue;

            bool reachable = false;
            for (int midi = pc; midi <= 127; midi += 12) {
                if (midi >= rangeMin && midi <= rangeMax) {
                    reachable = true;
                    break;
                }
            }
            if (!reachable)
                continue;

            noteNames.add(MusicConstants::CHROMATIC_NOTE_NAMES[static_cast<size_t>(pc)]);
            noteColours.add(state.getNoteColour(pc));
            noteIndices.add(pc);
        }

        if (noteNames.isEmpty()) {
            auto selectedIndices  = mainScreen.getPianoRoll().getSelectedNoteIndices();
            auto selectedNames    = mainScreen.getPianoRoll().getSelectedNoteNames();
            auto selectedColours  = mainScreen.getPianoRoll().getSelectedNoteColours();
            mainScreen.getMainControls().getNoteControls().updateNoteOptions(selectedNames, selectedColours, selectedIndices);
        } else {
            mainScreen.getMainControls().getNoteControls().updateNoteOptions(noteNames, noteColours, noteIndices);
        }

        // Octave slider: octaves where (octave*12 + pitchClass) is in range
        int pitchClass = (currentMidiNote >= 0) ? (currentMidiNote % 12) : 0;

        juce::Array<juce::String> octaveLabels;
        juce::Array<juce::Colour> octaveColours;
        juce::Array<int>          octaveIndices;

        for (int oct = 0; oct < 10; ++oct) {
            int midi = oct * 12 + pitchClass;
            if (midi >= rangeMin && midi <= rangeMax) {
                octaveLabels.add(MusicConstants::OCTAVE_LABELS[static_cast<size_t>(oct)]);
                octaveColours.add(MusicConstants::OCTAVE_COLOURS[static_cast<size_t>(oct)]);
                octaveIndices.add(oct);
            }
        }

        if (octaveLabels.isEmpty()) {
            octaveLabels  = MusicConstants::toJuceArray(MusicConstants::OCTAVE_LABELS);
            octaveColours = MusicConstants::toJuceArray(MusicConstants::OCTAVE_COLOURS);
            for (int i = 0; i < 10; ++i) octaveIndices.add(i);
        }

        mainScreen.getMainControls().getNoteControls().updateOctaveOptions(octaveLabels, octaveColours, octaveIndices);
    }

    // ========================================
    // Callback setup helpers
    // ========================================

    void setupSubsequenceCallbacks() {
        auto& subControls = mainScreen.getSubsequenceControls();
        subControls.setMidiLearnRegistry(&processorRef.getMidiLearnRegistry());

        subControls.onBarClicked = [this](int index) {
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int substepLength = state.getStepNotesLength(selectedStep);

            if (index < substepLength) {
                state.setStepNotesCurrentIndex(selectedStep, index);
                DBG("Substep " + juce::String(index) + " selected");

                mainScreen.updateSubsequenceBar();
                int midiNote = state.getCurrentNoteForStep(selectedStep);
                int chromaticNote = midiNote % 12;
                mainScreen.getPianoRoll().setActiveNote(chromaticNote);
                mainScreen.getMainControls().getNoteControls().setFromMidiNote(midiNote);
                mainScreen.getMainControls().getNoteControls().setRestOverlay(midiNote < 0);
            }
        };
        subControls.onMinusClicked = [this]() {
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int currentLength = state.getStepNotesLength(selectedStep);
            if (currentLength > 1) {
                state.setStepNotesLength(selectedStep, currentLength - 1);
                DBG("Subsequence length decreased to: " + juce::String(currentLength - 1));
                mainScreen.updateSubsequenceBar();
            }
        };
        subControls.onPlusClicked = [this]() {
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int currentLength = state.getStepNotesLength(selectedStep);
            if (currentLength < ParamConstants::MAX_SUBSTEPS) {
                state.setStepNotesLength(selectedStep, currentLength + 1);
                DBG("Subsequence length increased to: " + juce::String(currentLength + 1));
                mainScreen.updateSubsequenceBar();
            }
        };
    }

    void setupMidiLearnRegistries() {
        auto& registry = processorRef.getMidiLearnRegistry();

        mainScreen.getSequencerSteps().setRegistry(&registry);
        mainScreen.getSequencerControls().setRegistry(&registry);
        mainScreen.getMainControls().setRegistry(&registry);
        mainScreen.getPianoRoll().setRegistry(&registry);

        navBar.getSubdivisionSlider().setRegistry(&registry);
        navBar.getSubdivisionSlider().onMidiLearnSelected = [this]() {
            triggerAsyncUpdate();
        };

        navBar.setSettingsButtonRegistry(&registry);

        mainScreen.getMainControls().setMidiLearnSelectedCallback([this]() {
            triggerAsyncUpdate();
        });
    }

    void setupNoteRangeCallbacks() {
        auto& noteRange = mainScreen.getNoteRangeControls();

        noteRange.setRegistry(&processorRef.getMidiLearnRegistry());
        noteRange.setMidiLearnSelectedCallback([this]() {
            triggerAsyncUpdate();
        });

        noteRange.onRangeChanged = [this]() {
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int selectedNote = state.getCurrentNoteForStep(selectedStep);
            updateNoteControlOptions(selectedNote);
            mainScreen.getMainControls().getNoteControls().setFromMidiNote(selectedNote);
        };

        noteRange.setWindow(
            processorRef.getState().getScaleWindowOffset(),
            processorRef.getState().getScaleWindowSize());
    }

    // ========================================
    // Screen navigation
    // ========================================

    void showSettingsScreen() {
        wasCompactBeforeSettings = mainScreen.isCompactMode();
        if (wasCompactBeforeSettings) {
            if (onDesignHeightChanged)
                onDesignHeightChanged(MainScreenComponent::fullDesignHeight);
        }

        if (!settingsScreen) {
            settingsScreen = std::make_unique<SettingsScreenComponent>(processorRef);

            // Wire up settings callbacks to processor's clock
            auto currentClockMode = processorRef.getClock().getMode();
            settingsScreen->setInternalClockEnabled(currentClockMode == ClockMode::INTERNAL);
            settingsScreen->setExternalMidiClockEnabled(currentClockMode == ClockMode::EXTERNAL_MIDI_CLOCK);
            settingsScreen->setTempo(processorRef.getClock().getBpm());

            settingsScreen->onInternalClockChanged = [this](bool enabled) {
                if (auto* param = processorRef.getAPVTS().getParameter(Params::Keys::ID::CLOCK_MODE)) {
                    param->setValueNotifyingHost(param->convertTo0to1(enabled ? 0.0f : 1.0f));
                    DBG("Internal clock " + juce::String(enabled ? "enabled" : "disabled"));
                }
            };

            settingsScreen->onTempoChanged = [this](double bpm) {
                if (auto* param = processorRef.getAPVTS().getParameter(Params::Keys::ID::CLOCK_BPM)) {
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(bpm)));
                    DBG("Tempo changed to: " + juce::String(bpm) + " BPM");
                }
            };

            float currentNoteLength = processorRef.getState().getNoteLength();
            settingsScreen->setNoteLength(currentNoteLength);
            settingsScreen->onNoteLengthChanged = [this](double length) {
                processorRef.getState().setNoteLength(static_cast<float>(length));
            };

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

            settingsScreen->setMidiClockOutputEnabled(false);
            settingsScreen->onMidiClockOutputChanged = [this](bool enabled) {
                if (enabled) {
                    processorRef.openAllMidiClockOutputs();
                    DBG("MIDI clock output enabled - opened all devices");
                } else {
                    processorRef.closeAllMidiClockOutputs();
                    DBG("MIDI clock output disabled - closed all devices");
                }
            };

            settingsScreen->onExternalMidiClockChanged = [this](bool enabled) {
                if (auto* param = processorRef.getAPVTS().getParameter(Params::Keys::ID::CLOCK_MODE)) {
                    param->setValueNotifyingHost(param->convertTo0to1(enabled ? 2.0f : 0.0f));
                    DBG("External MIDI clock " + juce::String(enabled ? "enabled" : "disabled"));
                }
            };

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

        // Toggle settings button to show X icon
        navBar.setSettingsToggled(true);

        // Sync MIDI Learn button state
        bool midiLearnActive = processorRef.getMidiLearnRegistry().isLearning();
        navBar.setMidiLearnToggled(midiLearnActive);
    }

    void showMainScreen() {
        if (settingsScreen) {
            settingsScreen->setVisible(false);
        }
        mainScreen.setVisible(true);

        // Toggle settings button back to gear icon
        navBar.setSettingsToggled(false);

        // Sync MIDI Learn button state
        bool midiLearnActive = processorRef.getMidiLearnRegistry().isLearning();
        navBar.setMidiLearnToggled(midiLearnActive);

        // Restore compact mode height if we were compact before opening settings
        if (wasCompactBeforeSettings) {
            wasCompactBeforeSettings = false;
            if (onDesignHeightChanged)
                onDesignHeightChanged(mainScreen.getIdealHeight());
        }
    }

    // ========================================
    // Preset management
    // ========================================

    void setupPresetSelector() {
        auto& presetSelector = navBar.getPresetSelector();

        presetSelector.setDialogParent(this);
        refreshPresetList();

        presetSelector.onPresetSelected = [this](int index) {
            loadPreset(index);
        };
        presetSelector.onSavePreset = [this](const juce::String& name) {
            savePreset(name);
        };
        presetSelector.onDeletePreset = [this](int index) {
            deletePreset(index);
        };
        presetSelector.onCycleUp = [this]() { cyclePresetUp(); };
        presetSelector.onCycleDown = [this]() { cyclePresetDown(); };

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

        navBar.getPresetSelector().selectPresetByName(presets[static_cast<size_t>(nextIndex)].name, false);
    }

    void cyclePresetDown() {
        auto& presetManager = processorRef.getPresetManager();
        const auto& presets = presetManager.getPresets();
        if (presets.empty()) return;

        int currentIndex = presetManager.getPresetIndex(presetManager.getCurrentPresetName());
        int count = static_cast<int>(presets.size());
        int prevIndex = (currentIndex - 1 + count) % count;
        loadPreset(prevIndex);

        navBar.getPresetSelector().selectPresetByName(presets[static_cast<size_t>(prevIndex)].name, false);
    }

    void refreshPresetList() {
        auto& presetManager = processorRef.getPresetManager();
        presetManager.refreshPresetList();

        auto& presetSelector = navBar.getPresetSelector();
        presetSelector.setPresets(presetManager.getPresetNames());

        const auto& currentName = presetManager.getCurrentPresetName();
        if (!currentName.isEmpty()) {
            presetSelector.selectPresetByName(currentName, false);
        }
    }

    void loadPreset(int index) {
        auto& presetManager = processorRef.getPresetManager();
        auto xml = presetManager.loadPresetByIndex(index);

        if (xml) {
            processorRef.loadStateFromXml(*xml);

            const auto& presets = presetManager.getPresets();
            if (index >= 0 && index < static_cast<int>(presets.size())) {
                presetManager.setCurrentPresetName(presets[static_cast<size_t>(index)].name);
            }

            triggerAsyncUpdate();
            DBG("Loaded preset at index " + juce::String(index));
        }
    }

    void savePreset(const juce::String& name) {
        auto& presetManager = processorRef.getPresetManager();
        auto xml = processorRef.getStateAsXml();

        if (xml && presetManager.savePreset(name, *xml)) {
            presetManager.setCurrentPresetName(name);
            refreshPresetList();
            navBar.getPresetSelector().selectPresetByName(name, false);
            DBG("Saved preset: " + name);
        }
    }

    void deletePreset(int index) {
        auto& presetManager = processorRef.getPresetManager();
        auto& presetSelector = navBar.getPresetSelector();

        const auto& presets = presetManager.getPresets();
        juce::String deletedName;
        if (index >= 0 && index < static_cast<int>(presets.size())) {
            deletedName = presets[static_cast<size_t>(index)].name;
        }

        if (presetManager.deletePresetByIndex(index)) {
            refreshPresetList();

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

    // ========================================
    // Members
    // ========================================
    PluginProcessor& processorRef;
    NavBarComponent navBar;
    MainScreenComponent mainScreen;
    std::unique_ptr<SettingsScreenComponent> settingsScreen;
    std::unique_ptr<juce::FileChooser> fileChooser;
    bool wasCompactBeforeSettings = false;
};
