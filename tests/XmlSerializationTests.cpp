#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../source/backend/Scale.h"
#include "../source/backend/Clock.h"

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

SCENARIO("Scale XML serialization - default values", "[xml][scale]") {
    GIVEN("A Scale with default settings") {
        Scale scale;

        WHEN("Serialized to XML and deserialized back") {
            auto xml = scale.toXml();

            Scale deserializedScale;
            deserializedScale.fromXml(*xml);

            THEN("Default range is preserved") {
                REQUIRE(deserializedScale.getRangeMin() == 0);
                REQUIRE(deserializedScale.getRangeMax() == 127);
            }

            THEN("Default chromatic scale is preserved") {
                auto deserializedNotes = deserializedScale.getScaleNotes();
                for (int i = 0; i < 12; i++) {
                    REQUIRE(deserializedNotes[i] == 1);
                }
            }
        }
    }
}

SCENARIO("Scale XML serialization - edge cases", "[xml][scale]") {
    GIVEN("A Scale with only one note enabled") {
        Scale scale;
        std::array<int, 12> scaleNotes = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Only C
        scale.setScaleNotes(scaleNotes);

        WHEN("Serialized to XML and deserialized back") {
            auto xml = scale.toXml();

            Scale deserializedScale;
            deserializedScale.fromXml(*xml);

            THEN("Single note scale is preserved") {
                auto deserializedNotes = deserializedScale.getScaleNotes();
                REQUIRE(deserializedNotes[0] == 1);
                for (int i = 1; i < 12; i++) {
                    REQUIRE(deserializedNotes[i] == 0);
                }
            }
        }
    }

    GIVEN("A Scale with extreme range values") {
        Scale scale;
        scale.setRangeMin(0);
        scale.setRangeMax(127);

        WHEN("Serialized to XML and deserialized back") {
            auto xml = scale.toXml();

            Scale deserializedScale;
            deserializedScale.fromXml(*xml);

            THEN("Extreme range values are preserved") {
                REQUIRE(deserializedScale.getRangeMin() == 0);
                REQUIRE(deserializedScale.getRangeMax() == 127);
            }
        }
    }
}

SCENARIO("Scale XML deserialization - wrong tag name", "[xml][scale]") {
    GIVEN("An XML element with wrong tag name") {
        juce::XmlElement xml("NotAScale");
        xml.setAttribute("rangeMin", 24);
        xml.setAttribute("rangeMax", 96);

        WHEN("fromXml is called") {
            Scale scale;
            scale.fromXml(xml);

            THEN("Scale is unchanged (defaults)") {
                REQUIRE(scale.getRangeMin() == 0);
                REQUIRE(scale.getRangeMax() == 127);
            }
        }
    }
}

// ============================================================================
// Clock XML Serialization Tests
// ============================================================================

SCENARIO("Clock XML serialization - INTERNAL mode", "[xml][clock]") {
    GIVEN("A Clock in INTERNAL mode with custom settings") {
        Clock clock;
        clock.setMode(ClockMode::INTERNAL);
        clock.setBpm(140.0);
        clock.setSubdivision(Subdivision::EIGHTH);

        WHEN("Serialized to XML and deserialized back") {
            auto xml = clock.toXml();
            REQUIRE(xml != nullptr);
            REQUIRE(xml->getTagName() == "Clock");

            Clock restored;
            restored.fromXml(*xml);

            THEN("Mode is preserved") {
                REQUIRE(restored.getMode() == ClockMode::INTERNAL);
            }

            THEN("BPM is preserved") {
                REQUIRE(restored.getBpm() == Catch::Approx(140.0));
            }

            THEN("Subdivision is preserved") {
                REQUIRE(restored.getSubdivision() == Subdivision::EIGHTH);
            }
        }
    }
}

SCENARIO("Clock XML serialization - DAW_SYNC mode", "[xml][clock]") {
    GIVEN("A Clock in DAW_SYNC mode") {
        Clock clock;
        clock.setMode(ClockMode::DAW_SYNC);
        clock.setBpm(90.0);
        clock.setSubdivision(Subdivision::QUARTER);

        WHEN("Serialized to XML and deserialized back") {
            auto xml = clock.toXml();

            Clock restored;
            restored.fromXml(*xml);

            THEN("DAW_SYNC mode is preserved") {
                REQUIRE(restored.getMode() == ClockMode::DAW_SYNC);
            }

            THEN("BPM is preserved") {
                REQUIRE(restored.getBpm() == Catch::Approx(90.0));
            }

            THEN("Subdivision is preserved") {
                REQUIRE(restored.getSubdivision() == Subdivision::QUARTER);
            }
        }
    }
}

SCENARIO("Clock XML serialization - all subdivisions round-trip", "[xml][clock]") {
    GIVEN("Clocks with different subdivisions") {
        auto subdivisions = {
            Subdivision::QUARTER,
            Subdivision::EIGHTH,
            Subdivision::SIXTEENTH,
            Subdivision::THIRTY_SECOND
        };

        for (auto sub : subdivisions) {
            Clock clock;
            clock.setSubdivision(sub);

            WHEN("Serialized and deserialized") {
                auto xml = clock.toXml();
                Clock restored;
                restored.fromXml(*xml);

                THEN("Subdivision is preserved") {
                    REQUIRE(restored.getSubdivision() == sub);
                }
            }
        }
    }
}

SCENARIO("Clock XML deserialization - wrong tag name", "[xml][clock]") {
    GIVEN("An XML element with wrong tag name") {
        juce::XmlElement xml("NotAClock");
        xml.setAttribute("mode", "internal");
        xml.setAttribute("bpm", 200.0);

        WHEN("fromXml is called") {
            Clock clock;
            clock.fromXml(xml);

            THEN("Clock is unchanged (defaults)") {
                REQUIRE(clock.getMode() == ClockMode::INTERNAL);
                REQUIRE(clock.getBpm() == Catch::Approx(120.0));
            }
        }
    }
}

SCENARIO("Clock XML deserialization - defaults for missing attributes", "[xml][clock]") {
    GIVEN("An XML element with minimal attributes") {
        juce::XmlElement xml("Clock");
        // No mode, bpm, or subdivision attributes

        WHEN("fromXml is called") {
            Clock clock;
            clock.fromXml(xml);

            THEN("Defaults are used") {
                // Default mode string "daw_sync" when not specified
                REQUIRE(clock.getMode() == ClockMode::DAW_SYNC);
                REQUIRE(clock.getBpm() == Catch::Approx(120.0));
                REQUIRE(clock.getSubdivision() == Subdivision::SIXTEENTH);
            }
        }
    }
}
