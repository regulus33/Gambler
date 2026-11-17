#include "InternalState.h"

InternalState::InternalState()
    : tree(IDs::ROOT_TYPE)
{
    tree.addListener(this);
    initializeDefaults();
}

InternalState::~InternalState() {
    tree.removeListener(this);
}

// ========================================
// LISTENER MANAGEMENT
// ========================================

void InternalState::addListener(Listener* listener) {
    listeners.add(listener);
}

void InternalState::removeListener(Listener* listener) {
    listeners.remove(listener);
}

void InternalState::notifyListeners() {
    listeners.call([](Listener& l) { l.internalStateChanged(); });
}

void InternalState::valueTreePropertyChanged(juce::ValueTree& /*treeChanged*/, const juce::Identifier& /*property*/) {
    notifyListeners();
}

// ========================================
// STEP NOTES
// ========================================

int InternalState::getStepNote(int stepIndex, int substepIndex) const {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return ParamConstants::DEFAULT_NOTE;
    if (substepIndex < 0 || substepIndex >= ParamConstants::MAX_SUBSTEPS) return ParamConstants::DEFAULT_NOTE;
    return tree.getProperty(IDs::stepNote(stepIndex, substepIndex), ParamConstants::DEFAULT_NOTE);
}

void InternalState::setStepNote(int stepIndex, int substepIndex, int noteNumber) {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
    if (substepIndex < 0 || substepIndex >= ParamConstants::MAX_SUBSTEPS) return;
    tree.setProperty(IDs::stepNote(stepIndex, substepIndex),
                     juce::jlimit(-1, 127, noteNumber), nullptr);
}

int InternalState::getStepNotesLength(int stepIndex) const {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return 1;
    // Clamp on read too: setState() swallows whole trees from preset XML,
    // and a 0 here reaches (x + 1) % count on the audio thread.
    return juce::jlimit(1, ParamConstants::MAX_SUBSTEPS,
                        static_cast<int>(tree.getProperty(IDs::stepNotesLength(stepIndex), 1)));
}

void InternalState::setStepNotesLength(int stepIndex, int length) {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
    tree.setProperty(IDs::stepNotesLength(stepIndex),
                     juce::jlimit(1, ParamConstants::MAX_SUBSTEPS, length), nullptr);
}

int InternalState::getStepNotesCurrentIndex(int stepIndex) const {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return -1;
    return tree.getProperty(IDs::stepNotesCurrentIndex(stepIndex), -1);
}

void InternalState::setStepNotesCurrentIndex(int stepIndex, int index) {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
    tree.setProperty(IDs::stepNotesCurrentIndex(stepIndex),
                     juce::jlimit(-1, ParamConstants::MAX_SUBSTEPS - 1, index), nullptr);
}

int InternalState::getSelectedSubstepIndex(int stepIndex) const {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return 0;
    return tree.getProperty(IDs::selectedSubstepIndex(stepIndex), 0);
}

void InternalState::setSelectedSubstepIndex(int stepIndex, int index) {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
    tree.setProperty(IDs::selectedSubstepIndex(stepIndex),
                     juce::jlimit(0, ParamConstants::MAX_SUBSTEPS - 1, index), nullptr);
}

// ========================================
// STEP MODULATIONS
// ========================================

int InternalState::getStepModType(int stepIndex, int modIndex) const {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return 0;
    if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return 0;
    return tree.getProperty(IDs::stepModType(stepIndex, modIndex), 0);
}

void InternalState::setStepModType(int stepIndex, int modIndex, int type) {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
    if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return;
    tree.setProperty(IDs::stepModType(stepIndex, modIndex), type, nullptr);
}

int InternalState::getStepModCC(int stepIndex, int modIndex) const {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return 1;
    if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return 1;
    return tree.getProperty(IDs::stepModCC(stepIndex, modIndex), 1);
}

void InternalState::setStepModCC(int stepIndex, int modIndex, int cc) {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
    if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return;
    tree.setProperty(IDs::stepModCC(stepIndex, modIndex), juce::jlimit(0, 127, cc), nullptr);
}

int InternalState::getStepModValue(int stepIndex, int modIndex) const {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return 64;
    if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return 64;
    return tree.getProperty(IDs::stepModValue(stepIndex, modIndex), 64);
}

void InternalState::setStepModValue(int stepIndex, int modIndex, int value) {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
    if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return;
    tree.setProperty(IDs::stepModValue(stepIndex, modIndex), juce::jlimit(0, 127, value), nullptr);
}

bool InternalState::getStepModSelected(int stepIndex, int modIndex) const {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return false;
    if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return false;
    return tree.getProperty(IDs::stepModSelected(stepIndex, modIndex), false);
}

void InternalState::setStepModSelected(int stepIndex, int modIndex, bool selected) {
    if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
    if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return;
    tree.setProperty(IDs::stepModSelected(stepIndex, modIndex), selected, nullptr);
}

// ========================================
// SEQUENCE METADATA
// ========================================

int InternalState::getCurrentIndex() const {
    return tree.getProperty(IDs::SEQ_CURRENT_INDEX, -1);
}

void InternalState::setCurrentIndex(int index) {
    tree.setProperty(IDs::SEQ_CURRENT_INDEX, index, nullptr);
}

int InternalState::getLastProbabilityIndex() const {
    return tree.getProperty(IDs::SEQ_LAST_PROB_INDEX, -1);
}

void InternalState::setLastProbabilityIndex(int index) {
    tree.setProperty(IDs::SEQ_LAST_PROB_INDEX, index, nullptr);
}

int InternalState::getViewWindowStart() const {
    return tree.getProperty(IDs::SEQ_VIEW_WINDOW_START, 0);
}

void InternalState::setViewWindowStart(int start) {
    tree.setProperty(IDs::SEQ_VIEW_WINDOW_START, juce::jlimit(0, 56, start), nullptr);
}

// ========================================
// PLAY PROBABILITY
// ========================================

bool InternalState::getPlayProbability(int index) const {
    if (index < 0 || index >= ParamConstants::MAX_STEPS) return true;
    return tree.getProperty(IDs::playProbability(index), true);
}

void InternalState::setPlayProbability(int index, bool enabled) {
    if (index < 0 || index >= ParamConstants::MAX_STEPS) return;
    tree.setProperty(IDs::playProbability(index), enabled, nullptr);
}

int InternalState::getProbabilityDivisionLevel() const {
    return tree.getProperty(IDs::PROB_DIVISION_LEVEL, 0);
}

void InternalState::setProbabilityDivisionLevel(int level) {
    tree.setProperty(IDs::PROB_DIVISION_LEVEL, juce::jlimit(0, 2, level), nullptr);
}

// ========================================
// NOTE COLOURS
// ========================================

juce::Colour InternalState::getNoteColour(int chromaticNote) const {
    int index = chromaticNote % 12;
    if (index < 0) index += 12;
    int argb = tree.getProperty(IDs::noteColour(index), defaultNoteColours[index]);
    return juce::Colour(static_cast<juce::uint32>(argb));
}

void InternalState::setNoteColour(int chromaticNote, juce::Colour colour) {
    int index = chromaticNote % 12;
    if (index < 0) index += 12;
    tree.setProperty(IDs::noteColour(index), static_cast<int>(colour.getARGB()), nullptr);
}

void InternalState::resetNoteColours() {
    for (int i = 0; i < 12; ++i)
        tree.setProperty(IDs::noteColour(i), defaultNoteColours[i], nullptr);
}

// ========================================
// SERIALIZATION
// ========================================

juce::ValueTree InternalState::getState() const {
    return tree.createCopy();
}

void InternalState::setState(const juce::ValueTree& state) {
    if (state.hasType(IDs::ROOT_TYPE)) {
        tree.copyPropertiesFrom(state, nullptr);
    }
}

// ========================================
// INITIALIZATION
// ========================================

void InternalState::initializeDefaults() {
    // Initialize step notes
    for (int step = 0; step < ParamConstants::MAX_STEPS; ++step) {
        for (int substep = 0; substep < ParamConstants::MAX_SUBSTEPS; ++substep) {
            tree.setProperty(IDs::stepNote(step, substep), ParamConstants::DEFAULT_NOTE, nullptr);
        }
        tree.setProperty(IDs::stepNotesLength(step), 1, nullptr);
        tree.setProperty(IDs::stepNotesCurrentIndex(step), -1, nullptr);
    }

    // Initialize step modulations
    for (int step = 0; step < ParamConstants::MAX_STEPS; ++step) {
        for (int mod = 0; mod < ParamConstants::NUM_MODULATIONS; ++mod) {
            tree.setProperty(IDs::stepModType(step, mod), 0, nullptr);
            tree.setProperty(IDs::stepModCC(step, mod), 1, nullptr);
            tree.setProperty(IDs::stepModValue(step, mod), 64, nullptr);
            tree.setProperty(IDs::stepModSelected(step, mod), false, nullptr);
        }
    }

    // Initialize sequence metadata
    tree.setProperty(IDs::SEQ_CURRENT_INDEX, -1, nullptr);
    tree.setProperty(IDs::SEQ_LAST_PROB_INDEX, -1, nullptr);
    tree.setProperty(IDs::SEQ_VIEW_WINDOW_START, 0, nullptr);

    // Initialize play probability (all true = 100%)
    for (int i = 0; i < ParamConstants::MAX_STEPS; ++i) {
        tree.setProperty(IDs::playProbability(i), true, nullptr);
    }
    tree.setProperty(IDs::PROB_DIVISION_LEVEL, 0, nullptr);

    // Initialize note colours
    for (int i = 0; i < 12; ++i) {
        tree.setProperty(IDs::noteColour(i), defaultNoteColours[i], nullptr);
    }
}

