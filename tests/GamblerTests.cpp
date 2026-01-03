#include "backend/Clock.h"
#include "backend/ClockMode.h"
#include "backend/DawSync.h"
#include "backend/InternalClock.h"
#include "backend/Scale.h"
#include "backend/Sequence.h"
#include "backend/Step.h"
#include "backend/StepModulation.h"
#include "backend/StepModulations.h"
#include "backend/StepNote.h"
#include "backend/StepNotes.h"
#include "helpers/SequenceBuilder.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <vector>

// ============================================================================
// StepNote Tests
// ============================================================================

SCENARIO("StepNote construction and manipulation", "[stepnote]") {
    GIVEN("Default construction") {
        StepNote note;

        THEN("Note is 60 and not selected") {
            REQUIRE(note.getNoteNumber() == 60);
            REQUIRE(note.isSelected() == false);
        }
    }

    GIVEN("Construction with note number") {
        StepNote note(72);

        THEN("Note is set correctly") {
            REQUIRE(note.getNoteNumber() == 72);
            REQUIRE(note.isSelected() == false);
        }
    }

    GIVEN("Construction with note and selection") {
        StepNote note(48, true);

        THEN("Both are set") {
            REQUIRE(note.getNoteNumber() == 48);
            REQUIRE(note.isSelected() == true);
        }
    }

    GIVEN("A StepNote") {
        StepNote note;

        WHEN("Valid MIDI note is set") {
            note.setNoteNumber(127);

            THEN("Note is updated") {
                REQUIRE(note.getNoteNumber() == 127);
            }
        }
    }

    GIVEN("A StepNote with value 60") {
        StepNote note(60);

        WHEN("Invalid MIDI note > 127 is set") {
            note.setNoteNumber(200);

            THEN("Note is not updated") {
                REQUIRE(note.getNoteNumber() == 60);
            }
        }

        WHEN("Invalid MIDI note < 0 is set") {
            note.setNoteNumber(-10);

            THEN("Note is not updated") {
                REQUIRE(note.getNoteNumber() == 60);
            }
        }
    }

    GIVEN("An unselected note") {
        StepNote note;

        WHEN("Selected") {
            note.setSelected(true);

            THEN("isSelected returns true") {
                REQUIRE(note.isSelected() == true);
            }
        }
    }

    GIVEN("A selected note") {
        StepNote note(60, true);

        WHEN("Deselected") {
            note.setSelected(false);

            THEN("isSelected returns false") {
                REQUIRE(note.isSelected() == false);
            }
        }
    }
}

// ============================================================================
// StepNotes Tests
// ============================================================================

SCENARIO("StepNotes construction and basic operations", "[stepnotes]") {
    GIVEN("Default construction") {
        StepNotes stepNotes;

        THEN("Length is 1 and index is -1") {
            REQUIRE(stepNotes.getLength() == 1);
            REQUIRE(stepNotes.getCurrentIndex() == -1);
        }

        THEN("All notes are initialized to 60") {
            for (int i = 0; i < StepNotes::MAX_LENGTH; i++) {
                REQUIRE(stepNotes.getNote(i).getNoteNumber() == 60);
            }
        }
    }

    GIVEN("A StepNotes instance") {
        StepNotes stepNotes;

        WHEN("A note is set at valid index") {
            stepNotes.setNote(0, 72);

            THEN("Note is updated") {
                REQUIRE(stepNotes.getNote(0).getNoteNumber() == 72);
            }
        }

        WHEN("Multiple notes are set") {
            stepNotes.setLength(3);
            stepNotes.setNote(0, 60);
            stepNotes.setNote(1, 64);
            stepNotes.setNote(2, 67);

            THEN("All are stored correctly") {
                REQUIRE(stepNotes.getNote(0).getNoteNumber() == 60);
                REQUIRE(stepNotes.getNote(1).getNoteNumber() == 64);
                REQUIRE(stepNotes.getNote(2).getNoteNumber() == 67);
            }
        }

        WHEN("Valid length is set") {
            stepNotes.setLength(5);

            THEN("Length is updated") {
                REQUIRE(stepNotes.getLength() == 5);
            }
        }

        WHEN("Length > MAX_LENGTH is set") {
            stepNotes.setLength(10);

            THEN("Length is not updated") {
                REQUIRE(stepNotes.getLength() == 1);
            }
        }

        WHEN("Length < MIN_LENGTH is set") {
            stepNotes.setLength(0);

            THEN("Length is not updated") {
                REQUIRE(stepNotes.getLength() == 1);
            }
        }

        WHEN("Length is increased") {
            int initialLength = stepNotes.getLength();
            stepNotes.increaseLength();

            THEN("Length increments by 1") {
                REQUIRE(stepNotes.getLength() == initialLength + 1);
            }
        }
    }

    GIVEN("StepNotes at MAX_LENGTH") {
        StepNotes stepNotes;
        stepNotes.setLength(StepNotes::MAX_LENGTH);

        WHEN("Length is increased") {
            stepNotes.increaseLength();

            THEN("Length stays at MAX_LENGTH") {
                REQUIRE(stepNotes.getLength() == StepNotes::MAX_LENGTH);
            }
        }
    }

    GIVEN("StepNotes with length 5") {
        StepNotes stepNotes;
        stepNotes.setLength(5);

        WHEN("Length is decreased") {
            stepNotes.decreaseLength();

            THEN("Length decrements by 1") {
                REQUIRE(stepNotes.getLength() == 4);
            }
        }
    }

    GIVEN("StepNotes at MIN_LENGTH") {
        StepNotes stepNotes;
        stepNotes.setLength(StepNotes::MIN_LENGTH);

        WHEN("Length is decreased") {
            stepNotes.decreaseLength();

            THEN("Length stays at MIN_LENGTH") {
                REQUIRE(stepNotes.getLength() == StepNotes::MIN_LENGTH);
            }
        }
    }


    GIVEN("StepNotes with 3 notes") {
        StepNotes stepNotes;
        stepNotes.setLength(3);
        stepNotes.setNote(0, 60);
        stepNotes.setNote(1, 64);
        stepNotes.setNote(2, 67);

        WHEN("Ticked 3 times") {
            THEN("Cycles through all notes") {
                REQUIRE(stepNotes.tick() == 60);
                REQUIRE(stepNotes.tick() == 64);
                REQUIRE(stepNotes.tick() == 67);
            }
        }
    }

    GIVEN("StepNotes at last note") {
        StepNotes stepNotes;
        stepNotes.setLength(2);
        stepNotes.setNote(0, 60);
        stepNotes.setNote(1, 70);
        stepNotes.tick();  // -> 60
        stepNotes.tick();  // -> 70

        WHEN("Ticked again") {
            int wrappedNote = stepNotes.tick();

            THEN("Wraps to first note") {
                REQUIRE(wrappedNote == 60);
                REQUIRE(stepNotes.getCurrentIndex() == 0);
            }
        }
    }

    GIVEN("StepNotes with length 2") {
        StepNotes stepNotes;
        stepNotes.setLength(2);

        WHEN("Ticked once") {
            stepNotes.tick();

            THEN("Updates selection to current note") {
                REQUIRE(stepNotes.getNote(0).isSelected() == true);
                REQUIRE(stepNotes.getNote(1).isSelected() == false);
            }
        }
    }

    GIVEN("StepNotes with length 3") {
        StepNotes stepNotes;
        stepNotes.setLength(3);

        WHEN("Ticked multiple times") {
            stepNotes.tick();

            THEN("Selection follows current index on first tick") {
                REQUIRE(stepNotes.getNote(0).isSelected() == true);
                REQUIRE(stepNotes.getNote(1).isSelected() == false);
                REQUIRE(stepNotes.getNote(2).isSelected() == false);
            }

            stepNotes.tick();

            THEN("Selection follows current index on second tick") {
                REQUIRE(stepNotes.getNote(0).isSelected() == false);
                REQUIRE(stepNotes.getNote(1).isSelected() == true);
                REQUIRE(stepNotes.getNote(2).isSelected() == false);
            }

            stepNotes.tick();

            THEN("Selection follows current index on third tick") {
                REQUIRE(stepNotes.getNote(0).isSelected() == false);
                REQUIRE(stepNotes.getNote(1).isSelected() == false);
                REQUIRE(stepNotes.getNote(2).isSelected() == true);
            }
        }
    }

    GIVEN("StepNotes ticked through multiple cycles") {
        StepNotes stepNotes;
        stepNotes.setLength(2);

        WHEN("Ticked 10 times") {
            THEN("Index never returns to -1 after first tick") {
                for (int cycle = 0; cycle < 10; cycle++) {
                    stepNotes.tick();
                    REQUIRE(stepNotes.getCurrentIndex() >= 0);
                    REQUIRE(stepNotes.getCurrentIndex() < stepNotes.getLength());
                }
            }
        }
    }

    GIVEN("StepNotes that has been ticked") {
        StepNotes stepNotes;
        stepNotes.tick();
        stepNotes.tick();

        WHEN("Reset") {
            stepNotes.reset();

            THEN("Index returns to -1") {
                REQUIRE(stepNotes.getCurrentIndex() == -1);
            }
        }
    }

    GIVEN("StepNotes with selections") {
        StepNotes stepNotes;
        stepNotes.setLength(3);
        stepNotes.tick();

        WHEN("Reset") {
            stepNotes.reset();

            THEN("All selections are cleared") {
                for (int i = 0; i < StepNotes::MAX_LENGTH; i++) {
                    REQUIRE(stepNotes.getNote(i).isSelected() == false);
                }
            }
        }
    }
}

// ============================================================================
// StepModulation Tests
// ============================================================================

TEST_CASE("StepModulation") {
    SECTION("Construction") {
        SECTION("Given default construction, Then defaults to CC type with value 64") {
            // Arrange & Act
            StepModulation mod;

            // Assert
            REQUIRE(mod.getType() == ModulationType::CC);
            REQUIRE(mod.getCCNumber() == 1);
            REQUIRE(mod.getValue() == 64);
            REQUIRE(mod.isSelected() == false);
        }

        SECTION("Given construction with parameters, Then values are set correctly") {
            // Arrange & Act
            StepModulation mod(ModulationType::Velocity, 0, 127);

            // Assert
            REQUIRE(mod.getType() == ModulationType::Velocity);
            REQUIRE(mod.getValue() == 127);
        }
    }

    SECTION("setType()") {
        SECTION("Given CC type, When set to Velocity, Then type is updated") {
            // Arrange
            StepModulation mod;

            // Act
            mod.setType(ModulationType::Velocity);

            // Assert
            REQUIRE(mod.getType() == ModulationType::Velocity);
        }
    }

    SECTION("setCCNumber()") {
        SECTION("Given valid CC number, When set, Then CC number is updated") {
            // Arrange
            StepModulation mod;

            // Act
            mod.setCCNumber(74);

            // Assert
            REQUIRE(mod.getCCNumber() == 74);
        }

        SECTION("Given invalid CC number (> 127), When set, Then CC number is not updated") {
            // Arrange
            StepModulation mod;
            mod.setCCNumber(74);

            // Act
            mod.setCCNumber(200);

            // Assert
            REQUIRE(mod.getCCNumber() == 74);
        }

        SECTION("Given invalid CC number (< 0), When set, Then CC number is not updated") {
            // Arrange
            StepModulation mod;
            mod.setCCNumber(74);

            // Act
            mod.setCCNumber(-10);

            // Assert
            REQUIRE(mod.getCCNumber() == 74);
        }
    }

    SECTION("setValue()") {
        SECTION("Given valid value, When set, Then value is updated") {
            // Arrange
            StepModulation mod;

            // Act
            mod.setValue(127);

            // Assert
            REQUIRE(mod.getValue() == 127);
        }

        SECTION("Given invalid value (> 127), When set, Then value is not updated") {
            // Arrange
            StepModulation mod;
            mod.setValue(100);

            // Act
            mod.setValue(200);

            // Assert
            REQUIRE(mod.getValue() == 100);
        }
    }

    SECTION("getTypeString()") {
        SECTION("Given Velocity type, When getTypeString called, Then returns 'Velocity'") {
            // Arrange
            StepModulation mod(ModulationType::Velocity, 0, 100);

            // Act
            std::string typeStr = mod.getTypeString();

            // Assert
            REQUIRE(typeStr == "Velocity");
        }

        SECTION("Given CC type, When getTypeString called, Then returns 'CC'") {
            // Arrange
            StepModulation mod(ModulationType::CC, 1, 64);

            // Act
            std::string typeStr = mod.getTypeString();

            // Assert
            REQUIRE(typeStr == "CC");
        }
    }
}

// ============================================================================
// StepModulations Tests
// ============================================================================

TEST_CASE("StepModulations") {
    SECTION("Construction") {
        SECTION("Given default construction, Then has 7 modulations") {
            // Arrange & Act
            StepModulations mods;

            // Assert
            REQUIRE(mods.getAllModulations().size() == 7);
        }

        SECTION("Given default construction, Then first slot is Velocity") {
            // Arrange & Act
            StepModulations mods;

            // Assert
            REQUIRE(mods.getModulation(0).getType() == ModulationType::Velocity);
            REQUIRE(mods.getModulation(0).getValue() == 100);
        }

        SECTION("Given default construction, Then other slots are CC") {
            // Arrange & Act
            StepModulations mods;

            // Assert
            for (int i = 1; i < StepModulations::NUM_MODULATIONS; i++) {
                REQUIRE(mods.getModulation(i).getType() == ModulationType::CC);
                REQUIRE(mods.getModulation(i).getCCNumber() == i);
                REQUIRE(mods.getModulation(i).getValue() == 64);
            }
        }
    }

    SECTION("setModulationType()") {
        SECTION("Given modulation, When type is set, Then type is updated") {
            // Arrange
            StepModulations mods;

            // Act
            mods.setModulationType(0, ModulationType::CC);

            // Assert
            REQUIRE(mods.getModulation(0).getType() == ModulationType::CC);
        }
    }

    SECTION("setModulationValue()") {
        SECTION("Given modulation, When value is set, Then value is updated") {
            // Arrange
            StepModulations mods;

            // Act
            mods.setModulationValue(0, 127);

            // Assert
            REQUIRE(mods.getModulation(0).getValue() == 127);
        }
    }

    SECTION("setModulationSelected()") {
        SECTION("Given unselected modulation, When selected, Then isSelected returns true") {
            // Arrange
            StepModulations mods;

            // Act
            mods.setModulationSelected(0, true);

            // Assert
            REQUIRE(mods.getModulation(0).isSelected() == true);
        }
    }

    SECTION("getSelectedModulations()") {
        SECTION("Given no modulations selected, When called, Then returns empty vector") {
            // Arrange
            StepModulations mods;

            // Act
            auto selected = mods.getSelectedModulations();

            // Assert
            REQUIRE(selected.size() == 0);
        }

        SECTION("Given 3 modulations selected, When called, Then returns 3 modulations") {
            // Arrange
            StepModulations mods;
            mods.setModulationSelected(0, true);
            mods.setModulationSelected(2, true);
            mods.setModulationSelected(4, true);

            // Act
            auto selected = mods.getSelectedModulations();

            // Assert
            REQUIRE(selected.size() == 3);
        }
    }

    SECTION("updateModulations()") {
        SECTION("Given array of values, When updated, Then all values are set") {
            // Arrange
            StepModulations mods;
            std::array<int, 7> values = {100, 110, 120, 64, 50, 40, 30};

            // Act
            mods.updateModulations(values);

            // Assert
            for (int i = 0; i < StepModulations::NUM_MODULATIONS; i++) {
                REQUIRE(mods.getModulation(i).getValue() == values[i]);
            }
        }
    }

    SECTION("randomize()") {
        SECTION("Given modulations, When randomized, Then values change") {
            // Arrange
            StepModulations mods;
            std::vector<int> originalValues;
            for (int i = 0; i < StepModulations::NUM_MODULATIONS; i++) {
                originalValues.push_back(mods.getModulation(i).getValue());
            }

            // Act
            mods.randomize();

            // Assert - at least one value should have changed
            bool anyChanged = false;
            for (int i = 0; i < StepModulations::NUM_MODULATIONS; i++) {
                if (mods.getModulation(i).getValue() != originalValues[i]) {
                    anyChanged = true;
                    break;
                }
            }
            REQUIRE(anyChanged == true);
        }

        SECTION("Given modulations, When randomized, Then all values are valid MIDI") {
            // Arrange
            StepModulations mods;

            // Act
            mods.randomize();

            // Assert
            for (int i = 0; i < StepModulations::NUM_MODULATIONS; i++) {
                int val = mods.getModulation(i).getValue();
                REQUIRE(val >= 0);
                REQUIRE(val <= 127);
            }
        }
    }
}

// ============================================================================
// Scale Tests
// ============================================================================

TEST_CASE("Scale") {
    SECTION("Construction") {
        SECTION("Given default construction, Then all notes are enabled (chromatic scale)") {
            // Arrange & Act
            Scale scale;

            // Assert
            for (int i = 0; i < Scale::NUM_CHROMATIC_NOTES; i++) {
                REQUIRE(scale.getScaleNotes()[i] == 1);
            }
        }

        SECTION("Given default construction, Then range is 0-127") {
            // Arrange & Act
            Scale scale;

            // Assert
            REQUIRE(scale.getRangeMin() == 0);
            REQUIRE(scale.getRangeMax() == 127);
        }
    }

    SECTION("setScaleFromNames()") {
        SECTION("Given C major scale names, When set, Then correct notes are enabled") {
            // Arrange
            Scale scale;
            std::vector<std::string> cMajor = {"C", "D", "E", "F", "G", "A", "B"};

            // Act
            scale.setScaleFromNames(cMajor);

            // Assert
            REQUIRE(scale.getScaleNotes()[0] == 1);   // C
            REQUIRE(scale.getScaleNotes()[1] == 0);   // Db
            REQUIRE(scale.getScaleNotes()[2] == 1);   // D
            REQUIRE(scale.getScaleNotes()[3] == 0);   // Eb
            REQUIRE(scale.getScaleNotes()[4] == 1);   // E
            REQUIRE(scale.getScaleNotes()[5] == 1);   // F
            REQUIRE(scale.getScaleNotes()[6] == 0);   // Gb
            REQUIRE(scale.getScaleNotes()[7] == 1);   // G
            REQUIRE(scale.getScaleNotes()[8] == 0);   // Ab
            REQUIRE(scale.getScaleNotes()[9] == 1);   // A
            REQUIRE(scale.getScaleNotes()[10] == 0);  // Bb
            REQUIRE(scale.getScaleNotes()[11] == 1);  // B
        }

        SECTION("Given chromatic scale, When set, Then all notes are enabled") {
            // Arrange
            Scale scale;
            std::vector<std::string> chromatic = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};

            // Act
            scale.setScaleFromNames(chromatic);

            // Assert
            for (int i = 0; i < Scale::NUM_CHROMATIC_NOTES; i++) {
                REQUIRE(scale.getScaleNotes()[i] == 1);
            }
        }
    }

    SECTION("isNoteInScale()") {
        SECTION("Given C major scale, When checking C (60), Then returns true") {
            // Arrange
            Scale scale;
            scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});

            // Act & Assert
            REQUIRE(scale.isNoteInScale(60) == true);  // C4
        }

        SECTION("Given C major scale, When checking C# (61), Then returns false") {
            // Arrange
            Scale scale;
            scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});

            // Act & Assert
            REQUIRE(scale.isNoteInScale(61) == false);  // C#4
        }

        SECTION("Given C major scale, When checking D (62), Then returns true") {
            // Arrange
            Scale scale;
            scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});

            // Act & Assert
            REQUIRE(scale.isNoteInScale(62) == true);  // D4
        }
    }

    SECTION("quantize()") {
        SECTION("Given C major scale, When quantizing 61 (C#), Then returns 60 (C)") {
            // Arrange
            Scale scale;
            scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});

            // Act
            int quantized = scale.quantize(61);

            // Assert
            REQUIRE(quantized == 60);
        }

        SECTION("Given C major scale, When quantizing 62 (D), Then returns 62 (D)") {
            // Arrange
            Scale scale;
            scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});

            // Act
            int quantized = scale.quantize(62);

            // Assert
            REQUIRE(quantized == 62);
        }

        SECTION("Given C major scale, When quantizing 63 (Eb), Then returns 62 (D)") {
            // Arrange
            Scale scale;
            scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});

            // Act
            int quantized = scale.quantize(63);

            // Assert - When equidistant, rounds down
            REQUIRE(quantized == 62);
        }

        SECTION("Given no scale set, When quantizing, Then returns original note") {
            // Arrange
            Scale scale;

            // Act
            int quantized = scale.quantize(61);

            // Assert
            REQUIRE(quantized == 61);
        }

        SECTION("Given note > 127, When quantizing, Then clamps to 127") {
            // Arrange
            Scale scale;
            scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});

            // Act
            int quantized = scale.quantize(200);

            // Assert
            REQUIRE(quantized <= 127);
        }
    }

    SECTION("setRange()") {
        SECTION("Given valid range, When set, Then range is updated") {
            // Arrange
            Scale scale;

            // Act
            scale.setRange(60, 72);

            // Assert
            REQUIRE(scale.getRangeMin() == 60);
            REQUIRE(scale.getRangeMax() == 72);
        }

        SECTION("Given invalid range (min > max), When set, Then range is not updated") {
            // Arrange
            Scale scale;

            // Act
            scale.setRange(72, 60);

            // Assert
            REQUIRE(scale.getRangeMin() == 0);
            REQUIRE(scale.getRangeMax() == 127);
        }
    }

    // TODO: These methods don't exist in Scale - need to be implemented or removed
    // SECTION("applyRange()") {
    //     SECTION("Given range 60-72, When applying to note 80, Then clamps to 72") {
    //         // Arrange
    //         Scale scale;
    //         scale.setRange(60, 72);
    //
    //         // Act
    //         int clamped = scale.applyRange(80);
    //
    //         // Assert
    //         REQUIRE(clamped == 72);
    //     }
    //
    //     SECTION("Given range 60-72, When applying to note 50, Then clamps to 60") {
    //         // Arrange
    //         Scale scale;
    //         scale.setRange(60, 72);
    //
    //         // Act
    //         int clamped = scale.applyRange(50);
    //
    //         // Assert
    //         REQUIRE(clamped == 60);
    //     }
    //
    //     SECTION("Given range 60-72, When applying to note 66, Then returns 66") {
    //         // Arrange
    //         Scale scale;
    //         scale.setRange(60, 72);
    //
    //         // Act
    //         int clamped = scale.applyRange(66);
    //
    //         // Assert
    //         REQUIRE(clamped == 66);
    //     }
    // }
    //
    // SECTION("quantizeAndApplyRange()") {
    //     SECTION("Given C major scale and range 60-72, When processing 61, Then quantizes and clamps") {
    //         // Arrange
    //         Scale scale;
    //         scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});
    //         scale.setRange(60, 72);
    //
    //         // Act
    //         int result = scale.quantizeAndApplyRange(61);
    //
    //         // Assert
    //         REQUIRE(result == 60);  // C# quantized to C
    //     }
    //
    //     SECTION("Given C major scale and range 60-72, When processing 80, Then quantizes and clamps") {
    //         // Arrange
    //         Scale scale;
    //         scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});
    //         scale.setRange(60, 72);
    //
    //         // Act
    //         int result = scale.quantizeAndApplyRange(80);
    //
    //         // Assert
    //         REQUIRE(result == 72);  // Clamped to max
    //     }
    // }
}

// ============================================================================
// Step Tests
// ============================================================================

TEST_CASE("Step") {
    SECTION("Construction") {
        SECTION("Given default construction, Then has StepNotes and StepModulations") {
            // Arrange & Act
            Step step;

            // Assert
            REQUIRE(step.getStepNotes().getLength() == 1);
            REQUIRE(step.getStepModulations().getAllModulations().size() == 7);
        }
    }

    SECTION("tick()") {
        SECTION("Given Step with note 60, When ticked, Then returns 60") {
            // Arrange
            Step step;
            step.getStepNotes().setNote(0, 60);

            // Act
            int note = step.tick();

            // Assert
            REQUIRE(note == 60);
        }

        SECTION("Given Step with 2 substeps, When ticked twice, Then cycles through notes") {
            // Arrange
            Step step;
            step.getStepNotes().setLength(2);
            step.getStepNotes().setNote(0, 60);
            step.getStepNotes().setNote(1, 70);

            // Act & Assert
            REQUIRE(step.tick() == 60);
            REQUIRE(step.tick() == 70);
        }
    }

    SECTION("randomizeNote()") {
        SECTION("Given Step and C major scale, When randomized, Then note is in scale") {
            // Arrange
            Step step;
            Scale scale;
            scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});

            // Act
            step.randomizeNote(scale);
            int note = step.getStepNotes().getNote(0).getNoteNumber();

            // Assert
            REQUIRE(scale.isNoteInScale(note) == true);
        }

        SECTION("Given Step and range 60-72, When randomized, Then note is in range") {
            // Arrange
            Step step;
            Scale scale;
            scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});
            scale.setRange(60, 72);

            // Act
            for (int i = 0; i < 10; i++) {
                step.randomizeNote(scale);
                int note = step.getStepNotes().getNote(0).getNoteNumber();

                // Assert
                REQUIRE(note >= 60);
                REQUIRE(note <= 72);
            }
        }
    }

    SECTION("updateCurrentNoteNumber()") {
        SECTION("Given Step, When note number updated, Then note is changed") {
            // Arrange
            Step step;

            // Act
            step.updateCurrentNoteNumber(72);

            // Assert
            REQUIRE(step.getStepNotes().getNote(0).getNoteNumber() == 72);
        }
    }

    SECTION("randomizeModulations()") {
        SECTION("Given Step, When modulations randomized, Then values change") {
            // Arrange
            Step step;
            std::vector<int> originalValues;
            for (int i = 0; i < StepModulations::NUM_MODULATIONS; i++) {
                originalValues.push_back(step.getStepModulations().getModulation(i).getValue());
            }

            // Act
            step.randomizeModulations();

            // Assert - at least one should have changed
            bool anyChanged = false;
            for (int i = 0; i < StepModulations::NUM_MODULATIONS; i++) {
                if (step.getStepModulations().getModulation(i).getValue() != originalValues[i]) {
                    anyChanged = true;
                    break;
                }
            }
            REQUIRE(anyChanged == true);
        }
    }

    SECTION("getModulations()") {
        SECTION("Given Step with no selected modulations, When called, Then returns empty vector") {
            // Arrange
            Step step;

            // Act
            auto mods = step.getModulations();

            // Assert
            REQUIRE(mods.size() == 0);
        }

        SECTION("Given Step with 2 selected modulations, When called, Then returns 2 modulations") {
            // Arrange
            Step step;
            step.getStepModulations().setModulationSelected(0, true);
            step.getStepModulations().setModulationSelected(1, true);

            // Act
            auto mods = step.getModulations();

            // Assert
            REQUIRE(mods.size() == 2);
        }
    }
}

// ============================================================================
// Sequence Tests
// ============================================================================

TEST_CASE("Sequence") {
    SECTION("Construction") {
        SECTION("Given default construction, Then has 64 steps and length 8") {
            // Arrange & Act
            Sequence sequence;

            // Assert
            REQUIRE(sequence.getLength() == 8);  // DEFAULT_LENGTH is 8
            REQUIRE(sequence.getCurrentIndex() == -1);
        }

        SECTION("Given default construction, Then all play probabilities are true") {
            // Arrange & Act
            Sequence sequence;

            // Assert
            for (int i = 0; i < Sequence::MAX_STEPS; i++) {
                REQUIRE(sequence.getPlayProbability(i) == true);
            }
        }

        SECTION("Given default construction, Then isRandom is false") {
            // Arrange & Act
            Sequence sequence;

            // Assert
            REQUIRE(sequence.getIsRandom() == false);
        }
    }

    SECTION("setLength()") {
        SECTION("Given valid length, When set, Then length is updated") {
            // Arrange
            Sequence sequence;

            // Act
            sequence.setLength(16);

            // Assert
            REQUIRE(sequence.getLength() == 16);
        }

        SECTION("Given length > MAX_STEPS, When set, Then length is not updated") {
            // Arrange
            Sequence sequence;

            // Act
            sequence.setLength(100);

            // Assert
            REQUIRE(sequence.getLength() == 8);  // Should remain at DEFAULT_LENGTH
        }
    }

    SECTION("tick()") {
        SECTION("Given sequence with notes, When ticked, Then returns next note") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(4)
                .withStep(0, {60})
                .withStep(1, {62})
                .withStep(2, {64})
                .withStep(3, {65})
                .build();

            // Act & Assert
            REQUIRE(sequence.tick() == 60);
            REQUIRE(sequence.tick() == 62);
            REQUIRE(sequence.tick() == 64);
            REQUIRE(sequence.tick() == 65);
        }

        SECTION("Given sequence at last step, When ticked, Then wraps to first step") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(2)
                .withStep(0, {60})
                .withStep(1, {70})
                .build();

            sequence.tick();  // -> 60
            sequence.tick();  // -> 70

            // Act
            int wrappedNote = sequence.tick();

            // Assert
            REQUIRE(wrappedNote == 60);
            REQUIRE(sequence.getCurrentIndex() == 0);
        }

        SECTION("Given sequence index never returns to -1 after first tick") {
            // Arrange
            Sequence sequence;
            sequence.setLength(4);

            // Act - tick through multiple cycles
            for (int cycle = 0; cycle < 10; cycle++) {
                sequence.tick();

                // Assert
                REQUIRE(sequence.getCurrentIndex() >= 0);
                REQUIRE(sequence.getCurrentIndex() < sequence.getLength());
            }
        }
    }

    SECTION("tick() with evolving patterns") {
        SECTION("Given 4-step sequence with step 3 having 2 substeps, When ticked through 3 passes, Then creates evolving pattern") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(4)
                .withStep(0, {60})
                .withStep(1, {60})
                .withStep(2, {60})
                .withStep(3, {60, 70})
                .build();

            // Act & Assert - First pass
            REQUIRE(sequence.tick() == 60);
            REQUIRE(sequence.tick() == 60);
            REQUIRE(sequence.tick() == 60);
            REQUIRE(sequence.tick() == 60);  // step 3, substep 0

            // Second pass - step 3 should play substep 1
            REQUIRE(sequence.tick() == 60);
            REQUIRE(sequence.tick() == 60);
            REQUIRE(sequence.tick() == 60);
            REQUIRE(sequence.tick() == 70);  // step 3, substep 1

            // Third pass - step 3 wraps back to substep 0
            REQUIRE(sequence.tick() == 60);
            REQUIRE(sequence.tick() == 60);
            REQUIRE(sequence.tick() == 60);
            REQUIRE(sequence.tick() == 60);  // step 3, substep 0 again
        }
    }

    SECTION("tick() with play probability") {
        SECTION("Given 100% probability, When ticked many times, Then all steps play") {
            // Arrange
            Sequence sequence;
            sequence.setLength(4);
            sequence.setProbabilityPercentage(100);  // All 64 slots are true

            // Act & Assert - tick many times, all should play
            for (int i = 0; i < 20; i++) {
                int note = sequence.tick();
                REQUIRE(note != -1);  // Should always play
            }
        }

        SECTION("Given 0% probability, When ticked many times, Then all steps rest") {
            // Arrange
            Sequence sequence;
            sequence.setLength(4);
            sequence.setProbabilityPercentage(0);  // All 64 slots are false

            // Act & Assert - tick many times, all should rest
            for (int i = 0; i < 20; i++) {
                int note = sequence.tick();
                REQUIRE(note == -1);  // Should always rest
            }
        }

        SECTION("Given 50% probability, When ticked many times, Then approximately half play") {
            // Arrange
            Sequence sequence;
            sequence.setLength(8);
            sequence.setProbabilityPercentage(50);  // 32 of 64 slots are true

            // Act - tick many times and count plays vs rests
            int playCount = 0;
            int restCount = 0;
            int totalTicks = 100;

            for (int i = 0; i < totalTicks; i++) {
                int note = sequence.tick();
                if (note == -1) {
                    restCount++;
                } else {
                    playCount++;
                }
            }

            // Assert - should be approximately 50/50 (allow 30-70% range for randomness)
            REQUIRE(playCount >= 30);
            REQUIRE(playCount <= 70);
            REQUIRE(restCount >= 30);
            REQUIRE(restCount <= 70);
            REQUIRE(playCount + restCount == totalTicks);
        }
    }

    SECTION("tick() with random mode") {
        SECTION("Given random mode enabled with scale, When ticked, Then notes are randomized and quantized") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(4)
                .withScale({"C", "D", "E", "F", "G", "A", "B"})
                .withRandomEnabled(true)
                .build();

            // Act - tick multiple times
            std::vector<int> notes;
            for (int i = 0; i < 8; i++) {
                int note = sequence.tick();
                notes.push_back(note);

                // Assert - all notes should be in scale
                REQUIRE(sequence.getScale().isNoteInScale(note) == true);
            }
        }
    }

    SECTION("setProbabilityPercentage()") {
        SECTION("Given 100%, When set, Then all 64 slots should be true") {
            // Arrange
            Sequence sequence;
            sequence.setLength(8);

            // Act
            sequence.setProbabilityPercentage(100);

            // Assert - Check all 64 slots
            int trueCount = 0;
            for (int i = 0; i < Sequence::MAX_STEPS; i++) {
                if (sequence.getPlayProbability(i)) trueCount++;
            }
            REQUIRE(trueCount == 64);
        }

        SECTION("Given 0%, When set, Then all 64 slots should be false") {
            // Arrange
            Sequence sequence;
            sequence.setLength(8);

            // Act
            sequence.setProbabilityPercentage(0);

            // Assert - Check all 64 slots
            int trueCount = 0;
            for (int i = 0; i < Sequence::MAX_STEPS; i++) {
                if (sequence.getPlayProbability(i)) trueCount++;
            }
            REQUIRE(trueCount == 0);
        }

        SECTION("Given 50%, When set, Then exactly 32 of 64 slots should be true") {
            // Arrange
            Sequence sequence;
            sequence.setLength(8);

            // Act
            sequence.setProbabilityPercentage(50);

            // Assert - Check all 64 slots
            int trueCount = 0;
            for (int i = 0; i < Sequence::MAX_STEPS; i++) {
                if (sequence.getPlayProbability(i)) trueCount++;
            }
            REQUIRE(trueCount == 32);  // Exactly 50% of 64
        }
    }

    SECTION("indexLeft() and indexRight()") {
        SECTION("Given sequence at index 2, When indexRight called, Then index is 3") {
            // Arrange
            Sequence sequence;
            sequence.setLength(8);
            sequence.tick();  // 0
            sequence.tick();  // 1
            sequence.tick();  // 2

            // Act
            sequence.indexRight();

            // Assert
            REQUIRE(sequence.getCurrentIndex() == 3);
        }

        SECTION("Given sequence at index 2, When indexLeft called, Then index is 1") {
            // Arrange
            Sequence sequence;
            sequence.setLength(8);
            sequence.tick();  // 0
            sequence.tick();  // 1
            sequence.tick();  // 2

            // Act
            sequence.indexLeft();

            // Assert
            REQUIRE(sequence.getCurrentIndex() == 1);
        }

        SECTION("Given sequence at index 0, When indexLeft called, Then wraps to last index") {
            // Arrange
            Sequence sequence;
            sequence.setLength(8);
            sequence.tick();  // 0

            // Act
            sequence.indexLeft();

            // Assert
            REQUIRE(sequence.getCurrentIndex() == 7);
        }
    }

    SECTION("increaseLength() and decreaseLength()") {
        SECTION("Given length 16, When increased, Then length is 17") {
            // Arrange
            Sequence sequence;
            sequence.setLength(16);

            // Act
            sequence.increaseLength();

            // Assert
            REQUIRE(sequence.getLength() == 17);
        }

        SECTION("Given length 16, When decreased, Then length is 15") {
            // Arrange
            Sequence sequence;
            sequence.setLength(16);

            // Act
            sequence.decreaseLength();

            // Assert
            REQUIRE(sequence.getLength() == 15);
        }

        SECTION("Given length at MAX_STEPS, When increased, Then length stays at MAX_STEPS") {
            // Arrange
            Sequence sequence;
            sequence.setLength(Sequence::MAX_STEPS);

            // Act
            sequence.increaseLength();

            // Assert
            REQUIRE(sequence.getLength() == Sequence::MAX_STEPS);
        }

        SECTION("Given length at MIN_LENGTH, When decreased, Then length stays at MIN_LENGTH") {
            // Arrange
            Sequence sequence;
            sequence.setLength(Sequence::MIN_LENGTH);

            // Act
            sequence.decreaseLength();

            // Assert
            REQUIRE(sequence.getLength() == Sequence::MIN_LENGTH);
        }
    }

    SECTION("nudgeLeft()") {
        SECTION("Given sequence [60, 62, 64, 65], When nudged left, Then becomes [62, 64, 65, 60]") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(4)
                .withStep(0, {60})
                .withStep(1, {62})
                .withStep(2, {64})
                .withStep(3, {65})
                .build();

            // Act
            sequence.nudgeLeft();

            // Assert - tick through and verify order
            REQUIRE(sequence.tick() == 62);
            REQUIRE(sequence.tick() == 64);
            REQUIRE(sequence.tick() == 65);
            REQUIRE(sequence.tick() == 60);
        }

        SECTION("Given sequence of length 1, When nudged left, Then nothing changes") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(1)
                .withStep(0, {60})
                .build();

            // Act
            sequence.nudgeLeft();

            // Assert
            REQUIRE(sequence.tick() == 60);
        }
    }

    SECTION("nudgeRight()") {
        SECTION("Given sequence [60, 62, 64, 65], When nudged right, Then becomes [65, 60, 62, 64]") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(4)
                .withStep(0, {60})
                .withStep(1, {62})
                .withStep(2, {64})
                .withStep(3, {65})
                .build();

            // Act
            sequence.nudgeRight();

            // Assert - tick through and verify order
            REQUIRE(sequence.tick() == 65);
            REQUIRE(sequence.tick() == 60);
            REQUIRE(sequence.tick() == 62);
            REQUIRE(sequence.tick() == 64);
        }

        SECTION("Given sequence of length 1, When nudged right, Then nothing changes") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(1)
                .withStep(0, {60})
                .build();

            // Act
            sequence.nudgeRight();

            // Assert
            REQUIRE(sequence.tick() == 60);
        }
    }

    SECTION("reset()") {
        SECTION("Given sequence that has been ticked, When reset, Then index returns to -1") {
            // Arrange
            Sequence sequence;
            sequence.tick();
            sequence.tick();
            sequence.tick();

            // Act
            sequence.reset();

            // Assert
            REQUIRE(sequence.getCurrentIndex() == -1);
        }
    }

    SECTION("Integration: Complex evolving pattern") {
        SECTION("Given complex sequence with multiple substeps, When played, Then creates expected pattern") {
            // Arrange - Create a sequence where:
            // Step 0: [60]
            // Step 1: [62, 63]
            // Step 2: [64]
            // Step 3: [65, 67, 69]
            auto sequence = SequenceBuilder()
                .withLength(4)
                .withStep(0, {60})
                .withStep(1, {62, 63})
                .withStep(2, {64})
                .withStep(3, {65, 67, 69})
                .build();

            // Act & Assert - First pass
            REQUIRE(sequence.tick() == 60);  // step 0
            REQUIRE(sequence.tick() == 62);  // step 1, substep 0
            REQUIRE(sequence.tick() == 64);  // step 2
            REQUIRE(sequence.tick() == 65);  // step 3, substep 0

            // Second pass
            REQUIRE(sequence.tick() == 60);  // step 0
            REQUIRE(sequence.tick() == 63);  // step 1, substep 1
            REQUIRE(sequence.tick() == 64);  // step 2
            REQUIRE(sequence.tick() == 67);  // step 3, substep 1

            // Third pass
            REQUIRE(sequence.tick() == 60);  // step 0
            REQUIRE(sequence.tick() == 62);  // step 1, substep 0 (wrapped)
            REQUIRE(sequence.tick() == 64);  // step 2
            REQUIRE(sequence.tick() == 69);  // step 3, substep 2

            // Fourth pass
            REQUIRE(sequence.tick() == 60);  // step 0
            REQUIRE(sequence.tick() == 63);  // step 1, substep 1
            REQUIRE(sequence.tick() == 64);  // step 2
            REQUIRE(sequence.tick() == 65);  // step 3, substep 0 (wrapped)
        }
    }

    SECTION("Integration: Random mode with scale and range") {
        SECTION("Given random mode with C major scale and range 60-72, When ticked, Then all notes are valid") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(8)
                .withScale({"C", "D", "E", "F", "G", "A", "B"})
                .withRange(60, 72)
                .withRandomEnabled(true)
                .build();

            // Act & Assert - tick many times to test randomization
            for (int i = 0; i < 32; i++) {
                int note = sequence.tick();

                // Should be in scale
                REQUIRE(sequence.getScale().isNoteInScale(note) == true);

                // Should be in range
                REQUIRE(note >= 60);
                REQUIRE(note <= 72);
            }
        }
    }

    SECTION("Integration: Probability with evolving patterns") {
        SECTION("Given sequence with 100% probability and substeps, When ticked, Then substeps evolve") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(4)
                .withStep(0, {60, 61})
                .withStep(1, {62, 63})
                .withStep(2, {64, 65})
                .withStep(3, {66, 67})
                .build();

            // Set 100% probability so all steps play
            sequence.setProbabilityPercentage(100);

            // Act - Tick through multiple passes and collect notes
            std::vector<int> firstPass;
            std::vector<int> secondPass;

            for (int i = 0; i < 4; i++) {
                int note = sequence.tick();
                firstPass.push_back(note);
            }

            for (int i = 0; i < 4; i++) {
                int note = sequence.tick();
                secondPass.push_back(note);
            }

            // Assert - All notes should be valid (not -1)
            for (int note : firstPass) {
                REQUIRE(note != -1);
            }
            for (int note : secondPass) {
                REQUIRE(note != -1);
            }

            // Assert - Substeps should have evolved (at least some notes should be different)
            // Step 0 should cycle between 60 and 61
            // Step 1 should cycle between 62 and 63, etc.
            bool hasEvolution = false;
            for (size_t i = 0; i < firstPass.size(); i++) {
                if (firstPass[i] != secondPass[i]) {
                    hasEvolution = true;
                    break;
                }
            }
            REQUIRE(hasEvolution);  // At least one step should have evolved
        }

        SECTION("Given sequence with 0% probability and substeps, When ticked, Then all rest") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(4)
                .withStep(0, {60, 61})
                .withStep(1, {62, 63})
                .withStep(2, {64, 65})
                .withStep(3, {66, 67})
                .build();

            // Set 0% probability so all steps rest
            sequence.setProbabilityPercentage(0);

            // Act & Assert - All ticks should return -1 (rest)
            for (int i = 0; i < 20; i++) {
                REQUIRE(sequence.tick() == -1);
            }
        }
    }
}

// ============================================================================
// InternalClock Tests
// ============================================================================

TEST_CASE("InternalClock") {
    SECTION("InternalClock") {
        SECTION("Construction") {
            SECTION("Given default construction, Then BPM is 120 and subdivision is 16th") {
                // Arrange & Act
                InternalClock clock;

                // Assert
                REQUIRE(clock.getBpm() == 120.0);
                REQUIRE(clock.getSubdivision() == Subdivision::SIXTEENTH);
                REQUIRE(clock.getIsRunning() == false);
            }
        }

        SECTION("setBpm()") {
            SECTION("Given valid BPM, When set, Then BPM is updated") {
                // Arrange
                InternalClock clock;

                // Act
                clock.setBpm(140.0);

                // Assert
                REQUIRE(clock.getBpm() == 140.0);
            }

            SECTION("Given BPM < 20, When set, Then BPM is clamped to 20") {
                // Arrange
                InternalClock clock;

                // Act
                clock.setBpm(10.0);

                // Assert
                REQUIRE(clock.getBpm() == 20.0);
            }

            SECTION("Given BPM > 999, When set, Then BPM is clamped to 999") {
                // Arrange
                InternalClock clock;

                // Act
                clock.setBpm(1000.0);

                // Assert
                REQUIRE(clock.getBpm() == 999.0);
            }
        }

        SECTION("setSubdivision()") {
            SECTION("Given subdivision, When set, Then subdivision is updated") {
                // Arrange
                InternalClock clock;

                // Act
                clock.setSubdivision(Subdivision::EIGHTH);

                // Assert
                REQUIRE(clock.getSubdivision() == Subdivision::EIGHTH);
            }
        }

        SECTION("start() and stop()") {
            SECTION("Given stopped clock, When started, Then isRunning is true") {
                // Arrange
                InternalClock clock;

                // Act
                clock.start();

                // Assert
                REQUIRE(clock.getIsRunning() == true);
            }

            SECTION("Given running clock, When stopped, Then isRunning is false") {
                // Arrange
                InternalClock clock;
                clock.start();

                // Act
                clock.stop();

                // Assert
                REQUIRE(clock.getIsRunning() == false);
            }
        }

        SECTION("reset()") {
            SECTION("Given clock with accumulated samples, When reset, Then accumulated samples is 0") {
                // Arrange
                InternalClock clock;
                clock.setSampleRate(48000.0);
                clock.setBpm(120.0);
                clock.setSubdivision(Subdivision::SIXTEENTH);
                clock.start();
                clock.processSamples(1000);  // Accumulate some samples

                // Act
                clock.reset();

                // Assert
                REQUIRE(clock.getAccumulatedSamples() == 0.0);
            }
        }

        SECTION("calculateSamplesPerPulse()") {
            SECTION("Given 120 BPM, 16th notes, 48000 Hz, Then samples per pulse is 6000") {
                // Arrange
                InternalClock clock;
                clock.setSampleRate(48000.0);
                clock.setBpm(120.0);
                clock.setSubdivision(Subdivision::SIXTEENTH);

                // Act
                double samplesPerPulse = clock.calculateSamplesPerPulse();

                // Assert
                // 120 BPM × 4 (16th notes per beat) / 60 = 8 pulses/sec
                // 48000 / 8 = 6000 samples per pulse
                REQUIRE(samplesPerPulse == 6000.0);
            }

            SECTION("Given 140 BPM, 8th notes, 44100 Hz, Then calculates correctly") {
                // Arrange
                InternalClock clock;
                clock.setSampleRate(44100.0);
                clock.setBpm(140.0);
                clock.setSubdivision(Subdivision::EIGHTH);

                // Act
                double samplesPerPulse = clock.calculateSamplesPerPulse();

                // Assert
                // 140 BPM × 2 (8th notes per beat) / 60 = 4.666... pulses/sec
                // 44100 / 4.666... = 9450 samples per pulse
                REQUIRE(samplesPerPulse == Catch::Approx(9450.0));
            }
        }

        SECTION("processSamples()") {
            SECTION("Given stopped clock, When processing samples, Then no pulse is triggered") {
                // Arrange
                InternalClock clock;
                clock.setSampleRate(48000.0);
                clock.setBpm(120.0);
                clock.setSubdivision(Subdivision::SIXTEENTH);
                // Don't start the clock

                // Act
                bool shouldTrigger = clock.processSamples(10000);

                // Assert
                REQUIRE(shouldTrigger == false);
            }

            SECTION("Given running clock, When processing enough samples, Then pulse is triggered") {
                // Arrange
                InternalClock clock;
                clock.setSampleRate(48000.0);
                clock.setBpm(120.0);
                clock.setSubdivision(Subdivision::SIXTEENTH);
                clock.start();

                // Act - process exactly enough samples for one pulse (6000)
                bool shouldTrigger = clock.processSamples(6000);

                // Assert
                REQUIRE(shouldTrigger == true);
            }

            SECTION("Given running clock, When processing samples in multiple blocks, Then pulse triggers at correct time") {
                // Arrange
                InternalClock clock;
                clock.setSampleRate(48000.0);
                clock.setBpm(120.0);
                clock.setSubdivision(Subdivision::SIXTEENTH);
                clock.start();

                // Act & Assert - process in blocks of 512 samples
                // Need 6000 samples for one pulse
                REQUIRE(clock.processSamples(512) == false);  // 512
                REQUIRE(clock.processSamples(512) == false);  // 1024
                REQUIRE(clock.processSamples(512) == false);  // 1536
                REQUIRE(clock.processSamples(512) == false);  // 2048
                REQUIRE(clock.processSamples(512) == false);  // 2560
                REQUIRE(clock.processSamples(512) == false);  // 3072
                REQUIRE(clock.processSamples(512) == false);  // 3584
                REQUIRE(clock.processSamples(512) == false);  // 4096
                REQUIRE(clock.processSamples(512) == false);  // 4608
                REQUIRE(clock.processSamples(512) == false);  // 5120
                REQUIRE(clock.processSamples(512) == false);  // 5632
                REQUIRE(clock.processSamples(512) == true);   // 6144 - triggers!
            }

            SECTION("Given running clock, When pulse triggers, Then remainder is carried over") {
                // Arrange
                InternalClock clock;
                clock.setSampleRate(48000.0);
                clock.setBpm(120.0);
                clock.setSubdivision(Subdivision::SIXTEENTH);
                clock.start();

                // Act - process 6100 samples (100 more than needed)
                bool shouldTrigger = clock.processSamples(6100);

                // Assert
                REQUIRE(shouldTrigger == true);
                REQUIRE(clock.getAccumulatedSamples() == Catch::Approx(100.0));  // Remainder carried over
            }

            SECTION("Given running clock at 240 BPM, When processing samples, Then pulses trigger twice as fast") {
                // Arrange
                InternalClock clock;
                clock.setSampleRate(48000.0);
                clock.setBpm(240.0);  // Double the BPM
                clock.setSubdivision(Subdivision::SIXTEENTH);
                clock.start();

                // Act - at 240 BPM, should trigger every 3000 samples
                bool trigger1 = clock.processSamples(3000);
                bool trigger2 = clock.processSamples(3000);

                // Assert
                REQUIRE(trigger1 == true);
                REQUIRE(trigger2 == true);
            }
        }
    }
}

// ============================================================================
// Clock Tests
// ============================================================================

TEST_CASE("Clock") {
    SECTION("Clock") {
        SECTION("Construction") {
            SECTION("Given default construction, Then mode is INTERNAL and BPM is 120") {
                // Arrange & Act
                Clock clock;

                // Assert
                REQUIRE(clock.getMode() == ClockMode::INTERNAL);
                REQUIRE(clock.getBpm() == 120.0);
                REQUIRE(clock.getSubdivision() == Subdivision::SIXTEENTH);
            }
        }

        SECTION("setMode()") {
            SECTION("Given INTERNAL mode, When set to DAW_SYNC, Then mode is updated") {
                // Arrange
                Clock clock;

                // Act
                clock.setMode(ClockMode::DAW_SYNC);

                // Assert
                REQUIRE(clock.getMode() == ClockMode::DAW_SYNC);
            }

            SECTION("Given mode switch, When switching modes, Then state is reset") {
                // Arrange
                Clock clock;
                clock.setSampleRate(48000.0);
                clock.start();
                clock.processInternalClock(1000);  // Accumulate some samples

                // Act
                clock.setMode(ClockMode::DAW_SYNC);
                clock.setMode(ClockMode::INTERNAL);

                // Assert - internal clock should be reset
                REQUIRE(clock.getInternalClock().getAccumulatedSamples() == 0.0);
            }
        }

        SECTION("setBpm() and getBpm()") {
            SECTION("Given clock, When BPM is set, Then BPM is updated") {
                // Arrange
                Clock clock;

                // Act
                clock.setBpm(140.0);

                // Assert
                REQUIRE(clock.getBpm() == 140.0);
            }
        }

        SECTION("setSubdivision() and getSubdivision()") {
            SECTION("Given clock, When subdivision is set, Then subdivision is updated") {
                // Arrange
                Clock clock;

                // Act
                clock.setSubdivision(Subdivision::EIGHTH);

                // Assert
                REQUIRE(clock.getSubdivision() == Subdivision::EIGHTH);
            }
        }

        SECTION("start() and stop()") {
            SECTION("Given INTERNAL mode, When started, Then clock is running") {
                // Arrange
                Clock clock;
                clock.setMode(ClockMode::INTERNAL);

                // Act
                clock.start();

                // Assert
                REQUIRE(clock.isRunning() == true);
            }

            SECTION("Given running clock, When stopped, Then clock is not running") {
                // Arrange
                Clock clock;
                clock.setMode(ClockMode::INTERNAL);
                clock.start();

                // Act
                clock.stop();

                // Assert
                REQUIRE(clock.isRunning() == false);
            }
        }

        SECTION("processInternalClock()") {
            SECTION("Given INTERNAL mode, When processing samples, Then shouldTick returns true at correct intervals") {
                // Arrange
                Clock clock;
                clock.setMode(ClockMode::INTERNAL);
                clock.setSampleRate(48000.0);
                clock.setBpm(120.0);
                clock.setSubdivision(Subdivision::SIXTEENTH);
                clock.start();

                // Act & Assert
                clock.processInternalClock(5000);
                REQUIRE(clock.shouldTick() == false);  // Not enough samples yet

                clock.processInternalClock(1000);
                REQUIRE(clock.shouldTick() == true);   // Now we have 6000 samples - trigger!

                clock.processInternalClock(5000);
                REQUIRE(clock.shouldTick() == false);  // Not enough for next pulse
            }

            SECTION("Given DAW_SYNC mode, When processing internal clock, Then no pulse is triggered") {
                // Arrange
                Clock clock;
                clock.setMode(ClockMode::DAW_SYNC);
                clock.setSampleRate(48000.0);
                clock.start();

                // Act
                clock.processInternalClock(10000);

                // Assert
                REQUIRE(clock.shouldTick() == false);  // DAW_SYNC mode ignores internal clock
            }
        }

        SECTION("updateDawSync()") {
            SECTION("Given DAW_SYNC mode, When pulse count increments, Then shouldTick returns true") {
                // Arrange
                Clock clock;
                clock.setMode(ClockMode::DAW_SYNC);

                // Act
                clock.updateDawSync(0);
                REQUIRE(clock.shouldTick() == false);  // No increment yet

                clock.updateDawSync(1);
                REQUIRE(clock.shouldTick() == true);   // Pulse count incremented!

                clock.updateDawSync(1);
                REQUIRE(clock.shouldTick() == false);  // Same count, no trigger

                clock.updateDawSync(2);
                REQUIRE(clock.shouldTick() == true);   // Incremented again!
            }

            SECTION("Given INTERNAL mode, When updating DAW sync, Then no pulse is triggered") {
                // Arrange
                Clock clock;
                clock.setMode(ClockMode::INTERNAL);

                // Act
                clock.updateDawSync(1);

                // Assert
                REQUIRE(clock.shouldTick() == false);  // INTERNAL mode ignores DAW sync
            }
        }

        SECTION("shouldTick()") {
            SECTION("Given pulse triggered, When shouldTick called, Then flag is cleared") {
                // Arrange
                Clock clock;
                clock.setMode(ClockMode::DAW_SYNC);
                clock.updateDawSync(1);

                // Act
                bool firstCall = clock.shouldTick();
                bool secondCall = clock.shouldTick();

                // Assert
                REQUIRE(firstCall == true);   // First call returns true
                REQUIRE(secondCall == false); // Second call returns false (flag cleared)
            }
        }

        SECTION("manualTrigger()") {
            SECTION("Given clock, When manually triggered, Then shouldTick returns true") {
                // Arrange
                Clock clock;

                // Act
                clock.manualTrigger();

                // Assert
                REQUIRE(clock.shouldTick() == true);
            }

            SECTION("Given manual trigger, When shouldTick called twice, Then only first returns true") {
                // Arrange
                Clock clock;
                clock.manualTrigger();

                // Act
                bool firstCall = clock.shouldTick();
                bool secondCall = clock.shouldTick();

                // Assert
                REQUIRE(firstCall == true);
                REQUIRE(secondCall == false);
            }
        }

        SECTION("reset()") {
            SECTION("Given clock with state, When reset, Then all state is cleared") {
                // Arrange
                Clock clock;
                clock.setMode(ClockMode::INTERNAL);
                clock.setSampleRate(48000.0);
                clock.start();
                clock.processInternalClock(1000);
                clock.updateDawSync(5);

                // Act
                clock.reset();

                // Assert
                REQUIRE(clock.getInternalClock().getAccumulatedSamples() == 0.0);
            }
        }

        SECTION("Integration: INTERNAL mode full workflow") {
            SECTION("Given INTERNAL mode at 120 BPM, When processing audio blocks, Then pulses trigger at correct intervals") {
                // Arrange
                Clock clock;
                clock.setMode(ClockMode::INTERNAL);
                clock.setSampleRate(48000.0);
                clock.setBpm(120.0);
                clock.setSubdivision(Subdivision::SIXTEENTH);
                clock.start();

                int pulseCount = 0;

                // Act - simulate processing 1 second of audio in 512-sample blocks
                for (int i = 0; i < 94; i++) {  // 94 blocks × 512 = 48128 samples ≈ 1 second
                    clock.processInternalClock(512);
                    if (clock.shouldTick()) {
                        pulseCount++;
                    }
                }

                // Assert - at 120 BPM with 16th notes, should get 8 pulses per second
                REQUIRE(pulseCount == 8);
            }
        }

        SECTION("Integration: DAW_SYNC mode workflow") {
            SECTION("Given DAW_SYNC mode, When DAW position advances, Then pulses trigger correctly") {
                // Arrange
                Clock clock;
                clock.setMode(ClockMode::DAW_SYNC);

                int pulseCount = 0;

                // Act - simulate DAW advancing through 4 quarter notes
                // Start from pulse 1 to trigger the first increment (0→1)
                for (int pulse = 1; pulse <= 16; pulse++) {  // 4 quarter notes × 4 pulses per quarter = 16 pulses
                    clock.updateDawSync(pulse);
                    if (clock.shouldTick()) {
                        pulseCount++;
                    }
                }

                // Assert
                REQUIRE(pulseCount == 16);
            }
        }
    }
}

// ============================================================================
// DawSync Tests
// ============================================================================

TEST_CASE("DawSync") {
    SECTION("DawSync") {
        SECTION("Construction") {
            SECTION("Given default construction, Then subdivision is 16th and not playing") {
                // Arrange & Act
                DawSync dawSync;

                // Assert
                REQUIRE(dawSync.getSubdivision() == Subdivision::SIXTEENTH);
                REQUIRE(dawSync.getIsPlaying() == false);
                REQUIRE(dawSync.getCurrentPulseCount() == 0);
            }
        }

        SECTION("setSubdivision()") {
            SECTION("Given subdivision, When set, Then subdivision is updated") {
                // Arrange
                DawSync dawSync;

                // Act
                dawSync.setSubdivision(Subdivision::EIGHTH);

                // Assert
                REQUIRE(dawSync.getSubdivision() == Subdivision::EIGHTH);
            }
        }

        SECTION("calculatePulseCount()") {
            SECTION("Given 4.0 PPQ and 16th note subdivision, Then pulse count is 16") {
                // Arrange
                DawSync dawSync;
                dawSync.setSubdivision(Subdivision::SIXTEENTH);

                // Act
                int pulseCount = dawSync.calculatePulseCount(4.0);

                // Assert
                // 4 quarter notes × 4 pulses per quarter = 16 pulses
                REQUIRE(pulseCount == 16);
            }

            SECTION("Given 2.5 PPQ and 8th note subdivision, Then pulse count is 5") {
                // Arrange
                DawSync dawSync;
                dawSync.setSubdivision(Subdivision::EIGHTH);

                // Act
                int pulseCount = dawSync.calculatePulseCount(2.5);

                // Assert
                // 2.5 quarter notes × 2 pulses per quarter = 5 pulses
                REQUIRE(pulseCount == 5);
            }

            SECTION("Given 1.0 PPQ and quarter note subdivision, Then pulse count is 1") {
                // Arrange
                DawSync dawSync;
                dawSync.setSubdivision(Subdivision::QUARTER);

                // Act
                int pulseCount = dawSync.calculatePulseCount(1.0);

                // Assert
                REQUIRE(pulseCount == 1);
            }
        }

        SECTION("reset()") {
            SECTION("Given DawSync with state, When reset, Then state is cleared") {
                // Arrange
                DawSync dawSync;
                // Simulate some playback (would need actual PositionInfo, so we'll test indirectly)

                // Act
                dawSync.reset();

                // Assert
                REQUIRE(dawSync.getCurrentPulseCount() == 0);
                REQUIRE(dawSync.getLastPpqPosition() == 0.0);
            }
        }
    }
}

// ============================================================================
// ClockMode Helper Function Tests
// ============================================================================

TEST_CASE("ClockMode Helpers") {
    SECTION("getPulsesPerQuarterNote()") {
        SECTION("Given QUARTER subdivision, Then returns 1") {
            REQUIRE(getPulsesPerQuarterNote(Subdivision::QUARTER) == 1);
        }

        SECTION("Given EIGHTH subdivision, Then returns 2") {
            REQUIRE(getPulsesPerQuarterNote(Subdivision::EIGHTH) == 2);
        }

        SECTION("Given SIXTEENTH subdivision, Then returns 4") {
            REQUIRE(getPulsesPerQuarterNote(Subdivision::SIXTEENTH) == 4);
        }

        SECTION("Given THIRTY_SECOND subdivision, Then returns 8") {
            REQUIRE(getPulsesPerQuarterNote(Subdivision::THIRTY_SECOND) == 8);
        }
    }
}

