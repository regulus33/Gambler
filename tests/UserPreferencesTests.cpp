#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../source/backend/UserPreferences.h"

/**
 * UserPreferences Tests
 *
 * Tests for user preference storage and XML serialization
 */

SCENARIO("UserPreferences default construction", "[userpreferences]") {
    GIVEN("Default-constructed UserPreferences") {
        UserPreferences prefs;

        THEN("All preferences have sensible defaults") {
            REQUIRE(prefs.previewNotesOnEdit == true);
            REQUIRE(prefs.followStep == true);
            REQUIRE(prefs.followPlayback == true);
            REQUIRE(prefs.defaultNoteLength == Catch::Approx(0.9));
            REQUIRE(prefs.midiClockOutputEnabled == false);
        }
    }
}

SCENARIO("UserPreferences modification", "[userpreferences]") {
    GIVEN("UserPreferences with default values") {
        UserPreferences prefs;

        WHEN("previewNotesOnEdit is disabled") {
            prefs.previewNotesOnEdit = false;

            THEN("Value is updated") {
                REQUIRE(prefs.previewNotesOnEdit == false);
            }
        }

        WHEN("followStep is disabled") {
            prefs.followStep = false;

            THEN("Value is updated") {
                REQUIRE(prefs.followStep == false);
            }
        }

        WHEN("followPlayback is disabled") {
            prefs.followPlayback = false;

            THEN("Value is updated") {
                REQUIRE(prefs.followPlayback == false);
            }
        }

        WHEN("defaultNoteLength is changed") {
            prefs.defaultNoteLength = 0.5;

            THEN("Value is updated") {
                REQUIRE(prefs.defaultNoteLength == Catch::Approx(0.5));
            }
        }

        WHEN("midiClockOutputEnabled is enabled") {
            prefs.midiClockOutputEnabled = true;

            THEN("Value is updated") {
                REQUIRE(prefs.midiClockOutputEnabled == true);
            }
        }
    }
}

SCENARIO("UserPreferences XML serialization", "[userpreferences][xml]") {
    GIVEN("UserPreferences with default values") {
        UserPreferences prefs;

        WHEN("Serialized to XML and deserialized") {
            auto xml = prefs.toXml();
            UserPreferences deserializedPrefs;
            deserializedPrefs.fromXml(*xml);

            THEN("All properties are preserved") {
                REQUIRE(deserializedPrefs.previewNotesOnEdit == prefs.previewNotesOnEdit);
                REQUIRE(deserializedPrefs.followStep == prefs.followStep);
                REQUIRE(deserializedPrefs.followPlayback == prefs.followPlayback);
                REQUIRE(deserializedPrefs.defaultNoteLength == Catch::Approx(prefs.defaultNoteLength));
                REQUIRE(deserializedPrefs.midiClockOutputEnabled == prefs.midiClockOutputEnabled);
            }
        }
    }

    GIVEN("UserPreferences with custom values") {
        UserPreferences prefs;
        prefs.previewNotesOnEdit = false;
        prefs.followStep = false;
        prefs.followPlayback = false;
        prefs.defaultNoteLength = 0.75;
        prefs.midiClockOutputEnabled = true;

        WHEN("Serialized to XML and deserialized") {
            auto xml = prefs.toXml();
            UserPreferences deserializedPrefs;
            deserializedPrefs.fromXml(*xml);

            THEN("All custom properties are preserved") {
                REQUIRE(deserializedPrefs.previewNotesOnEdit == false);
                REQUIRE(deserializedPrefs.followStep == false);
                REQUIRE(deserializedPrefs.followPlayback == false);
                REQUIRE(deserializedPrefs.defaultNoteLength == Catch::Approx(0.75));
                REQUIRE(deserializedPrefs.midiClockOutputEnabled == true);
            }
        }
    }

    GIVEN("UserPreferences with boundary values") {
        UserPreferences prefs;
        prefs.defaultNoteLength = 0.0;  // Minimum

        WHEN("Serialized to XML and deserialized") {
            auto xml = prefs.toXml();
            UserPreferences deserializedPrefs;
            deserializedPrefs.fromXml(*xml);

            THEN("Boundary values are preserved") {
                REQUIRE(deserializedPrefs.defaultNoteLength == Catch::Approx(0.0));
            }
        }
    }

    GIVEN("UserPreferences with maximum note length") {
        UserPreferences prefs;
        prefs.defaultNoteLength = 1.0;  // Maximum

        WHEN("Serialized to XML and deserialized") {
            auto xml = prefs.toXml();
            UserPreferences deserializedPrefs;
            deserializedPrefs.fromXml(*xml);

            THEN("Maximum value is preserved") {
                REQUIRE(deserializedPrefs.defaultNoteLength == Catch::Approx(1.0));
            }
        }
    }
}

SCENARIO("UserPreferences XML structure", "[userpreferences][xml]") {
    GIVEN("UserPreferences") {
        UserPreferences prefs;
        prefs.previewNotesOnEdit = false;
        prefs.followStep = true;
        prefs.followPlayback = false;
        prefs.defaultNoteLength = 0.8;
        prefs.midiClockOutputEnabled = true;

        WHEN("Converted to XML") {
            auto xml = prefs.toXml();

            THEN("XML has correct tag name") {
                REQUIRE(xml->getTagName() == "UserPreferences");
            }

            THEN("XML contains all attributes") {
                REQUIRE(xml->hasAttribute("previewNotesOnEdit"));
                REQUIRE(xml->hasAttribute("followStep"));
                REQUIRE(xml->hasAttribute("followPlayback"));
                REQUIRE(xml->hasAttribute("defaultNoteLength"));
                REQUIRE(xml->hasAttribute("midiClockOutputEnabled"));
            }

            THEN("XML attribute values are correct") {
                REQUIRE(xml->getBoolAttribute("previewNotesOnEdit") == false);
                REQUIRE(xml->getBoolAttribute("followStep") == true);
                REQUIRE(xml->getBoolAttribute("followPlayback") == false);
                REQUIRE(xml->getDoubleAttribute("defaultNoteLength") == Catch::Approx(0.8));
                REQUIRE(xml->getBoolAttribute("midiClockOutputEnabled") == true);
            }
        }
    }
}

SCENARIO("UserPreferences fromXml with missing attributes", "[userpreferences][xml]") {
    GIVEN("XML with missing attributes") {
        auto xml = std::make_unique<juce::XmlElement>("UserPreferences");
        // Only set some attributes, leaving others missing
        xml->setAttribute("previewNotesOnEdit", false);
        xml->setAttribute("followStep", false);
        // defaultNoteLength, followPlayback, midiClockOutputEnabled are missing

        WHEN("Deserialized to UserPreferences") {
            UserPreferences prefs;
            prefs.fromXml(*xml);

            THEN("Missing attributes get default values") {
                REQUIRE(prefs.previewNotesOnEdit == false);  // Set in XML
                REQUIRE(prefs.followStep == false);          // Set in XML
                REQUIRE(prefs.followPlayback == true);       // Default from fromXml
                REQUIRE(prefs.defaultNoteLength == Catch::Approx(0.9));  // Default
                REQUIRE(prefs.midiClockOutputEnabled == false);          // Default
            }
        }
    }
}

SCENARIO("UserPreferences fromXml with invalid tag name", "[userpreferences][xml]") {
    GIVEN("XML with wrong tag name") {
        auto xml = std::make_unique<juce::XmlElement>("WrongTagName");
        xml->setAttribute("previewNotesOnEdit", false);

        WHEN("Deserialized to UserPreferences") {
            UserPreferences prefs;
            prefs.previewNotesOnEdit = true;  // Set to non-default
            prefs.fromXml(*xml);

            THEN("Preferences remain unchanged") {
                REQUIRE(prefs.previewNotesOnEdit == true);  // Unchanged
            }
        }
    }
}

SCENARIO("UserPreferences round-trip serialization", "[userpreferences][xml]") {
    GIVEN("UserPreferences with various configurations") {
        struct TestCase {
            bool previewNotesOnEdit;
            bool followStep;
            bool followPlayback;
            double defaultNoteLength;
            bool midiClockOutputEnabled;
        };

        std::vector<TestCase> testCases = {
            {true,  true,  true,  0.9,  false},  // All defaults
            {false, false, false, 0.0,  true},   // All opposites
            {true,  false, true,  0.5,  false},  // Mixed
            {false, true,  false, 0.25, true},   // Mixed
            {true,  true,  false, 1.0,  true},   // Max note length
        };

        for (const auto& testCase : testCases) {
            WHEN("Testing configuration") {
                UserPreferences original;
                original.previewNotesOnEdit = testCase.previewNotesOnEdit;
                original.followStep = testCase.followStep;
                original.followPlayback = testCase.followPlayback;
                original.defaultNoteLength = testCase.defaultNoteLength;
                original.midiClockOutputEnabled = testCase.midiClockOutputEnabled;

                // Serialize and deserialize
                auto xml = original.toXml();
                UserPreferences deserialized;
                deserialized.fromXml(*xml);

                THEN("All properties match after round-trip") {
                    REQUIRE(deserialized.previewNotesOnEdit == original.previewNotesOnEdit);
                    REQUIRE(deserialized.followStep == original.followStep);
                    REQUIRE(deserialized.followPlayback == original.followPlayback);
                    REQUIRE(deserialized.defaultNoteLength == Catch::Approx(original.defaultNoteLength));
                    REQUIRE(deserialized.midiClockOutputEnabled == original.midiClockOutputEnabled);
                }
            }
        }
    }
}

SCENARIO("UserPreferences note length edge cases", "[userpreferences]") {
    GIVEN("UserPreferences") {
        UserPreferences prefs;

        WHEN("Note length is set to 0.0 (instant notes)") {
            prefs.defaultNoteLength = 0.0;

            THEN("Value is accepted") {
                REQUIRE(prefs.defaultNoteLength == Catch::Approx(0.0));
            }

            WHEN("Serialized and deserialized") {
                auto xml = prefs.toXml();
                UserPreferences deserialized;
                deserialized.fromXml(*xml);

                THEN("Value is preserved") {
                    REQUIRE(deserialized.defaultNoteLength == Catch::Approx(0.0));
                }
            }
        }

        WHEN("Note length is set to 1.0 (full step length)") {
            prefs.defaultNoteLength = 1.0;

            THEN("Value is accepted") {
                REQUIRE(prefs.defaultNoteLength == Catch::Approx(1.0));
            }

            WHEN("Serialized and deserialized") {
                auto xml = prefs.toXml();
                UserPreferences deserialized;
                deserialized.fromXml(*xml);

                THEN("Value is preserved") {
                    REQUIRE(deserialized.defaultNoteLength == Catch::Approx(1.0));
                }
            }
        }

        WHEN("Note length is set to precise value") {
            prefs.defaultNoteLength = 0.777777;

            WHEN("Serialized and deserialized") {
                auto xml = prefs.toXml();
                UserPreferences deserialized;
                deserialized.fromXml(*xml);

                THEN("Precision is maintained") {
                    REQUIRE(deserialized.defaultNoteLength == Catch::Approx(0.777777).epsilon(0.000001));
                }
            }
        }
    }
}
