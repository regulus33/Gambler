#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../source/backend/MidiMapping.h"

/**
 * MIDI Mapping Tests
 *
 * Tests for the MIDI mapping system including:
 * - MidiValueConverter utilities
 * - MidiMapping data structures
 * - MidiControlId name lookup
 */

// ============================================================================
// MidiValueConverter Tests
// ============================================================================

SCENARIO("MidiValueConverter normalized conversion", "[midimapping]") {
    GIVEN("MIDI values") {
        WHEN("Converting minimum MIDI value (0)") {
            float normalized = MidiValueConverter::midiToNormalized(0);

            THEN("Result is 0.0") {
                REQUIRE(normalized == Catch::Approx(0.0f));
            }
        }

        WHEN("Converting maximum MIDI value (127)") {
            float normalized = MidiValueConverter::midiToNormalized(127);

            THEN("Result is 1.0") {
                REQUIRE(normalized == Catch::Approx(1.0f));
            }
        }

        WHEN("Converting mid-range MIDI value (64)") {
            float normalized = MidiValueConverter::midiToNormalized(64);

            THEN("Result is approximately 0.5") {
                REQUIRE(normalized == Catch::Approx(0.504f).epsilon(0.01f));
            }
        }

        WHEN("Converting with invert flag") {
            float normalized0 = MidiValueConverter::midiToNormalized(0, true);
            float normalized127 = MidiValueConverter::midiToNormalized(127, true);

            THEN("Values are inverted") {
                REQUIRE(normalized0 == Catch::Approx(1.0f));
                REQUIRE(normalized127 == Catch::Approx(0.0f));
            }
        }
    }
}

SCENARIO("MidiValueConverter scaled conversion", "[midimapping]") {
    GIVEN("MIDI values and scale ranges") {
        WHEN("Scaling 0-127 to 0.0-1.0 range") {
            float scaled0 = MidiValueConverter::midiToScaled(0, 0.0f, 1.0f);
            float scaled127 = MidiValueConverter::midiToScaled(127, 0.0f, 1.0f);

            THEN("Values map to range") {
                REQUIRE(scaled0 == Catch::Approx(0.0f));
                REQUIRE(scaled127 == Catch::Approx(1.0f));
            }
        }

        WHEN("Scaling 0-127 to custom range (50.0-100.0)") {
            float scaled0 = MidiValueConverter::midiToScaled(0, 50.0f, 100.0f);
            float scaled127 = MidiValueConverter::midiToScaled(127, 50.0f, 100.0f);
            float scaled64 = MidiValueConverter::midiToScaled(64, 50.0f, 100.0f);

            THEN("Values map correctly") {
                REQUIRE(scaled0 == Catch::Approx(50.0f));
                REQUIRE(scaled127 == Catch::Approx(100.0f));
                REQUIRE(scaled64 == Catch::Approx(75.0f).epsilon(0.5f));
            }
        }

        WHEN("Scaling with inverted values") {
            float scaled0 = MidiValueConverter::midiToScaled(0, 0.0f, 1.0f, true);
            float scaled127 = MidiValueConverter::midiToScaled(127, 0.0f, 1.0f, true);

            THEN("Range is inverted") {
                REQUIRE(scaled0 == Catch::Approx(1.0f));
                REQUIRE(scaled127 == Catch::Approx(0.0f));
            }
        }

        WHEN("Scaling to negative range") {
            float scaled0 = MidiValueConverter::midiToScaled(0, -1.0f, 1.0f);
            float scaled127 = MidiValueConverter::midiToScaled(127, -1.0f, 1.0f);
            float scaled64 = MidiValueConverter::midiToScaled(64, -1.0f, 1.0f);

            THEN("Negative range works correctly") {
                REQUIRE(scaled0 == Catch::Approx(-1.0f));
                REQUIRE(scaled127 == Catch::Approx(1.0f));
                // 64/127 = 0.5039..., mapped to [-1, 1] = -1 + 0.5039 * 2 = 0.0079
                REQUIRE(scaled64 == Catch::Approx(0.0f).margin(0.02f));
            }
        }
    }
}

SCENARIO("MidiValueConverter boolean conversion", "[midimapping]") {
    GIVEN("MIDI CC values") {
        WHEN("Converting values below threshold (< 64)") {
            THEN("Results are false") {
                REQUIRE(MidiValueConverter::midiToBoolean(0) == false);
                REQUIRE(MidiValueConverter::midiToBoolean(32) == false);
                REQUIRE(MidiValueConverter::midiToBoolean(63) == false);
            }
        }

        WHEN("Converting values at or above threshold (>= 64)") {
            THEN("Results are true") {
                REQUIRE(MidiValueConverter::midiToBoolean(64) == true);
                REQUIRE(MidiValueConverter::midiToBoolean(96) == true);
                REQUIRE(MidiValueConverter::midiToBoolean(127) == true);
            }
        }

        WHEN("Converting boundary value (64)") {
            THEN("Result is true") {
                REQUIRE(MidiValueConverter::midiToBoolean(64) == true);
            }
        }
    }
}

SCENARIO("MidiValueConverter discrete index conversion", "[midimapping]") {
    GIVEN("MIDI values and max indices") {
        WHEN("Converting to octave range (0-9)") {
            int octave0 = MidiValueConverter::midiToDiscreteIndex(0, 9);
            int octave127 = MidiValueConverter::midiToDiscreteIndex(127, 9);

            THEN("Values map to octave range") {
                REQUIRE(octave0 == 0);
                REQUIRE(octave127 == 9);
            }
        }

        WHEN("Converting to note range (0-11)") {
            int note0 = MidiValueConverter::midiToDiscreteIndex(0, 11);
            int note127 = MidiValueConverter::midiToDiscreteIndex(127, 11);

            THEN("Values map to note range") {
                REQUIRE(note0 == 0);
                REQUIRE(note127 == 11);
            }
        }

        WHEN("Converting to small ranges") {
            int index0 = MidiValueConverter::midiToDiscreteIndex(0, 3);
            int index127 = MidiValueConverter::midiToDiscreteIndex(127, 3);
            int index64 = MidiValueConverter::midiToDiscreteIndex(64, 3);

            THEN("Values are distributed across range") {
                REQUIRE(index0 == 0);
                REQUIRE(index127 == 3);
                REQUIRE(index64 >= 1);
                REQUIRE(index64 <= 2);
            }
        }

        WHEN("Converting to single value range (0)") {
            int index = MidiValueConverter::midiToDiscreteIndex(64, 0);

            THEN("Result is 0") {
                REQUIRE(index == 0);
            }
        }
    }
}

// ============================================================================
// MidiMapping Tests
// ============================================================================

SCENARIO("MidiMapping structure", "[midimapping]") {
    GIVEN("A default MidiMapping") {
        MidiMapping mapping;
        mapping.type = MidiMessageType::CC;
        mapping.midiNumber = 1;
        mapping.controlId = MidiControlId::OCTAVE_SLIDER;

        THEN("Has expected default values") {
            REQUIRE(mapping.type == MidiMessageType::CC);
            REQUIRE(mapping.midiNumber == 1);
            REQUIRE(mapping.controlId == MidiControlId::OCTAVE_SLIDER);
            REQUIRE(mapping.minValue == Catch::Approx(0.0f));
            REQUIRE(mapping.maxValue == Catch::Approx(1.0f));
            REQUIRE(mapping.invert == false);
        }
    }

    GIVEN("A MidiMapping with custom range") {
        MidiMapping mapping;
        mapping.type = MidiMessageType::CC;
        mapping.midiNumber = 10;
        mapping.controlId = MidiControlId::NOTE_SLIDER;
        mapping.minValue = 0.25f;
        mapping.maxValue = 0.75f;
        mapping.invert = true;

        THEN("Custom values are set correctly") {
            REQUIRE(mapping.minValue == Catch::Approx(0.25f));
            REQUIRE(mapping.maxValue == Catch::Approx(0.75f));
            REQUIRE(mapping.invert == true);
        }
    }

    GIVEN("A NOTE-based MidiMapping") {
        MidiMapping mapping;
        mapping.type = MidiMessageType::NOTE;
        mapping.midiNumber = 60;
        mapping.controlId = MidiControlId::BUTTON_ROLL;

        THEN("Type is NOTE") {
            REQUIRE(mapping.type == MidiMessageType::NOTE);
            REQUIRE(mapping.midiNumber == 60);
        }
    }
}

// ============================================================================
// MidiControlId Name Lookup Tests
// ============================================================================

SCENARIO("MidiControlId name lookup", "[midimapping]") {
    GIVEN("Various MidiControlIds") {
        WHEN("Getting names for piano roll notes") {
            THEN("All 12 notes have names") {
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_C) == "Scale Note C");
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_Db) == "Scale Note Db");
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_D) == "Scale Note D");
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_Eb) == "Scale Note Eb");
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_E) == "Scale Note E");
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_F) == "Scale Note F");
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_Gb) == "Scale Note Gb");
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_G) == "Scale Note G");
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_Ab) == "Scale Note Ab");
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_A) == "Scale Note A");
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_Bb) == "Scale Note Bb");
                REQUIRE(getMidiControlName(MidiControlId::SCALE_NOTE_B) == "Scale Note B");
            }
        }

        WHEN("Getting names for sliders") {
            THEN("All slider names are descriptive") {
                REQUIRE(getMidiControlName(MidiControlId::OCTAVE_SLIDER) == "Octave Slider");
                REQUIRE(getMidiControlName(MidiControlId::NOTE_SLIDER) == "Note Slider");
                REQUIRE(getMidiControlName(MidiControlId::PROBABILITY_SLIDER) == "Probability Slider");
            }
        }

        WHEN("Getting names for sequencer steps") {
            THEN("Steps are numbered") {
                REQUIRE(getMidiControlName(MidiControlId::STEP_0) == "Step 0");
                REQUIRE(getMidiControlName(MidiControlId::STEP_7) == "Step 7");
            }
        }

        WHEN("Getting names for buttons") {
            THEN("All button names are descriptive") {
                REQUIRE(getMidiControlName(MidiControlId::BUTTON_ROLL) == "Roll Button");
                REQUIRE(getMidiControlName(MidiControlId::BUTTON_PAGE_LEFT) == "Page Left / Duplicate");
                REQUIRE(getMidiControlName(MidiControlId::BUTTON_PAGE_RIGHT) == "Page Right / Clear Window");
                REQUIRE(getMidiControlName(MidiControlId::BUTTON_LENGTH_PLUS) == "Length + / Clear Step");
                REQUIRE(getMidiControlName(MidiControlId::BUTTON_LENGTH_MINUS) == "Length - / Randomize All");
                REQUIRE(getMidiControlName(MidiControlId::BUTTON_NUDGE_LEFT) == "Nudge Left / Randomize Step");
                REQUIRE(getMidiControlName(MidiControlId::BUTTON_NUDGE_RIGHT) == "Nudge Right / Shuffle");
                REQUIRE(getMidiControlName(MidiControlId::BUTTON_SHIFT) == "Shift Button");
            }
        }

        WHEN("Getting names for subsequence controls") {
            THEN("Names are descriptive") {
                REQUIRE(getMidiControlName(MidiControlId::SUBSEQ_BAR_0) == "Subseq Bar 0");
                REQUIRE(getMidiControlName(MidiControlId::SUBSEQ_BAR_7) == "Subseq Bar 7");
                REQUIRE(getMidiControlName(MidiControlId::SUBSEQ_MINUS) == "Subseq Minus Button");
                REQUIRE(getMidiControlName(MidiControlId::SUBSEQ_PLUS) == "Subseq Plus Button");
            }
        }

        WHEN("Getting names for note range controls") {
            THEN("Names are descriptive") {
                REQUIRE(getMidiControlName(MidiControlId::NOTE_RANGE_MIN) == "Note Range Offset");
                REQUIRE(getMidiControlName(MidiControlId::NOTE_RANGE_MAX) == "Note Range Size");
            }
        }

        WHEN("Getting names for clock controls") {
            THEN("Names are descriptive") {
                REQUIRE(getMidiControlName(MidiControlId::SUBDIVISION_SLIDER) == "Subdivision");
            }
        }

        WHEN("Getting names for navigation") {
            THEN("Names are descriptive") {
                REQUIRE(getMidiControlName(MidiControlId::SETTINGS_TOGGLE) == "Settings Toggle");
            }
        }

        WHEN("Getting name for unknown control ID") {
            THEN("Returns 'Unknown Control'") {
                REQUIRE(getMidiControlName(MidiControlId::COUNT) == "Unknown Control");
            }
        }
    }
}
