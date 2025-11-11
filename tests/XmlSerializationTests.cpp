#include <catch2/catch_test_macros.hpp>
#include "../source/backend/Sequence.h"
#include "../source/backend/Step.h"
#include "../source/backend/StepNotes.h"
#include "../source/backend/StepModulations.h"
#include "../source/backend/Scale.h"
#include "../source/backend/StepNote.h"
#include "../source/backend/StepModulation.h"

SCENARIO("Scale XML serialization", "[xml][scale]") {
    GIVEN("A Scale with custom settings") {
        Scale scale;
        scale.setRangeMin(24);
        scale.setRangeMax(96);
        
        std::array<int, 12> scaleNotes = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1}; // Major scale
        scale.setScaleNotes(scaleNotes);
        
        WHEN("Serialized to XML and deserialized back") {
            auto xml = scale.toXml();
            
            Scale deserializedScale;
            deserializedScale.fromXml(*xml);
            
            THEN("All properties are preserved") {
                REQUIRE(deserializedScale.getRangeMin() == 24);
                REQUIRE(deserializedScale.getRangeMax() == 96);
                
                auto deserializedNotes = deserializedScale.getScaleNotes();
                for (int i = 0; i < 12; i++) {
                    REQUIRE(deserializedNotes[i] == scaleNotes[i]);
                }
            }
        }
    }
}

SCENARIO("StepNote XML serialization", "[xml][stepnote]") {
    GIVEN("A StepNote with custom values") {
        StepNote note(72, true);
        
        WHEN("Serialized to XML and deserialized back") {
            auto xml = note.toXml();
            
            StepNote deserializedNote;
            deserializedNote.fromXml(*xml);
            
            THEN("All properties are preserved") {
                REQUIRE(deserializedNote.getNoteNumber() == 72);
                REQUIRE(deserializedNote.isSelected() == true);
            }
        }
    }
}

SCENARIO("StepModulation XML serialization", "[xml][stepmodulation]") {
    GIVEN("A StepModulation with custom values") {
        StepModulation mod(ModulationType::CC, 7, 100);
        mod.setSelected(true);
        
        WHEN("Serialized to XML and deserialized back") {
            auto xml = mod.toXml();
            
            StepModulation deserializedMod;
            deserializedMod.fromXml(*xml);
            
            THEN("All properties are preserved") {
                REQUIRE(deserializedMod.getType() == ModulationType::CC);
                REQUIRE(deserializedMod.getCCNumber() == 7);
                REQUIRE(deserializedMod.getValue() == 100);
                REQUIRE(deserializedMod.isSelected() == true);
            }
        }
    }
    
    GIVEN("A Velocity modulation") {
        StepModulation mod(ModulationType::Velocity, 0, 127);
        
        WHEN("Serialized to XML and deserialized back") {
            auto xml = mod.toXml();
            
            StepModulation deserializedMod;
            deserializedMod.fromXml(*xml);
            
            THEN("Type is preserved correctly") {
                REQUIRE(deserializedMod.getType() == ModulationType::Velocity);
                REQUIRE(deserializedMod.getValue() == 127);
            }
        }
    }
}

SCENARIO("StepNotes XML serialization", "[xml][stepnotes]") {
    GIVEN("StepNotes with multiple notes and custom length") {
        StepNotes stepNotes;
        stepNotes.setLength(4);
        stepNotes.setNote(0, 60);
        stepNotes.setNote(1, 64);
        stepNotes.setNote(2, 67);
        stepNotes.setNote(3, 72);
        
        // Advance to index 2
        stepNotes.tick();
        stepNotes.tick();
        stepNotes.tick();
        
        WHEN("Serialized to XML and deserialized back") {
            auto xml = stepNotes.toXml();
            
            StepNotes deserializedStepNotes;
            deserializedStepNotes.fromXml(*xml);
            
            THEN("All properties are preserved") {
                REQUIRE(deserializedStepNotes.getLength() == 4);
                REQUIRE(deserializedStepNotes.getCurrentIndex() == 2);
                REQUIRE(deserializedStepNotes.getNote(0).getNoteNumber() == 60);
                REQUIRE(deserializedStepNotes.getNote(1).getNoteNumber() == 64);
                REQUIRE(deserializedStepNotes.getNote(2).getNoteNumber() == 67);
                REQUIRE(deserializedStepNotes.getNote(3).getNoteNumber() == 72);
            }
        }
    }
}

SCENARIO("StepModulations XML serialization", "[xml][stepmodulations]") {
    GIVEN("StepModulations with custom values") {
        StepModulations stepMods;
        stepMods.setModulationValue(0, 127);  // Velocity
        stepMods.setModulationSelected(0, true);
        stepMods.setModulationType(1, ModulationType::CC);
        stepMods.setModulationCCNumber(1, 7);
        stepMods.setModulationValue(1, 64);
        stepMods.setModulationSelected(1, true);
        
        WHEN("Serialized to XML and deserialized back") {
            auto xml = stepMods.toXml();
            
            StepModulations deserializedStepMods;
            deserializedStepMods.fromXml(*xml);
            
            THEN("All modulations are preserved") {
                REQUIRE(deserializedStepMods.getModulation(0).getValue() == 127);
                REQUIRE(deserializedStepMods.getModulation(0).isSelected() == true);
                REQUIRE(deserializedStepMods.getModulation(1).getType() == ModulationType::CC);
                REQUIRE(deserializedStepMods.getModulation(1).getCCNumber() == 7);
                REQUIRE(deserializedStepMods.getModulation(1).getValue() == 64);
                REQUIRE(deserializedStepMods.getModulation(1).isSelected() == true);
            }
        }
    }
}

SCENARIO("Step XML serialization", "[xml][step]") {
    GIVEN("A Step with custom StepNotes and StepModulations") {
        Step step;
        step.getStepNotes().setLength(3);
        step.getStepNotes().setNote(0, 60);
        step.getStepNotes().setNote(1, 64);
        step.getStepNotes().setNote(2, 67);
        
        step.getStepModulations().setModulationValue(0, 100);
        step.getStepModulations().setModulationSelected(0, true);
        
        WHEN("Serialized to XML and deserialized back") {
            auto xml = step.toXml();
            
            Step deserializedStep;
            deserializedStep.fromXml(*xml);
            
            THEN("StepNotes are preserved") {
                REQUIRE(deserializedStep.getStepNotes().getLength() == 3);
                REQUIRE(deserializedStep.getStepNotes().getNote(0).getNoteNumber() == 60);
                REQUIRE(deserializedStep.getStepNotes().getNote(1).getNoteNumber() == 64);
                REQUIRE(deserializedStep.getStepNotes().getNote(2).getNoteNumber() == 67);
            }
            
            THEN("StepModulations are preserved") {
                REQUIRE(deserializedStep.getStepModulations().getModulation(0).getValue() == 100);
                REQUIRE(deserializedStep.getStepModulations().getModulation(0).isSelected() == true);
            }
        }
    }
}

SCENARIO("Sequence XML serialization", "[xml][sequence]") {
    GIVEN("A Sequence with custom configuration") {
        Sequence sequence;
        sequence.setLength(16);
        sequence.setIsRandom(false);  // Keep notes stable for testing
        sequence.setIsShift(false);

        // Configure scale
        std::array<int, 12> scaleNotes = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};
        sequence.getScale().setScaleNotes(scaleNotes);
        sequence.getScale().setRangeMin(36);
        sequence.getScale().setRangeMax(84);

        // Configure play probability
        sequence.setPlayProbability(0, true);
        sequence.setPlayProbability(1, false);
        sequence.setPlayProbability(2, true);

        // Configure some steps
        sequence.getStep(0).getStepNotes().setLength(2);
        sequence.getStep(0).getStepNotes().setNote(0, 60);
        sequence.getStep(0).getStepNotes().setNote(1, 72);

        sequence.getStep(1).getStepNotes().setNote(0, 64);
        sequence.getStep(1).getStepModulations().setModulationValue(0, 127);

        // Advance sequence a few times
        sequence.tick();
        sequence.tick();
        
        WHEN("Serialized to XML and deserialized back") {
            auto xml = sequence.toXml();
            
            Sequence deserializedSequence;
            deserializedSequence.fromXml(*xml);
            
            THEN("Basic properties are preserved") {
                REQUIRE(deserializedSequence.getLength() == 16);
                REQUIRE(deserializedSequence.getIsRandom() == false);
                REQUIRE(deserializedSequence.getIsShift() == false);
                REQUIRE(deserializedSequence.getCurrentIndex() == 1);
            }
            
            THEN("Scale is preserved") {
                REQUIRE(deserializedSequence.getScale().getRangeMin() == 36);
                REQUIRE(deserializedSequence.getScale().getRangeMax() == 84);
                
                auto deserializedNotes = deserializedSequence.getScale().getScaleNotes();
                for (int i = 0; i < 12; i++) {
                    REQUIRE(deserializedNotes[i] == scaleNotes[i]);
                }
            }
            
            THEN("Play probability is preserved") {
                REQUIRE(deserializedSequence.getPlayProbability(0) == true);
                REQUIRE(deserializedSequence.getPlayProbability(1) == false);
                REQUIRE(deserializedSequence.getPlayProbability(2) == true);
            }
            
            THEN("Step data is preserved") {
                REQUIRE(deserializedSequence.getStep(0).getStepNotes().getLength() == 2);
                REQUIRE(deserializedSequence.getStep(0).getStepNotes().getNote(0).getNoteNumber() == 60);
                REQUIRE(deserializedSequence.getStep(0).getStepNotes().getNote(1).getNoteNumber() == 72);
                
                REQUIRE(deserializedSequence.getStep(1).getStepNotes().getNote(0).getNoteNumber() == 64);
                REQUIRE(deserializedSequence.getStep(1).getStepModulations().getModulation(0).getValue() == 127);
            }
        }
    }
}

SCENARIO("Sequence XML serialization - Full 64 steps", "[xml][sequence][full]") {
    GIVEN("A Sequence with all 64 steps configured") {
        Sequence sequence;
        
        // Configure all 64 steps with unique data
        for (int i = 0; i < 64; i++) {
            sequence.getStep(i).getStepNotes().setNote(0, 60 + i % 12);
            sequence.getStep(i).getStepModulations().setModulationValue(0, i * 2);
            sequence.setPlayProbability(i, (i % 3) != 0);  // Every 3rd step is rest
        }
        
        WHEN("Serialized to XML and deserialized back") {
            auto xml = sequence.toXml();
            
            Sequence deserializedSequence;
            deserializedSequence.fromXml(*xml);
            
            THEN("All 64 steps are preserved correctly") {
                for (int i = 0; i < 64; i++) {
                    REQUIRE(deserializedSequence.getStep(i).getStepNotes().getNote(0).getNoteNumber() == 60 + i % 12);
                    REQUIRE(deserializedSequence.getStep(i).getStepModulations().getModulation(0).getValue() == i * 2);
                    REQUIRE(deserializedSequence.getPlayProbability(i) == ((i % 3) != 0));
                }
            }
        }
    }
}

