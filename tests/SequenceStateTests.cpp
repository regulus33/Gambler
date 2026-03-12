#include <catch2/catch_test_macros.hpp>
#include "../source/PluginProcessor.h"

/**
 * SequenceState Integration Tests
 *
 * These tests verify SequenceState behavior using a real PluginProcessor
 * to provide the APVTS backing store. This ensures the state accessors
 * work correctly with real APVTS parameters.
 */

// Helper to create a PluginProcessor and get its SequenceState
class SequenceStateFixture {
public:
    SequenceStateFixture() {}

    PluginProcessor& getProcessor() { return processor; }
    SequenceState& getState() { return processor.getState(); }

private:
    PluginProcessor processor;
};

// ============================================================================
// Sequence Metadata Tests
// ============================================================================

TEST_CASE("SequenceState sequence metadata") {
    SequenceStateFixture fixture;
    auto& state = fixture.getState();

    SECTION("Default length is 8") {
        REQUIRE(state.getLength() == 8);
    }

    SECTION("Default current index is -1") {
        REQUIRE(state.getCurrentIndex() == -1);
    }

    SECTION("Default selected step is 0") {
        REQUIRE(state.getSelectedStepForEditing() == 0);
    }

    SECTION("Default isRandom is false") {
        REQUIRE(state.getIsRandom() == false);
    }

    SECTION("Default isShift is false") {
        REQUIRE(state.getIsShift() == false);
    }

    SECTION("setLength() updates length") {
        state.setLength(16);
        REQUIRE(state.getLength() == 16);
    }

    SECTION("setLength() clamps to valid range") {
        state.setLength(0);
        REQUIRE(state.getLength() == 1);  // Minimum is 1

        state.setLength(100);
        REQUIRE(state.getLength() == 64);  // Maximum is MAX_STEPS
    }

    SECTION("setCurrentIndex() updates index") {
        state.setCurrentIndex(5);
        REQUIRE(state.getCurrentIndex() == 5);
    }

    SECTION("setCurrentIndex() clamps to length-1") {
        state.setLength(8);
        state.setCurrentIndex(10);
        REQUIRE(state.getCurrentIndex() <= 7);
    }

    SECTION("setSelectedStepForEditing() updates selection") {
        state.setSelectedStepForEditing(3);
        REQUIRE(state.getSelectedStepForEditing() == 3);
    }

    SECTION("setIsRandom() toggles random mode") {
        state.setIsRandom(true);
        REQUIRE(state.getIsRandom() == true);

        state.setIsRandom(false);
        REQUIRE(state.getIsRandom() == false);
    }

    SECTION("setIsShift() toggles shift mode") {
        state.setIsShift(true);
        REQUIRE(state.getIsShift() == true);

        state.setIsShift(false);
        REQUIRE(state.getIsShift() == false);
    }

    SECTION("viewWindowStart defaults to 0 and is settable") {
        REQUIRE(state.getViewWindowStart() == 0);
        state.setViewWindowStart(8);
        REQUIRE(state.getViewWindowStart() == 8);
    }

    SECTION("viewWindowStart clamps to valid range") {
        state.setViewWindowStart(-5);
        REQUIRE(state.getViewWindowStart() == 0);

        state.setViewWindowStart(100);
        REQUIRE(state.getViewWindowStart() == 56);  // Max is 56 (64-8)
    }
}

// ============================================================================
// Step Notes Tests
// ============================================================================

TEST_CASE("SequenceState step notes") {
    SequenceStateFixture fixture;
    auto& state = fixture.getState();

    SECTION("Default step note is 60 (middle C)") {
        REQUIRE(state.getStepNote(0, 0) == 60);
    }

    SECTION("setStepNote() updates note") {
        state.setStepNote(0, 0, 72);
        REQUIRE(state.getStepNote(0, 0) == 72);
    }

    SECTION("setStepNote() clamps to valid MIDI range") {
        state.setStepNote(0, 0, 200);
        REQUIRE(state.getStepNote(0, 0) == 127);

        state.setStepNote(0, 0, -5);
        REQUIRE(state.getStepNote(0, 0) == -1);  // -1 is rest
    }

    SECTION("setStepNote() with out-of-range step index does nothing") {
        state.setStepNote(-1, 0, 72);  // Should not crash
        state.setStepNote(64, 0, 72);  // Should not crash
        REQUIRE(state.getStepNote(0, 0) == 60);  // Default unchanged
    }

    SECTION("setStepNote() with out-of-range substep index does nothing") {
        state.setStepNote(0, -1, 72);  // Should not crash
        state.setStepNote(0, 8, 72);   // Should not crash
        REQUIRE(state.getStepNote(0, 0) == 60);  // Default unchanged
    }

    SECTION("getStepNote() with out-of-range returns default") {
        REQUIRE(state.getStepNote(-1, 0) == ParamConstants::DEFAULT_NOTE);
        REQUIRE(state.getStepNote(64, 0) == ParamConstants::DEFAULT_NOTE);
        REQUIRE(state.getStepNote(0, -1) == ParamConstants::DEFAULT_NOTE);
        REQUIRE(state.getStepNote(0, 8) == ParamConstants::DEFAULT_NOTE);
    }

    SECTION("stepHasRest() returns true for rest notes") {
        state.setStepNote(0, 0, -1);
        state.setStepNotesCurrentIndex(0, 0);
        REQUIRE(state.stepHasRest(0) == true);
    }

    SECTION("stepHasRest() returns false for normal notes") {
        state.setStepNote(0, 0, 60);
        state.setStepNotesCurrentIndex(0, 0);
        REQUIRE(state.stepHasRest(0) == false);
    }

    SECTION("Step substep length defaults to 1") {
        REQUIRE(state.getStepNotesLength(0) == 1);
    }

    SECTION("setStepNotesLength() clamps to valid range") {
        state.setStepNotesLength(0, 4);
        REQUIRE(state.getStepNotesLength(0) == 4);

        state.setStepNotesLength(0, 0);
        REQUIRE(state.getStepNotesLength(0) == 1);  // Min is 1

        state.setStepNotesLength(0, 100);
        REQUIRE(state.getStepNotesLength(0) == 8);  // Max is MAX_SUBSTEPS
    }

    SECTION("Multiple substeps can have different notes") {
        state.setStepNote(0, 0, 60);
        state.setStepNote(0, 1, 64);
        state.setStepNote(0, 2, 67);
        state.setStepNotesLength(0, 3);

        REQUIRE(state.getStepNote(0, 0) == 60);
        REQUIRE(state.getStepNote(0, 1) == 64);
        REQUIRE(state.getStepNote(0, 2) == 67);
    }
}

// ============================================================================
// Play Probability Tests
// ============================================================================

TEST_CASE("SequenceState play probability") {
    SequenceStateFixture fixture;
    auto& state = fixture.getState();

    SECTION("setPlayProbability() and getPlayProbability()") {
        state.setPlayProbability(0, false);
        REQUIRE(state.getPlayProbability(0) == false);

        state.setPlayProbability(0, true);
        REQUIRE(state.getPlayProbability(0) == true);
    }

    SECTION("getPlayProbability() out-of-range returns true") {
        REQUIRE(state.getPlayProbability(-1) == true);
        REQUIRE(state.getPlayProbability(64) == true);
    }

    SECTION("setPlayProbability() out-of-range does nothing") {
        state.setPlayProbability(-1, false);  // Should not crash
        state.setPlayProbability(64, false);  // Should not crash
    }

    SECTION("setProbabilityPercentage() sets percentage of steps") {
        state.setProbabilityPercentage(100);
        int enabledCount = 0;
        for (int i = 0; i < ParamConstants::MAX_STEPS; i++) {
            if (state.getPlayProbability(i)) enabledCount++;
        }
        REQUIRE(enabledCount == ParamConstants::MAX_STEPS);

        state.setProbabilityPercentage(0);
        enabledCount = 0;
        for (int i = 0; i < ParamConstants::MAX_STEPS; i++) {
            if (state.getPlayProbability(i)) enabledCount++;
        }
        REQUIRE(enabledCount == 0);
    }

    SECTION("setProbabilityPercentage() clamps input") {
        // Should not crash with out-of-range values
        state.setProbabilityPercentage(-10);
        state.setProbabilityPercentage(200);
    }
}

// ============================================================================
// Sequence Operations Tests
// ============================================================================

TEST_CASE("SequenceState increaseLength and decreaseLength") {
    SequenceStateFixture fixture;
    auto& state = fixture.getState();

    SECTION("increaseLength() adds one step") {
        int initialLength = state.getLength();
        state.increaseLength();
        REQUIRE(state.getLength() == initialLength + 1);
    }

    SECTION("increaseLength() stops at MAX_STEPS") {
        state.setLength(ParamConstants::MAX_STEPS);
        state.increaseLength();
        REQUIRE(state.getLength() == ParamConstants::MAX_STEPS);
    }

    SECTION("decreaseLength() removes one step") {
        int initialLength = state.getLength();
        state.decreaseLength();
        REQUIRE(state.getLength() == initialLength - 1);
    }

    SECTION("decreaseLength() stops at 1") {
        state.setLength(1);
        state.decreaseLength();
        REQUIRE(state.getLength() == 1);
    }

    SECTION("decreaseLength() wraps currentIndex if needed") {
        state.setLength(4);
        state.setCurrentIndex(3);
        state.decreaseLength();
        REQUIRE(state.getCurrentIndex() <= state.getLength() - 1);
    }
}

TEST_CASE("SequenceState nudge operations") {
    SequenceStateFixture fixture;
    auto& state = fixture.getState();

    SECTION("nudgeLeft() rotates notes left") {
        state.setLength(4);
        state.setStepNote(0, 0, 60);
        state.setStepNote(1, 0, 64);
        state.setStepNote(2, 0, 67);
        state.setStepNote(3, 0, 72);

        state.nudgeLeft();

        // After left rotation: [64, 67, 72, 60]
        REQUIRE(state.getStepNote(0, 0) == 64);
        REQUIRE(state.getStepNote(1, 0) == 67);
        REQUIRE(state.getStepNote(2, 0) == 72);
        REQUIRE(state.getStepNote(3, 0) == 60);
    }

    SECTION("nudgeRight() rotates notes right") {
        state.setLength(4);
        state.setStepNote(0, 0, 60);
        state.setStepNote(1, 0, 64);
        state.setStepNote(2, 0, 67);
        state.setStepNote(3, 0, 72);

        state.nudgeRight();

        // After right rotation: [72, 60, 64, 67]
        REQUIRE(state.getStepNote(0, 0) == 72);
        REQUIRE(state.getStepNote(1, 0) == 60);
        REQUIRE(state.getStepNote(2, 0) == 64);
        REQUIRE(state.getStepNote(3, 0) == 67);
    }

    SECTION("nudgeLeft() with length 1 does nothing") {
        state.setLength(1);
        state.setStepNote(0, 0, 60);
        state.nudgeLeft();
        REQUIRE(state.getStepNote(0, 0) == 60);
    }

    SECTION("nudgeRight() with length 1 does nothing") {
        state.setLength(1);
        state.setStepNote(0, 0, 60);
        state.nudgeRight();
        REQUIRE(state.getStepNote(0, 0) == 60);
    }

    SECTION("nudgeLeft() and nudgeRight() are inverse operations") {
        state.setLength(4);
        state.setStepNote(0, 0, 60);
        state.setStepNote(1, 0, 64);
        state.setStepNote(2, 0, 67);
        state.setStepNote(3, 0, 72);

        state.nudgeLeft();
        state.nudgeRight();

        REQUIRE(state.getStepNote(0, 0) == 60);
        REQUIRE(state.getStepNote(1, 0) == 64);
        REQUIRE(state.getStepNote(2, 0) == 67);
        REQUIRE(state.getStepNote(3, 0) == 72);
    }
}

TEST_CASE("SequenceState tick") {
    SequenceStateFixture fixture;
    auto& state = fixture.getState();

    SECTION("tick() advances current index") {
        state.setLength(4);
        state.setCurrentIndex(-1);
        state.setIsRandom(false);

        state.tick();
        REQUIRE(state.getCurrentIndex() == 0);

        state.tick();
        REQUIRE(state.getCurrentIndex() == 1);
    }

    SECTION("tick() wraps at length boundary") {
        state.setLength(4);
        state.setCurrentIndex(2);
        state.setIsRandom(false);

        state.tick();
        REQUIRE(state.getCurrentIndex() == 3);

        state.tick();
        REQUIRE(state.getCurrentIndex() == 0);  // Wrapped
    }

    SECTION("tick() returns note for current step") {
        state.setLength(4);
        state.setCurrentIndex(-1);
        state.setIsRandom(false);
        state.setStepNote(0, 0, 60);
        state.setStepNotesCurrentIndex(0, -1);

        int note = state.tick();
        // Note should be from step 0 (which is 60)
        // After quantization through per-step scale, it should still be 60
        REQUIRE(note == 60);
    }

    SECTION("tick() in random mode returns -1 or a note") {
        state.setLength(4);
        state.setCurrentIndex(-1);
        state.setIsRandom(true);

        // With default probability (all true initially), tick should return a note
        // Set all probabilities to true
        for (int i = 0; i < ParamConstants::MAX_STEPS; i++) {
            state.setPlayProbability(i, true);
        }

        int note = state.tick();
        // In random mode, returns a random note (>= 0) or -1 (rest)
        REQUIRE(note >= -1);
        REQUIRE(note <= 127);
    }

    SECTION("reset() resets current index and substep indices") {
        state.setLength(4);
        state.setCurrentIndex(3);

        state.reset();

        REQUIRE(state.getCurrentIndex() == -1);
    }
}

// ============================================================================
// Global Scale Tests
// ============================================================================

TEST_CASE("SequenceState global scale") {
    SequenceStateFixture fixture;
    auto& state = fixture.getState();

    SECTION("Default: all scale notes enabled") {
        for (int note = 0; note < 12; note++) {
            REQUIRE(state.getGlobalScaleNote(note) == true);
        }
    }

    SECTION("setGlobalScaleNote() disables a note") {
        state.setGlobalScaleNote(1, false);  // Disable Db
        REQUIRE(state.getGlobalScaleNote(1) == false);
        REQUIRE(state.getGlobalScaleNote(0) == true);  // C still enabled
    }

    SECTION("getGlobalScaleNotes() returns all 12 note states") {
        auto notes = state.getGlobalScaleNotes();
        for (int i = 0; i < 12; i++) {
            REQUIRE(notes[i] == 1);
        }
    }

    SECTION("setGlobalScaleNotes() sets all 12 notes at once") {
        std::array<int, 12> cMajor = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};
        state.setGlobalScaleNotes(cMajor);

        auto notes = state.getGlobalScaleNotes();
        for (int i = 0; i < 12; i++) {
            REQUIRE(notes[i] == cMajor[i]);
        }
    }

    SECTION("isNoteInGlobalScale() checks note against global scale") {
        std::array<int, 12> cMajor = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};
        state.setGlobalScaleNotes(cMajor);

        REQUIRE(state.isNoteInGlobalScale(60) == true);   // C4 in C major
        REQUIRE(state.isNoteInGlobalScale(61) == false);  // C#4 not in C major
        REQUIRE(state.isNoteInGlobalScale(62) == true);   // D4 in C major
    }

    SECTION("quantizeNoteToGlobalScale() quantizes to nearest scale note") {
        std::array<int, 12> cMajor = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};
        state.setGlobalScaleNotes(cMajor);

        REQUIRE(state.quantizeNoteToGlobalScale(60) == 60);  // C stays C
        REQUIRE(state.quantizeNoteToGlobalScale(61) == 60);  // C# -> C (nearest down)
        REQUIRE(state.quantizeNoteToGlobalScale(66) == 65);  // Gb -> F (equidistant, prefers down)
    }

    SECTION("quantizeNoteToGlobalScale() with no enabled notes returns -1") {
        std::array<int, 12> noNotes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        state.setGlobalScaleNotes(noNotes);

        REQUIRE(state.quantizeNoteToGlobalScale(60) == -1);
    }

    SECTION("getGlobalScale() returns a Scale object") {
        std::array<int, 12> cMajor = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};
        state.setGlobalScaleNotes(cMajor);

        Scale scale = state.getGlobalScale();
        auto notes = scale.getScaleNotes();
        for (int i = 0; i < 12; i++) {
            REQUIRE(notes[i] == cMajor[i]);
        }
    }

    SECTION("Boundary: getGlobalScaleNote() with invalid note returns true") {
        REQUIRE(state.getGlobalScaleNote(-1) == true);
        REQUIRE(state.getGlobalScaleNote(12) == true);
    }
}

// ============================================================================
// Step Modulations Tests
// ============================================================================

TEST_CASE("SequenceState step modulations") {
    SequenceStateFixture fixture;
    auto& state = fixture.getState();

    SECTION("Default modulation type for slot 0 is Velocity") {
        // First modulation slot defaults to Velocity, others to CC
        REQUIRE(state.getStepModType(0, 0) == ModulationType::Velocity);
    }

    SECTION("Default modulation type for slot 1+ is CC") {
        REQUIRE(state.getStepModType(0, 1) == ModulationType::CC);
    }

    SECTION("setStepModType() updates type") {
        state.setStepModType(0, 0, ModulationType::Velocity);
        REQUIRE(state.getStepModType(0, 0) == ModulationType::Velocity);
    }

    SECTION("setStepModValue() and getStepModValue()") {
        state.setStepModValue(0, 0, 100);
        REQUIRE(state.getStepModValue(0, 0) == 100);
    }

    SECTION("setStepModValue() clamps to 0-127") {
        state.setStepModValue(0, 0, 200);
        REQUIRE(state.getStepModValue(0, 0) == 127);

        state.setStepModValue(0, 0, -5);
        REQUIRE(state.getStepModValue(0, 0) == 0);
    }

    SECTION("setStepModCCNum() and getStepModCCNum()") {
        state.setStepModCCNum(0, 0, 74);
        REQUIRE(state.getStepModCCNum(0, 0) == 74);
    }

    SECTION("setStepModSelected() and getStepModSelected()") {
        state.setStepModSelected(0, 0, true);
        REQUIRE(state.getStepModSelected(0, 0) == true);

        state.setStepModSelected(0, 0, false);
        REQUIRE(state.getStepModSelected(0, 0) == false);
    }

    SECTION("getSelectedModulations() returns only selected mods") {
        state.setStepModSelected(0, 0, true);
        state.setStepModSelected(0, 1, false);
        state.setStepModSelected(0, 2, true);
        state.setStepModValue(0, 0, 100);
        state.setStepModValue(0, 2, 50);

        auto mods = state.getSelectedModulations(0);
        REQUIRE(mods.size() == 2);
        REQUIRE(mods[0].value == 100);
        REQUIRE(mods[1].value == 50);
    }

    SECTION("Boundary: out-of-range step returns defaults") {
        REQUIRE(state.getStepModType(-1, 0) == ModulationType::CC);
        REQUIRE(state.getStepModValue(-1, 0) == 64);
        REQUIRE(state.getStepModCCNum(-1, 0) == 0);
        REQUIRE(state.getStepModSelected(-1, 0) == false);
    }

    SECTION("Boundary: out-of-range mod index returns defaults") {
        REQUIRE(state.getStepModType(0, -1) == ModulationType::CC);
        REQUIRE(state.getStepModType(0, 7) == ModulationType::CC);
    }
}

// ============================================================================
// Clock State Tests
// ============================================================================

TEST_CASE("SequenceState clock state") {
    SequenceStateFixture fixture;
    auto& state = fixture.getState();

    SECTION("Clock BPM defaults and setter") {
        state.setClockBpm(140.0f);
        REQUIRE(state.getClockBpm() == 140.0f);
    }

    SECTION("Clock BPM clamps to valid range") {
        state.setClockBpm(10.0f);
        REQUIRE(state.getClockBpm() == 20.0f);

        state.setClockBpm(2000.0f);
        REQUIRE(state.getClockBpm() == 999.0f);
    }

    SECTION("Clock mode defaults and setter") {
        state.setClockMode(1);
        REQUIRE(state.getClockMode() == 1);

        state.setClockMode(0);
        REQUIRE(state.getClockMode() == 0);
    }

    SECTION("Clock subdivision defaults and setter") {
        state.setClockSubdivision(2);
        REQUIRE(state.getClockSubdivision() == 2);
    }

    SECTION("Clock subdivision clamps to valid range") {
        state.setClockSubdivision(-1);
        REQUIRE(state.getClockSubdivision() == 0);

        state.setClockSubdivision(10);
        REQUIRE(state.getClockSubdivision() == 3);
    }
}

// ============================================================================
// User Preferences Tests
// ============================================================================

TEST_CASE("SequenceState user preferences") {
    SequenceStateFixture fixture;
    auto& state = fixture.getState();

    SECTION("previewNotesOnEdit default and setter") {
        state.setPreviewNotesOnEdit(true);
        REQUIRE(state.getPreviewNotesOnEdit() == true);

        state.setPreviewNotesOnEdit(false);
        REQUIRE(state.getPreviewNotesOnEdit() == false);
    }

    SECTION("followStep default and setter") {
        state.setFollowStep(true);
        REQUIRE(state.getFollowStep() == true);

        state.setFollowStep(false);
        REQUIRE(state.getFollowStep() == false);
    }

    SECTION("noteLength default and setter") {
        state.setNoteLength(0.5f);
        REQUIRE(state.getNoteLength() == 0.5f);
    }

    SECTION("noteLength clamps to SequenceState range (0.0 - 1.0)") {
        state.setNoteLength(0.1f);
        REQUIRE(state.getNoteLength() >= 0.1f);

        // SequenceState clamps to 0.0-1.0 before APVTS clamp
        state.setNoteLength(2.0f);
        REQUIRE(state.getNoteLength() <= 1.0f);

        state.setNoteLength(-0.5f);
        REQUIRE(state.getNoteLength() >= 0.0f);
    }
}

// ============================================================================
// Scale Window Tests
// ============================================================================

TEST_CASE("SequenceState scale window") {
    SequenceStateFixture fixture;
    auto& state = fixture.getState();

    SECTION("setScaleWindowOffset and getScaleWindowOffset") {
        state.setScaleWindowOffset(24);
        REQUIRE(state.getScaleWindowOffset() == 24);
    }

    SECTION("setScaleWindowSize and getScaleWindowSize") {
        state.setScaleWindowSize(48);
        REQUIRE(state.getScaleWindowSize() == 48);
    }

    SECTION("Scale window offset clamps to valid range") {
        state.setScaleWindowOffset(-5);
        REQUIRE(state.getScaleWindowOffset() == 0);

        state.setScaleWindowOffset(200);
        REQUIRE(state.getScaleWindowOffset() == 127);
    }

    SECTION("Scale window size clamps to valid range") {
        state.setScaleWindowSize(0);
        REQUIRE(state.getScaleWindowSize() == 1);

        state.setScaleWindowSize(200);
        REQUIRE(state.getScaleWindowSize() == 128);
    }
}
