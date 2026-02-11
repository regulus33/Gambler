#include <catch2/catch_test_macros.hpp>
#include "../source/backend/MidiLearnRegistry.h"

/**
 * MidiLearnRegistry Tests
 *
 * Tests for the MIDI learn system including:
 * - Learn mode state machine (enter/exit/isLearning)
 * - Control registration and unregistration
 * - MIDI message learning and mapping lookup
 * - Mapping clearing (individual and all)
 * - XML serialization/deserialization
 * - MidiMessageIdentifier helpers
 */

// ============================================================================
// MidiMessageIdentifier Tests
// ============================================================================

TEST_CASE("MidiMessageIdentifier") {
    SECTION("toString() formats correctly for CC") {
        MidiMessageIdentifier id;
        id.type = MidiMessageType::CC;
        id.channel = 1;
        id.number = 74;

        REQUIRE(id.toString() == "CH1-CC74");
    }

    SECTION("toString() formats correctly for NOTE") {
        MidiMessageIdentifier id;
        id.type = MidiMessageType::NOTE;
        id.channel = 10;
        id.number = 60;

        REQUIRE(id.toString() == "CH10-N60");
    }

    SECTION("Equality operator works for identical identifiers") {
        MidiMessageIdentifier a{MidiMessageType::CC, 1, 74};
        MidiMessageIdentifier b{MidiMessageType::CC, 1, 74};

        REQUIRE(a == b);
    }

    SECTION("Equality operator returns false for different type") {
        MidiMessageIdentifier a{MidiMessageType::CC, 1, 74};
        MidiMessageIdentifier b{MidiMessageType::NOTE, 1, 74};

        REQUIRE_FALSE(a == b);
    }

    SECTION("Equality operator returns false for different channel") {
        MidiMessageIdentifier a{MidiMessageType::CC, 1, 74};
        MidiMessageIdentifier b{MidiMessageType::CC, 2, 74};

        REQUIRE_FALSE(a == b);
    }

    SECTION("Equality operator returns false for different number") {
        MidiMessageIdentifier a{MidiMessageType::CC, 1, 74};
        MidiMessageIdentifier b{MidiMessageType::CC, 1, 75};

        REQUIRE_FALSE(a == b);
    }

    SECTION("Hash function produces different values for different identifiers") {
        MidiMessageIdentifierHash hasher;
        MidiMessageIdentifier a{MidiMessageType::CC, 1, 74};
        MidiMessageIdentifier b{MidiMessageType::NOTE, 2, 60};

        // Different identifiers should (almost always) have different hashes
        REQUIRE(hasher(a) != hasher(b));
    }
}

// ============================================================================
// MidiLearnRegistry Learn Mode Tests
// ============================================================================

SCENARIO("MidiLearnRegistry learn mode state machine", "[midilearnregistry]") {
    GIVEN("A fresh MidiLearnRegistry") {
        MidiLearnRegistry registry;

        THEN("Learn mode is off by default") {
            REQUIRE(registry.isLearning() == false);
        }

        THEN("No control is learning by default") {
            REQUIRE(registry.getLearningControl() == MidiControlId::COUNT);
        }

        WHEN("Entering learn mode") {
            registry.enterLearnMode();

            THEN("Learn mode is active") {
                REQUIRE(registry.isLearning() == true);
            }

            THEN("No specific control is selected yet") {
                REQUIRE(registry.getLearningControl() == MidiControlId::COUNT);
            }

            WHEN("Setting a learning control") {
                registry.setLearningControl(MidiControlId::OCTAVE_SLIDER);

                THEN("The learning control is set") {
                    REQUIRE(registry.getLearningControl() == MidiControlId::OCTAVE_SLIDER);
                }
            }

            WHEN("Exiting learn mode") {
                registry.exitLearnMode();

                THEN("Learn mode is off") {
                    REQUIRE(registry.isLearning() == false);
                }

                THEN("Learning control is cleared") {
                    REQUIRE(registry.getLearningControl() == MidiControlId::COUNT);
                }
            }
        }
    }
}

// ============================================================================
// Control Registration Tests
// ============================================================================

SCENARIO("MidiLearnRegistry control registration", "[midilearnregistry]") {
    GIVEN("A MidiLearnRegistry") {
        MidiLearnRegistry registry;

        WHEN("Registering a control") {
            registry.registerControl(
                MidiControlId::OCTAVE_SLIDER,
                juce::Rectangle<int>(10, 20, 100, 50),
                "Octave Slider"
            );

            THEN("Control appears in registered controls") {
                auto& controls = registry.getControls();
                REQUIRE(controls.size() == 1);
                auto it = controls.find(MidiControlId::OCTAVE_SLIDER);
                REQUIRE(it != controls.end());
                REQUIRE(it->second.displayName == "Octave Slider");
                REQUIRE(it->second.isActive == true);
            }
        }

        WHEN("Registering multiple controls") {
            registry.registerControl(MidiControlId::OCTAVE_SLIDER, {0, 0, 100, 50}, "Octave");
            registry.registerControl(MidiControlId::NOTE_SLIDER, {0, 60, 100, 50}, "Note");
            registry.registerControl(MidiControlId::PROBABILITY_SLIDER, {0, 120, 100, 50}, "Prob");

            THEN("All controls are registered") {
                REQUIRE(registry.getControls().size() == 3);
            }
        }

        WHEN("Unregistering a control") {
            registry.registerControl(MidiControlId::OCTAVE_SLIDER, {0, 0, 100, 50}, "Octave");
            registry.registerControl(MidiControlId::NOTE_SLIDER, {0, 60, 100, 50}, "Note");
            registry.unregisterControl(MidiControlId::OCTAVE_SLIDER);

            THEN("Control is removed") {
                REQUIRE(registry.getControls().size() == 1);
                REQUIRE(registry.getControls().find(MidiControlId::OCTAVE_SLIDER) == registry.getControls().end());
            }

            THEN("Other controls remain") {
                REQUIRE(registry.getControls().find(MidiControlId::NOTE_SLIDER) != registry.getControls().end());
            }
        }

        WHEN("Updating control bounds") {
            registry.registerControl(MidiControlId::OCTAVE_SLIDER, {0, 0, 100, 50}, "Octave");
            registry.updateControlBounds(MidiControlId::OCTAVE_SLIDER, {10, 20, 200, 100});

            THEN("Bounds are updated") {
                auto& controls = registry.getControls();
                auto it = controls.find(MidiControlId::OCTAVE_SLIDER);
                REQUIRE(it->second.bounds == juce::Rectangle<int>(10, 20, 200, 100));
            }
        }

        WHEN("Updating bounds for non-existent control") {
            // Should not crash
            registry.updateControlBounds(MidiControlId::OCTAVE_SLIDER, {10, 20, 200, 100});

            THEN("No controls are registered") {
                REQUIRE(registry.getControls().empty());
            }
        }
    }
}

// ============================================================================
// MIDI Learning Tests
// ============================================================================

SCENARIO("MidiLearnRegistry MIDI learning", "[midilearnregistry]") {
    GIVEN("A MidiLearnRegistry in learn mode with a control selected") {
        MidiLearnRegistry registry;
        registry.enterLearnMode();
        registry.setLearningControl(MidiControlId::OCTAVE_SLIDER);

        WHEN("A CC message is received") {
            bool learned = registry.learnMidiMessage(MidiMessageType::CC, 1, 74);

            THEN("Mapping is created") {
                REQUIRE(learned == true);
            }

            THEN("Learning control is cleared after learning") {
                REQUIRE(registry.getLearningControl() == MidiControlId::COUNT);
            }

            THEN("Mapping can be found") {
                auto controlId = registry.findUserMapping(MidiMessageType::CC, 1, 74);
                REQUIRE(controlId == MidiControlId::OCTAVE_SLIDER);
            }

            THEN("MIDI identifier can be retrieved for the control") {
                auto* midi = registry.getMidiForControl(MidiControlId::OCTAVE_SLIDER);
                REQUIRE(midi != nullptr);
                REQUIRE(midi->type == MidiMessageType::CC);
                REQUIRE(midi->channel == 1);
                REQUIRE(midi->number == 74);
            }
        }

        WHEN("A NOTE message is received") {
            bool learned = registry.learnMidiMessage(MidiMessageType::NOTE, 1, 60);

            THEN("Mapping is created") {
                REQUIRE(learned == true);
                auto controlId = registry.findUserMapping(MidiMessageType::NOTE, 1, 60);
                REQUIRE(controlId == MidiControlId::OCTAVE_SLIDER);
            }
        }
    }

    GIVEN("A MidiLearnRegistry NOT in learn mode") {
        MidiLearnRegistry registry;

        WHEN("A MIDI message is received") {
            bool learned = registry.learnMidiMessage(MidiMessageType::CC, 1, 74);

            THEN("No mapping is created") {
                REQUIRE(learned == false);
            }
        }
    }

    GIVEN("A MidiLearnRegistry in learn mode but no control selected") {
        MidiLearnRegistry registry;
        registry.enterLearnMode();
        // Don't select a control

        WHEN("A MIDI message is received") {
            bool learned = registry.learnMidiMessage(MidiMessageType::CC, 1, 74);

            THEN("No mapping is created") {
                REQUIRE(learned == false);
            }
        }
    }

    GIVEN("A MidiLearnRegistry with a callback") {
        MidiLearnRegistry registry;
        bool callbackCalled = false;
        registry.onMappingLearned = [&callbackCalled]() {
            callbackCalled = true;
        };

        registry.enterLearnMode();
        registry.setLearningControl(MidiControlId::NOTE_SLIDER);

        WHEN("A mapping is learned") {
            registry.learnMidiMessage(MidiMessageType::CC, 1, 10);

            THEN("Callback is invoked") {
                REQUIRE(callbackCalled == true);
            }
        }
    }
}

// ============================================================================
// Mapping Lookup Tests
// ============================================================================

SCENARIO("MidiLearnRegistry mapping lookup", "[midilearnregistry]") {
    GIVEN("A MidiLearnRegistry with mappings") {
        MidiLearnRegistry registry;

        // Learn CC 74 -> OCTAVE_SLIDER
        registry.enterLearnMode();
        registry.setLearningControl(MidiControlId::OCTAVE_SLIDER);
        registry.learnMidiMessage(MidiMessageType::CC, 1, 74);

        // Learn NOTE 60 -> NOTE_SLIDER
        registry.setLearningControl(MidiControlId::NOTE_SLIDER);
        registry.learnMidiMessage(MidiMessageType::NOTE, 1, 60);

        WHEN("Looking up an existing CC mapping") {
            auto result = registry.findUserMapping(MidiMessageType::CC, 1, 74);

            THEN("Correct control is returned") {
                REQUIRE(result == MidiControlId::OCTAVE_SLIDER);
            }
        }

        WHEN("Looking up an existing NOTE mapping") {
            auto result = registry.findUserMapping(MidiMessageType::NOTE, 1, 60);

            THEN("Correct control is returned") {
                REQUIRE(result == MidiControlId::NOTE_SLIDER);
            }
        }

        WHEN("Looking up a non-existent mapping") {
            auto result = registry.findUserMapping(MidiMessageType::CC, 1, 99);

            THEN("COUNT is returned") {
                REQUIRE(result == MidiControlId::COUNT);
            }
        }

        WHEN("Looking up MIDI for an unmapped control") {
            auto* midi = registry.getMidiForControl(MidiControlId::PROBABILITY_SLIDER);

            THEN("Nullptr is returned") {
                REQUIRE(midi == nullptr);
            }
        }
    }
}

// ============================================================================
// Mapping Clearing Tests
// ============================================================================

SCENARIO("MidiLearnRegistry mapping clearing", "[midilearnregistry]") {
    GIVEN("A MidiLearnRegistry with multiple mappings") {
        MidiLearnRegistry registry;
        registry.enterLearnMode();

        registry.setLearningControl(MidiControlId::OCTAVE_SLIDER);
        registry.learnMidiMessage(MidiMessageType::CC, 1, 74);

        registry.setLearningControl(MidiControlId::NOTE_SLIDER);
        registry.learnMidiMessage(MidiMessageType::CC, 1, 75);

        registry.setLearningControl(MidiControlId::PROBABILITY_SLIDER);
        registry.learnMidiMessage(MidiMessageType::CC, 1, 76);

        WHEN("Clearing a single mapping") {
            registry.clearMapping(MidiControlId::NOTE_SLIDER);

            THEN("That mapping is removed") {
                REQUIRE(registry.findUserMapping(MidiMessageType::CC, 1, 75) == MidiControlId::COUNT);
                REQUIRE(registry.getMidiForControl(MidiControlId::NOTE_SLIDER) == nullptr);
            }

            THEN("Other mappings remain") {
                REQUIRE(registry.findUserMapping(MidiMessageType::CC, 1, 74) == MidiControlId::OCTAVE_SLIDER);
                REQUIRE(registry.findUserMapping(MidiMessageType::CC, 1, 76) == MidiControlId::PROBABILITY_SLIDER);
            }
        }

        WHEN("Clearing all mappings with clearAllMappings()") {
            registry.clearAllMappings();

            THEN("All mappings are removed") {
                REQUIRE(registry.findUserMapping(MidiMessageType::CC, 1, 74) == MidiControlId::COUNT);
                REQUIRE(registry.findUserMapping(MidiMessageType::CC, 1, 75) == MidiControlId::COUNT);
                REQUIRE(registry.findUserMapping(MidiMessageType::CC, 1, 76) == MidiControlId::COUNT);
            }
        }

        WHEN("Clearing all mappings with clearUserMappings()") {
            registry.clearUserMappings();

            THEN("All mappings are removed") {
                REQUIRE(registry.findUserMapping(MidiMessageType::CC, 1, 74) == MidiControlId::COUNT);
                REQUIRE(registry.findUserMapping(MidiMessageType::CC, 1, 75) == MidiControlId::COUNT);
                REQUIRE(registry.findUserMapping(MidiMessageType::CC, 1, 76) == MidiControlId::COUNT);
            }
        }

        WHEN("Clearing a non-existent mapping") {
            // Should not crash
            registry.clearMapping(MidiControlId::BUTTON_ROLL);

            THEN("Other mappings are unaffected") {
                REQUIRE(registry.findUserMapping(MidiMessageType::CC, 1, 74) == MidiControlId::OCTAVE_SLIDER);
            }
        }
    }
}

// ============================================================================
// XML Serialization Tests
// ============================================================================

SCENARIO("MidiLearnRegistry XML serialization", "[midilearnregistry][xml]") {
    GIVEN("A MidiLearnRegistry with mappings") {
        MidiLearnRegistry registry;
        registry.enterLearnMode();

        registry.setLearningControl(MidiControlId::OCTAVE_SLIDER);
        registry.learnMidiMessage(MidiMessageType::CC, 1, 74);

        registry.setLearningControl(MidiControlId::NOTE_SLIDER);
        registry.learnMidiMessage(MidiMessageType::NOTE, 10, 60);

        WHEN("Serialized to XML and deserialized back") {
            auto xml = registry.toXml();
            REQUIRE(xml != nullptr);
            REQUIRE(xml->getTagName() == "MidiLearnMappings");

            MidiLearnRegistry restored;
            restored.fromXml(*xml);

            THEN("CC mapping is preserved") {
                auto controlId = restored.findUserMapping(MidiMessageType::CC, 1, 74);
                REQUIRE(controlId == MidiControlId::OCTAVE_SLIDER);
            }

            THEN("NOTE mapping is preserved") {
                auto controlId = restored.findUserMapping(MidiMessageType::NOTE, 10, 60);
                REQUIRE(controlId == MidiControlId::NOTE_SLIDER);
            }

            THEN("Reverse mapping is preserved") {
                auto* midi = restored.getMidiForControl(MidiControlId::OCTAVE_SLIDER);
                REQUIRE(midi != nullptr);
                REQUIRE(midi->type == MidiMessageType::CC);
                REQUIRE(midi->channel == 1);
                REQUIRE(midi->number == 74);
            }
        }
    }

    GIVEN("A MidiLearnRegistry with no mappings") {
        MidiLearnRegistry registry;

        WHEN("Serialized to XML") {
            auto xml = registry.toXml();

            THEN("XML has no children") {
                REQUIRE(xml != nullptr);
                REQUIRE(xml->getNumChildElements() == 0);
            }
        }
    }

    GIVEN("An XML element with mappings") {
        MidiLearnRegistry original;
        original.enterLearnMode();
        original.setLearningControl(MidiControlId::BUTTON_ROLL);
        original.learnMidiMessage(MidiMessageType::NOTE, 5, 36);
        auto xml = original.toXml();

        WHEN("fromXml is called on a registry with existing mappings") {
            MidiLearnRegistry target;
            target.enterLearnMode();
            target.setLearningControl(MidiControlId::OCTAVE_SLIDER);
            target.learnMidiMessage(MidiMessageType::CC, 1, 1);

            target.fromXml(*xml);

            THEN("Old mappings are replaced") {
                REQUIRE(target.findUserMapping(MidiMessageType::CC, 1, 1) == MidiControlId::COUNT);
            }

            THEN("New mappings are loaded") {
                REQUIRE(target.findUserMapping(MidiMessageType::NOTE, 5, 36) == MidiControlId::BUTTON_ROLL);
            }
        }
    }
}
