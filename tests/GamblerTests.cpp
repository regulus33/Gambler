#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "backend/StepNote.h"
#include "backend/StepNotes.h"
#include "backend/StepModulation.h"
#include "backend/StepModulations.h"
#include "backend/Scale.h"
#include "backend/Step.h"
#include "backend/Sequence.h"
#include "helpers/SequenceBuilder.h"
#include <vector>

// ============================================================================
// StepNote Tests
// ============================================================================

TEST_CASE("StepNote") {
    SECTION("Construction") {
        SECTION("Given default construction, Then note is 60 and not selected") {
            // Arrange & Act
            StepNote note;

            // Assert
            REQUIRE(note.getNoteNumber() == 60);
            REQUIRE(note.isSelected() == false);
        }

        SECTION("Given construction with note number, Then note is set correctly") {
            // Arrange & Act
            StepNote note(72);

            // Assert
            REQUIRE(note.getNoteNumber() == 72);
            REQUIRE(note.isSelected() == false);
        }

        SECTION("Given construction with note and selection, Then both are set") {
            // Arrange & Act
            StepNote note(48, true);

            // Assert
            REQUIRE(note.getNoteNumber() == 48);
            REQUIRE(note.isSelected() == true);
        }
    }

    SECTION("setNoteNumber()") {
        SECTION("Given valid MIDI note, When set, Then note is updated") {
            // Arrange
            StepNote note;

            // Act
            note.setNoteNumber(127);

            // Assert
            REQUIRE(note.getNoteNumber() == 127);
        }

        SECTION("Given invalid MIDI note (> 127), When set, Then note is not updated") {
            // Arrange
            StepNote note(60);

            // Act
            note.setNoteNumber(200);

            // Assert
            REQUIRE(note.getNoteNumber() == 60);
        }

        SECTION("Given invalid MIDI note (< 0), When set, Then note is not updated") {
            // Arrange
            StepNote note(60);

            // Act
            note.setNoteNumber(-10);

            // Assert
            REQUIRE(note.getNoteNumber() == 60);
        }
    }

    SECTION("setSelected()") {
        SECTION("Given unselected note, When selected, Then isSelected returns true") {
            // Arrange
            StepNote note;

            // Act
            note.setSelected(true);

            // Assert
            REQUIRE(note.isSelected() == true);
        }

        SECTION("Given selected note, When deselected, Then isSelected returns false") {
            // Arrange
            StepNote note(60, true);

            // Act
            note.setSelected(false);

            // Assert
            REQUIRE(note.isSelected() == false);
        }
    }
}

// ============================================================================
// StepNotes Tests
// ============================================================================

TEST_CASE("StepNotes") {
    SECTION("Construction") {
        SECTION("Given default construction, Then length is 1 and index is -1") {
            // Arrange & Act
            StepNotes stepNotes;

            // Assert
            REQUIRE(stepNotes.getLength() == 1);
            REQUIRE(stepNotes.getCurrentIndex() == -1);
        }

        SECTION("Given default construction, Then all notes are initialized to 60") {
            // Arrange & Act
            StepNotes stepNotes;

            // Assert
            for (int i = 0; i < StepNotes::MAX_LENGTH; i++) {
                REQUIRE(stepNotes.getNote(i).getNoteNumber() == 60);
            }
        }
    }

    SECTION("setNote()") {
        SECTION("Given valid index, When note is set, Then note is updated") {
            // Arrange
            StepNotes stepNotes;

            // Act
            stepNotes.setNote(0, 72);

            // Assert
            REQUIRE(stepNotes.getNote(0).getNoteNumber() == 72);
        }

        SECTION("Given multiple notes, When set, Then all are stored correctly") {
            // Arrange
            StepNotes stepNotes;
            stepNotes.setLength(3);

            // Act
            stepNotes.setNote(0, 60);
            stepNotes.setNote(1, 64);
            stepNotes.setNote(2, 67);

            // Assert
            REQUIRE(stepNotes.getNote(0).getNoteNumber() == 60);
            REQUIRE(stepNotes.getNote(1).getNoteNumber() == 64);
            REQUIRE(stepNotes.getNote(2).getNoteNumber() == 67);
        }
    }

    SECTION("setLength()") {
        SECTION("Given valid length, When set, Then length is updated") {
            // Arrange
            StepNotes stepNotes;

            // Act
            stepNotes.setLength(5);

            // Assert
            REQUIRE(stepNotes.getLength() == 5);
        }

        SECTION("Given length > MAX_LENGTH, When set, Then length is not updated") {
            // Arrange
            StepNotes stepNotes;

            // Act
            stepNotes.setLength(10);

            // Assert
            REQUIRE(stepNotes.getLength() == 1);
        }

        SECTION("Given length < MIN_LENGTH, When set, Then length is not updated") {
            // Arrange
            StepNotes stepNotes;

            // Act
            stepNotes.setLength(0);

            // Assert
            REQUIRE(stepNotes.getLength() == 1);
        }
    }

    SECTION("increaseLength()") {
        SECTION("Given length < MAX_LENGTH, When increased, Then length increments by 1") {
            // Arrange
            StepNotes stepNotes;
            int initialLength = stepNotes.getLength();

            // Act
            stepNotes.increaseLength();

            // Assert
            REQUIRE(stepNotes.getLength() == initialLength + 1);
        }

        SECTION("Given length = MAX_LENGTH, When increased, Then length stays at MAX_LENGTH") {
            // Arrange
            StepNotes stepNotes;
            stepNotes.setLength(StepNotes::MAX_LENGTH);

            // Act
            stepNotes.increaseLength();

            // Assert
            REQUIRE(stepNotes.getLength() == StepNotes::MAX_LENGTH);
        }
    }

    SECTION("decreaseLength()") {
        SECTION("Given length > MIN_LENGTH, When decreased, Then length decrements by 1") {
            // Arrange
            StepNotes stepNotes;
            stepNotes.setLength(5);

            // Act
            stepNotes.decreaseLength();

            // Assert
            REQUIRE(stepNotes.getLength() == 4);
        }

        SECTION("Given length = MIN_LENGTH, When decreased, Then length stays at MIN_LENGTH") {
            // Arrange
            StepNotes stepNotes;
            stepNotes.setLength(StepNotes::MIN_LENGTH);

            // Act
            stepNotes.decreaseLength();

            // Assert
            REQUIRE(stepNotes.getLength() == StepNotes::MIN_LENGTH);
        }
    }

    SECTION("tick()") {
        SECTION("Given StepNotes with 3 notes, When ticked 3 times, Then cycles through all notes") {
            // Arrange
            StepNotes stepNotes;
            stepNotes.setLength(3);
            stepNotes.setNote(0, 60);
            stepNotes.setNote(1, 64);
            stepNotes.setNote(2, 67);

            // Act & Assert
            REQUIRE(stepNotes.tick() == 60);
            REQUIRE(stepNotes.tick() == 64);
            REQUIRE(stepNotes.tick() == 67);
        }

        SECTION("Given StepNotes at last note, When ticked, Then wraps to first note") {
            // Arrange
            StepNotes stepNotes;
            stepNotes.setLength(2);
            stepNotes.setNote(0, 60);
            stepNotes.setNote(1, 70);
            stepNotes.tick();  // -> 60
            stepNotes.tick();  // -> 70

            // Act
            int wrappedNote = stepNotes.tick();

            // Assert
            REQUIRE(wrappedNote == 60);
            REQUIRE(stepNotes.getCurrentIndex() == 0);
        }

        SECTION("Given StepNotes, When ticked, Then updates selection to current note") {
            // Arrange
            StepNotes stepNotes;
            stepNotes.setLength(2);

            // Act
            stepNotes.tick();

            // Assert
            REQUIRE(stepNotes.getNote(0).isSelected() == true);
            REQUIRE(stepNotes.getNote(1).isSelected() == false);
        }

        SECTION("Given StepNotes, When ticked multiple times, Then selection follows current index") {
            // Arrange
            StepNotes stepNotes;
            stepNotes.setLength(3);

            // Act & Assert - first tick
            stepNotes.tick();
            REQUIRE(stepNotes.getNote(0).isSelected() == true);
            REQUIRE(stepNotes.getNote(1).isSelected() == false);
            REQUIRE(stepNotes.getNote(2).isSelected() == false);

            // Act & Assert - second tick
            stepNotes.tick();
            REQUIRE(stepNotes.getNote(0).isSelected() == false);
            REQUIRE(stepNotes.getNote(1).isSelected() == true);
            REQUIRE(stepNotes.getNote(2).isSelected() == false);

            // Act & Assert - third tick
            stepNotes.tick();
            REQUIRE(stepNotes.getNote(0).isSelected() == false);
            REQUIRE(stepNotes.getNote(1).isSelected() == false);
            REQUIRE(stepNotes.getNote(2).isSelected() == true);
        }

        SECTION("Given StepNotes index never returns to -1 after first tick") {
            // Arrange
            StepNotes stepNotes;
            stepNotes.setLength(2);

            // Act - tick through multiple cycles
            for (int cycle = 0; cycle < 10; cycle++) {
                stepNotes.tick();

                // Assert - index should never be -1 after first tick
                REQUIRE(stepNotes.getCurrentIndex() >= 0);
                REQUIRE(stepNotes.getCurrentIndex() < stepNotes.getLength());
            }
        }
    }

    SECTION("reset()") {
        SECTION("Given StepNotes that has been ticked, When reset, Then index returns to -1") {
            // Arrange
            StepNotes stepNotes;
            stepNotes.tick();
            stepNotes.tick();

            // Act
            stepNotes.reset();

            // Assert
            REQUIRE(stepNotes.getCurrentIndex() == -1);
        }

        SECTION("Given StepNotes with selections, When reset, Then all selections are cleared") {
            // Arrange
            StepNotes stepNotes;
            stepNotes.setLength(3);
            stepNotes.tick();

            // Act
            stepNotes.reset();

            // Assert
            for (int i = 0; i < StepNotes::MAX_LENGTH; i++) {
                REQUIRE(stepNotes.getNote(i).isSelected() == false);
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
        SECTION("Given default construction, Then all notes are disabled") {
            // Arrange & Act
            Scale scale;

            // Assert
            for (int i = 0; i < Scale::NUM_CHROMATIC_NOTES; i++) {
                REQUIRE(scale.getScaleNotes()[i] == 0);
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

    SECTION("applyRange()") {
        SECTION("Given range 60-72, When applying to note 80, Then clamps to 72") {
            // Arrange
            Scale scale;
            scale.setRange(60, 72);

            // Act
            int clamped = scale.applyRange(80);

            // Assert
            REQUIRE(clamped == 72);
        }

        SECTION("Given range 60-72, When applying to note 50, Then clamps to 60") {
            // Arrange
            Scale scale;
            scale.setRange(60, 72);

            // Act
            int clamped = scale.applyRange(50);

            // Assert
            REQUIRE(clamped == 60);
        }

        SECTION("Given range 60-72, When applying to note 66, Then returns 66") {
            // Arrange
            Scale scale;
            scale.setRange(60, 72);

            // Act
            int clamped = scale.applyRange(66);

            // Assert
            REQUIRE(clamped == 66);
        }
    }

    SECTION("quantizeAndApplyRange()") {
        SECTION("Given C major scale and range 60-72, When processing 61, Then quantizes and clamps") {
            // Arrange
            Scale scale;
            scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});
            scale.setRange(60, 72);

            // Act
            int result = scale.quantizeAndApplyRange(61);

            // Assert
            REQUIRE(result == 60);  // C# quantized to C
        }

        SECTION("Given C major scale and range 60-72, When processing 80, Then quantizes and clamps") {
            // Arrange
            Scale scale;
            scale.setScaleFromNames({"C", "D", "E", "F", "G", "A", "B"});
            scale.setRange(60, 72);

            // Act
            int result = scale.quantizeAndApplyRange(80);

            // Assert
            REQUIRE(result == 72);  // Clamped to max
        }
    }
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
        SECTION("Given default construction, Then has 64 steps and length 64") {
            // Arrange & Act
            Sequence sequence;

            // Assert
            REQUIRE(sequence.getLength() == 64);
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
            REQUIRE(sequence.getLength() == 64);
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
        SECTION("Given step with probability false, When ticked, Then returns -1 (rest)") {
            // Arrange
            Sequence sequence;
            sequence.setLength(4);
            sequence.setPlayProbability(0, true);
            sequence.setPlayProbability(1, false);  // Rest
            sequence.setPlayProbability(2, true);
            sequence.setPlayProbability(3, true);

            // Act & Assert
            int note0 = sequence.tick();
            REQUIRE(note0 != -1);  // Should play

            int note1 = sequence.tick();
            REQUIRE(note1 == -1);  // Should rest

            int note2 = sequence.tick();
            REQUIRE(note2 != -1);  // Should play
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
        SECTION("Given 100%, When set, Then all steps should play") {
            // Arrange
            Sequence sequence;
            sequence.setLength(8);

            // Act
            sequence.setProbabilityPercentage(100);

            // Assert
            int trueCount = 0;
            for (int i = 0; i < 8; i++) {
                if (sequence.getPlayProbability(i)) trueCount++;
            }
            REQUIRE(trueCount == 8);
        }

        SECTION("Given 0%, When set, Then no steps should play") {
            // Arrange
            Sequence sequence;
            sequence.setLength(8);

            // Act
            sequence.setProbabilityPercentage(0);

            // Assert
            int trueCount = 0;
            for (int i = 0; i < 8; i++) {
                if (sequence.getPlayProbability(i)) trueCount++;
            }
            REQUIRE(trueCount == 0);
        }

        SECTION("Given 50%, When set, Then approximately half should play") {
            // Arrange
            Sequence sequence;
            sequence.setLength(8);

            // Act
            sequence.setProbabilityPercentage(50);

            // Assert
            int trueCount = 0;
            for (int i = 0; i < 8; i++) {
                if (sequence.getPlayProbability(i)) trueCount++;
            }
            REQUIRE(trueCount == 4);  // Exactly 50% of 8
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
        SECTION("Given sequence with 50% probability and substeps, When ticked, Then respects both") {
            // Arrange
            auto sequence = SequenceBuilder()
                .withLength(4)
                .withStep(0, {60, 61})
                .withStep(1, {62, 63})
                .withStep(2, {64, 65})
                .withStep(3, {66, 67})
                .build();

            // Set specific probability pattern
            sequence.setPlayProbability(0, true);
            sequence.setPlayProbability(1, false);  // Rest
            sequence.setPlayProbability(2, true);
            sequence.setPlayProbability(3, false);  // Rest

            // Act & Assert - First pass
            REQUIRE(sequence.tick() == 60);   // step 0, plays
            REQUIRE(sequence.tick() == -1);   // step 1, rest
            REQUIRE(sequence.tick() == 64);   // step 2, plays
            REQUIRE(sequence.tick() == -1);   // step 3, rest

            // Second pass - substeps should advance even for playing steps
            REQUIRE(sequence.tick() == 61);   // step 0, substep 1
            REQUIRE(sequence.tick() == -1);   // step 1, rest (but substep advances internally)
            REQUIRE(sequence.tick() == 65);   // step 2, substep 1
            REQUIRE(sequence.tick() == -1);   // step 3, rest
        }
    }
}
