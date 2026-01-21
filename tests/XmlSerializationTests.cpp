#include <catch2/catch_test_macros.hpp>
#include "../source/backend/Scale.h"

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
