#include "UiBridge.h"
#include "../PluginProcessor.h"

namespace {
    using ID = juce::String;
    namespace P = Params::Keys::ID;

    constexpr int maxUndoDepth = 20;

    // The UI names its MIDI-learnable controls with strings; the registry uses MidiControlId.
    struct LearnTarget {
        const char* uiId;
        MidiControlId control;
    };

    const std::vector<LearnTarget>& learnTargets() {
        static const std::vector<LearnTarget> targets = [] {
            std::vector<LearnTarget> t {
                { "roll", MidiControlId::BUTTON_ROLL },
                { "nudge-left", MidiControlId::BUTTON_NUDGE_LEFT },
                { "nudge-right", MidiControlId::BUTTON_NUDGE_RIGHT },
                { "duplicate", MidiControlId::BUTTON_DUPLICATE },
                { "shuffle", MidiControlId::BUTTON_SHUFFLE },
                { "randomize-step", MidiControlId::BUTTON_RANDOMIZE },
                { "randomize-all", MidiControlId::BUTTON_RANDOMIZE_ALL },
                { "clear-step", MidiControlId::BUTTON_CLEAR },
                { "clear-page", MidiControlId::BUTTON_CLEAR_PAGE },
                { "play", MidiControlId::PLAY_PAUSE },
                { "stop", MidiControlId::STOP },
                { "subdivision", MidiControlId::SUBDIVISION_SLIDER },
                { "latch", MidiControlId::SCALE_LATCH },
                { "bake-scale", MidiControlId::DESTRUCTIVE_SCALE },
                { "bake-range", MidiControlId::DESTRUCTIVE_RANGE },
                { "range", MidiControlId::NOTE_RANGE_MIN },
                { "probability", MidiControlId::PROBABILITY_SLIDER },
                { "preset-prev", MidiControlId::PRESET_CYCLE_DOWN },
                { "preset-next", MidiControlId::PRESET_CYCLE_UP },
            };
            return t;
        }();
        return targets;
    }

    // Indexed families: "key-3", "step-0", "cycle-7".
    struct LearnFamily {
        const char* prefix;
        MidiControlId first;
        int count;
    };
    constexpr LearnFamily learnFamilies[] = {
        { "key-", MidiControlId::SCALE_NOTE_C, 12 },
        { "step-", MidiControlId::STEP_0, 8 },
        { "cycle-", MidiControlId::SUBSEQ_BAR_0, 8 },
    };

    MidiControlId controlForUiId (const juce::String& uiId) {
        for (const auto& family : learnFamilies) {
            if (uiId.startsWith (family.prefix)) {
                const int index = uiId.fromFirstOccurrenceOf (family.prefix, false, false).getIntValue();
                if (index >= 0 && index < family.count)
                    return static_cast<MidiControlId> (static_cast<int> (family.first) + index);
            }
        }
        for (const auto& target : learnTargets())
            if (uiId == target.uiId) return target.control;
        return MidiControlId::COUNT;
    }

    juce::String uiIdForControl (MidiControlId control) {
        const int value = static_cast<int> (control);
        for (const auto& family : learnFamilies) {
            const int first = static_cast<int> (family.first);
            if (value >= first && value < first + family.count)
                return juce::String (family.prefix) + juce::String (value - first);
        }
        for (const auto& target : learnTargets())
            if (control == target.control) return target.uiId;
        return {};
    }

    int intOf (const juce::var& payload, const char* key, int fallback = 0) {
        const auto& v = payload[key];
        return v.isVoid() ? fallback : static_cast<int> (v);
    }

    bool boolOf (const juce::var& payload, const char* key) {
        return static_cast<bool> (payload[key]);
    }

    bool validStep (int step) { return step >= 0 && step < ParamConstants::MAX_STEPS; }
    bool validSub (int sub) { return sub >= 0 && sub < ParamConstants::MAX_SUBSTEPS; }
}

UiBridge::UiBridge (PluginProcessor& p) : processor (p) {
    for (auto* param : processor.getParameters())
        if (auto* withId = dynamic_cast<juce::RangedAudioParameter*> (param))
            processor.getAPVTS().addParameterListener (withId->paramID, this);
    processor.getInternalState().addListener (this);

    // These fire on the audio thread (or wherever a parameter was changed): only set flags.
    processor.onSequenceTick = [this] { tickPending = true; dirty = true; };
    processor.onNoteOff = [this] { noteOffPending = true; };
    processor.onMidiLearned = [this] { dirty = true; };
    processor.onPlayPauseTriggered = [this] { dirty = true; };
    processor.onStopTriggered = [this] { dirty = true; };
    processor.onPresetCycleUp = [this] { presetCyclePending = 1; };
    processor.onPresetCycleDown = [this] { presetCyclePending = -1; };

    // The shift layer is gone from the UI; make sure a saved "shift on" can't redirect MIDI buttons.
    processor.getState().setIsShift (false);

    startTimerHz (60);
}

UiBridge::~UiBridge() {
    stopTimer();
    processor.onSequenceTick = nullptr;
    processor.onNoteOff = nullptr;
    processor.onMidiLearned = nullptr;
    processor.onPlayPauseTriggered = nullptr;
    processor.onStopTriggered = nullptr;
    processor.onPresetCycleUp = nullptr;
    processor.onPresetCycleDown = nullptr;

    processor.getInternalState().removeListener (this);
    for (auto* param : processor.getParameters())
        if (auto* withId = dynamic_cast<juce::RangedAudioParameter*> (param))
            processor.getAPVTS().removeParameterListener (withId->paramID, this);
}

void UiBridge::emit (const juce::Identifier& event, const juce::var& payload) const {
    if (onEvent) onEvent (event, payload);
}

void UiBridge::toast (const juce::String& text) const {
    emit ("toast", text);
}

void UiBridge::flush() {
    if (const int direction = presetCyclePending.exchange (0); direction != 0)
        cyclePreset (direction);

    auto& state = processor.getState();
    const bool ticked = tickPending.exchange (false);

    // Notes generated by Roll on the audio thread are written to the state here.
    state.applyPendingNoteWrites();

    if (ticked) {
        const int step = state.getCurrentIndex();
        auto* tick = new juce::DynamicObject();
        tick->setProperty ("step", step);
        tick->setProperty ("sub", step >= 0 ? state.step (step).effectiveCurrentSubstep() : 0);
        tick->setProperty ("note", step >= 0 ? state.getCurrentNoteForStep (step) : -1);
        tick->setProperty ("probTick", state.getLastProbabilityIndex());
        emit ("tick", juce::var (tick));
    }

    if (noteOffPending.exchange (false))
        emit ("noteOff", juce::var());

    if (dirty.exchange (false) && onEvent)
        emit ("state", buildSnapshot());
}

//==============================================================================
juce::var UiBridge::buildSnapshot() const {
    auto& state = processor.getState();
    const auto filter = state.scaleQuantizer().playbackFilter();
    auto* root = new juce::DynamicObject();

    root->setProperty ("length", state.getLength());
    root->setProperty ("selectedStep", state.getSelectedStepForEditing());
    root->setProperty ("currentStep", state.getCurrentIndex());
    root->setProperty ("page", state.getViewWindowStart() / 8);
    root->setProperty ("roll", state.getIsRandom());

    switch (processor.getSequencerState()) {
        case PluginProcessor::PlaybackState::Playing: root->setProperty ("transport", "playing"); break;
        case PluginProcessor::PlaybackState::Paused:  root->setProperty ("transport", "paused"); break;
        case PluginProcessor::PlaybackState::Stopped: root->setProperty ("transport", "stopped"); break;
    }

    juce::Array<juce::var> steps;
    for (int i = 0; i < ParamConstants::MAX_STEPS; ++i) {
        const Step s = state.step (i);
        auto* step = new juce::DynamicObject();
        step->setProperty ("count", s.substepCount());
        step->setProperty ("cur", s.currentSubstep());
        step->setProperty ("sel", juce::jlimit (0, s.substepCount() - 1, s.selectedSubstep()));
        juce::Array<juce::var> notes;
        for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; ++n)
            notes.add (filter.quantize (s.note (n)));
        step->setProperty ("notes", notes);
        steps.add (juce::var (step));
    }
    root->setProperty ("steps", steps);

    juce::Array<juce::var> scale;
    for (int pc = 0; pc < 12; ++pc) scale.add (state.getGlobalScaleNote (pc));
    root->setProperty ("scale", scale);

    juce::Array<juce::var> ranges;
    for (int i = 0; i < state.getNoteRangeCount(); ++i) {
        const auto r = state.getNoteRange (i);
        auto* range = new juce::DynamicObject();
        range->setProperty ("min", r.min);
        range->setProperty ("max", r.max);
        ranges.add (juce::var (range));
    }
    root->setProperty ("ranges", ranges);

    // Chance grid: 64 underlying cells, shown as dim x dim blocks.
    const int dim = state.getProbabilityGridDim();
    const int block = 8 / dim;
    juce::Array<juce::var> prob;
    for (int i = 0; i < 64; ++i) prob.add (state.getPlayProbability (i));
    int cellsOn = 0;
    for (int cell = 0; cell < dim * dim; ++cell)
        if (state.getPlayProbability ((cell / dim) * block * 8 + (cell % dim) * block)) ++cellsOn;
    root->setProperty ("prob", prob);
    root->setProperty ("probDim", dim);
    root->setProperty ("probPercent", juce::roundToInt (100.0 * cellsOn / (dim * dim)));
    root->setProperty ("probTick", state.getLastProbabilityIndex());

    juce::Array<juce::var> colours;
    for (int pc = 0; pc < 12; ++pc)
        colours.add ("#" + state.getNoteColour (pc).toDisplayString (false).toLowerCase());
    root->setProperty ("colours", colours);

    root->setProperty ("bpm", juce::roundToInt (state.getClockBpm()));
    root->setProperty ("subdivision", state.getClockSubdivision());
    root->setProperty ("clockMode", state.getClockMode());
    root->setProperty ("channel", state.getMidiOutputChannel());
    root->setProperty ("noteLength", state.getNoteLength());
    root->setProperty ("previewNotes", state.getPreviewNotesOnEdit());
    root->setProperty ("followStep", state.getFollowStep());
    root->setProperty ("latch", processor.getAPVTS().getRawParameterValue (P::SCALE_LATCH)->load() > 0.5f);
    root->setProperty ("bakeScale", state.getDestructiveScale());
    root->setProperty ("bakeRange", state.getDestructiveRange());

    const auto& presets = processor.getPresetManager();
    juce::Array<juce::var> presetNames;
    for (const auto& name : presets.getPresetNames()) presetNames.add (name);
    root->setProperty ("presets", presetNames);
    root->setProperty ("presetName", presets.getCurrentPresetName());

    const auto& registry = processor.getMidiLearnRegistry();
    auto* learn = new juce::DynamicObject();
    auto* mappings = new juce::DynamicObject();
    for (int c = 0; c < static_cast<int> (MidiControlId::COUNT); ++c) {
        const auto control = static_cast<MidiControlId> (c);
        if (const auto* midi = registry.getMidiForControl (control)) {
            const auto uiId = uiIdForControl (control);
            if (uiId.isNotEmpty()) mappings->setProperty (uiId, midi->toString());
        }
    }
    learn->setProperty ("active", registry.isLearning());
    const auto learning = uiIdForControl (registry.getLearningControl());
    learn->setProperty ("selected", learning.isEmpty() ? juce::var() : juce::var (learning));
    learn->setProperty ("mappings", juce::var (mappings));
    root->setProperty ("learn", juce::var (learn));

    root->setProperty ("canUndo", canUndo());
    return juce::var (root);
}

//==============================================================================
void UiBridge::handleAction (const juce::String& action, const juce::var& payload) {
    auto& state = processor.getState();
    const int step = intOf (payload, "step", -1);
    const int sub = intOf (payload, "sub", -1);
    dirty = true;

    if (action == "ready") {
        // nothing to do: the dirty flag above makes the next flush push a full state
    }
    // ---- selection and note editing
    else if (action == "selectStep" && validStep (step)) {
        state.setSelectedStepForEditing (step);
        if (boolOf (payload, "audition")) previewSelected();
    }
    else if (action == "selectSub" && validStep (step) && validSub (sub)) {
        state.setSelectedSubstepIndex (step, juce::jmin (sub, state.getStepNotesLength (step) - 1));
        if (boolOf (payload, "audition")) previewSelected();
    }
    else if (action == "jumpTo" && validStep (step)) {
        if (step < state.getLength()) state.setCurrentIndex (step);
    }
    else if (action == "shiftNote" && validStep (step) && validSub (sub)) {
        const int note = state.shiftStepNote (step, sub, intOf (payload, "delta"));
        if (note >= 0 && step == state.getSelectedStepForEditing()) previewSelected (note);
    }
    else if (action == "addSub" && validStep (step)) {
        pushUndo();
        if (state.addSubstep (step)) previewSelected();
    }
    else if (action == "removeSub" && validStep (step)) {
        pushUndo();
        state.removeSubstep (step, sub);
    }
    // ---- sequence
    else if (action == "setLength") {
        state.setLength (juce::jlimit (1, ParamConstants::MAX_STEPS, intOf (payload, "length", state.getLength())));
    }
    else if (action == "setPage") {
        state.setViewWindowStart (juce::jlimit (0, 7, intOf (payload, "page")) * 8);
    }
    else if (action == "roll") {
        setFloatParam (P::ROLL, boolOf (payload, "on") ? 1.0f : 0.0f);
    }
    else if (action == "nudge") {
        pushUndo();
        if (intOf (payload, "dir") < 0) state.nudgeLeft(); else state.nudgeRight();
    }
    else if (action == "duplicate") {
        pushUndo();
        pulseParam (P::DUPLICATE);
    }
    else if (action == "shuffle") {
        pushUndo();
        pulseParam (P::SHUFFLE);
    }
    else if (action == "randomize") {
        pushUndo();
        const bool all = payload["scope"].toString() == "all";
        pulseParam (all ? P::RANDOMIZE_ALL : P::RANDOMIZE_STEP);
        if (! all) previewSelected();
    }
    else if (action == "clear") {
        pushUndo();
        const auto scope = payload["scope"].toString();
        if (scope == "all") clearAll();
        else pulseParam (scope == "page" ? P::CLEAR_WINDOW : P::CLEAR_STEP);
    }
    else if (action == "undo") {
        undo();
    }
    // ---- scale, ranges, chance
    else if (action == "setScaleNote") {
        state.setGlobalScaleNote (intOf (payload, "pc"), boolOf (payload, "on"));
    }
    else if (action == "setRange") {
        state.setNoteRange (intOf (payload, "index"), intOf (payload, "min"), intOf (payload, "max", 127));
        if (state.getDestructiveRange()) state.bakeAllExistingNotes();
    }
    else if (action == "addRange") {
        state.addNoteRange();
        if (state.getDestructiveRange()) state.bakeAllExistingNotes();
    }
    else if (action == "removeRange") {
        state.removeNoteRange (intOf (payload, "index"));
        if (state.getDestructiveRange()) state.bakeAllExistingNotes();
    }
    else if (action == "setProbPercent") {
        state.setProbabilityPercentage (juce::jlimit (0, 100, intOf (payload, "percent", 100)));
    }
    else if (action == "probPaint") {
        paintProbabilityCell (intOf (payload, "cell", -1), boolOf (payload, "on"));
    }
    else if (action == "probReseed") {
        state.reseedProbabilityPattern();
    }
    else if (action == "probDivision") {
        state.setProbabilityDivisionLevel ((state.getProbabilityDivisionLevel() + 1) % 3);
    }
    // ---- transport and settings
    else if (action == "playPause") {
        processor.togglePlayPause();
    }
    else if (action == "stop") {
        processor.stopSequencer();
    }
    else if (action == "setParam") {
        setParam (payload["id"].toString(), payload["value"]);
    }
    else if (action == "setColour") {
        const auto hex = payload["colour"].toString().trimCharactersAtStart ("#");
        if (hex.length() == 6)
            state.setNoteColour (intOf (payload, "pc"), juce::Colour::fromString ("ff" + hex));
    }
    else if (action == "resetColours") {
        processor.getInternalState().resetNoteColours();
    }
    // ---- presets
    else if (action == "savePreset") {
        const auto name = payload["name"].toString().trim();
        auto& presets = processor.getPresetManager();
        if (auto xml = processor.getStateAsXml(); name.isNotEmpty() && xml && presets.savePreset (name, *xml)) {
            presets.setCurrentPresetName (name);
            presets.refreshPresetList();
            toast ("Saved \"" + name + "\"");
        } else {
            toast ("Could not save that preset");
        }
    }
    else if (action == "loadPreset") {
        loadPreset (payload["name"].toString());
    }
    else if (action == "deletePreset") {
        const auto name = payload["name"].toString();
        auto& presets = processor.getPresetManager();
        if (presets.deletePreset (name)) {
            presets.refreshPresetList();
            if (presets.getCurrentPresetName() == name) presets.setCurrentPresetName ({});
        }
    }
    else if (action == "cyclePreset") {
        cyclePreset (intOf (payload, "dir", 1));
    }
    // ---- MIDI learn
    else if (action == "learnToggle") {
        auto& registry = processor.getMidiLearnRegistry();
        if (registry.isLearning()) registry.exitLearnMode(); else registry.enterLearnMode();
    }
    else if (action == "learnSelect") {
        const auto control = controlForUiId (payload["id"].toString());
        if (control != MidiControlId::COUNT) processor.getMidiLearnRegistry().setLearningControl (control);
    }
    else if (action == "learnClear") {
        const auto control = controlForUiId (payload["id"].toString());
        if (control != MidiControlId::COUNT) processor.getMidiLearnRegistry().clearMapping (control);
    }
    else if (action == "exportMidi") {
        if (onExportRequested) onExportRequested();
    }
    else {
        DBG ("UiBridge: ignored action '" << action << "'");
    }
}

void UiBridge::setParam (const juce::String& id, const juce::var& value) {
    const auto number = static_cast<float> (static_cast<double> (value));
    const auto flag = static_cast<bool> (value) ? 1.0f : 0.0f;

    if (id == "bpm")               setFloatParam (P::CLOCK_BPM, number);
    else if (id == "subdivision")  setFloatParam (P::CLOCK_SUBDIVISION, number);
    else if (id == "clockMode")    setFloatParam (P::CLOCK_MODE, number);
    else if (id == "channel")      setFloatParam (P::MIDI_OUTPUT_CHANNEL, number);
    else if (id == "noteLength")   setFloatParam (P::PREF_NOTE_LENGTH, number);
    else if (id == "previewNotes") setFloatParam (P::PREF_PREVIEW_NOTES, flag);
    else if (id == "followStep")   setFloatParam (P::PREF_FOLLOW_STEP, flag);
    else if (id == "latch")        setFloatParam (P::SCALE_LATCH, flag);
    else if (id == "bakeScale")    setFloatParam (P::PREF_DESTRUCTIVE_SCALE, flag);
    else if (id == "bakeRange")    setFloatParam (P::PREF_DESTRUCTIVE_RANGE, flag);
}

/** Set a parameter from its real-world value (clamped to the parameter's range). */
void UiBridge::setFloatParam (const juce::String& paramId, float value) {
    if (auto* param = processor.getAPVTS().getParameter (paramId))
        param->setValueNotifyingHost (param->convertTo0to1 (param->getNormalisableRange().snapToLegalValue (value)));
}

/** Trigger parameters act on their rising edge; press and release in one go. */
void UiBridge::pulseParam (const juce::String& paramId) {
    setFloatParam (paramId, 1.0f);
    setFloatParam (paramId, 0.0f);
}

void UiBridge::previewSelected (int noteOverride) {
    if (processor.getState().getPreviewNotesOnEdit())
        processor.triggerCurrentStepMidi (noteOverride);
}

void UiBridge::clearAll() {
    auto& state = processor.getState();
    for (int i = 0; i < state.getLength(); ++i) {
        for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; ++n) state.setStepNote (i, n, -1);
        state.setStepNotesLength (i, 1);
        state.setSelectedSubstepIndex (i, 0);
    }
}

void UiBridge::paintProbabilityCell (int cell, bool on) {
    auto& state = processor.getState();
    const int dim = state.getProbabilityGridDim();
    if (cell < 0 || cell >= dim * dim) return;
    const int block = 8 / dim;
    const int top = (cell / dim) * block * 8 + (cell % dim) * block;
    for (int r = 0; r < block; ++r)
        for (int c = 0; c < block; ++c)
            state.setPlayProbability (top + r * 8 + c, on);
}

void UiBridge::loadPreset (const juce::String& name) {
    auto& presets = processor.getPresetManager();
    if (auto xml = presets.loadPreset (name)) {
        processor.loadStateFromXml (*xml);
        presets.setCurrentPresetName (name);
        processor.getState().setIsShift (false);
        undoStack.clear();
        dirty = true;
    }
}

void UiBridge::cyclePreset (int direction) {
    auto& presets = processor.getPresetManager();
    const auto names = presets.getPresetNames();
    if (names.isEmpty()) return;
    const int current = presets.getPresetIndex (presets.getCurrentPresetName());
    const int next = current < 0 ? 0 : (current + (direction < 0 ? -1 : 1) + names.size()) % names.size();
    loadPreset (names[next]);
}

//==============================================================================
void UiBridge::pushUndo() {
    auto& state = processor.getState();
    UndoEntry entry;
    entry.length = state.getLength();
    for (int i = 0; i < ParamConstants::MAX_STEPS; ++i) {
        auto& step = entry.steps[static_cast<size_t> (i)];
        step.count = state.getStepNotesLength (i);
        for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; ++n)
            step.notes[static_cast<size_t> (n)] = state.getStepNote (i, n);
    }
    if (undoStack.size() >= maxUndoDepth) undoStack.erase (undoStack.begin());
    undoStack.push_back (entry);
}

void UiBridge::undo() {
    if (undoStack.empty()) return;
    auto& state = processor.getState();
    const auto& entry = undoStack.back();
    for (int i = 0; i < ParamConstants::MAX_STEPS; ++i) {
        const auto& step = entry.steps[static_cast<size_t> (i)];
        for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; ++n)
            state.setStepNote (i, n, step.notes[static_cast<size_t> (n)]);
        state.setStepNotesLength (i, step.count);
        state.setSelectedSubstepIndex (i, juce::jlimit (0, step.count - 1, state.getSelectedSubstepIndex (i)));
    }
    state.setLength (entry.length);
    undoStack.pop_back();
}
