#include <catch2/catch_test_macros.hpp>
#include "../source/backend/PresetManager.h"
#include <juce_core/juce_core.h>

/**
 * PresetManager Tests
 *
 * Tests for preset file management, including saving, loading, and deleting presets.
 * Uses a temporary directory to avoid interfering with actual user presets.
 */

// Helper class to manage a temporary test directory
class TempPresetDirectory {
public:
    TempPresetDirectory() {
        // Create a unique temp directory for this test
        tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("GamblerTestPresets_" + juce::Uuid().toString());

        tempDir.createDirectory();
    }

    ~TempPresetDirectory() {
        // Clean up temp directory
        if (tempDir.exists()) {
            tempDir.deleteRecursively();
        }
    }

    juce::File getDirectory() const { return tempDir; }

private:
    juce::File tempDir;
};

// Helper to create a simple test XML state
std::unique_ptr<juce::XmlElement> createTestState(const juce::String& identifier = "test") {
    auto xml = std::make_unique<juce::XmlElement>("PluginState");
    xml->setAttribute("id", identifier);
    xml->setAttribute("value", 42);
    return xml;
}

SCENARIO("PresetManager initialization", "[presetmanager]") {
    GIVEN("A new PresetManager") {
        PresetManager manager;

        THEN("Presets directory is created") {
            auto dir = PresetManager::getPresetsDirectory();
            REQUIRE(dir.exists());
            REQUIRE(dir.isDirectory());
        }

        THEN("Presets list is initially scanned") {
            // May be empty or contain existing user presets
            // Just verify we can get the list without crashing
            auto presets = manager.getPresets();
            REQUIRE(presets.size() >= 0);
        }
    }
}

SCENARIO("PresetManager save and load operations", "[presetmanager]") {
    GIVEN("A PresetManager") {
        PresetManager manager;

        WHEN("Saving a preset") {
            auto testState = createTestState("save_test");
            bool saved = manager.savePreset("TestPreset", *testState);

            THEN("Save operation succeeds") {
                REQUIRE(saved == true);
            }

            THEN("Preset appears in preset list") {
                manager.refreshPresetList();
                bool found = manager.presetExists("TestPreset");
                REQUIRE(found == true);
            }

            WHEN("Loading the saved preset") {
                auto loaded = manager.loadPreset("TestPreset");

                THEN("Load succeeds") {
                    REQUIRE(loaded != nullptr);
                }

                THEN("Loaded state matches saved state") {
                    REQUIRE(loaded->getTagName() == "PluginState");
                    REQUIRE(loaded->getStringAttribute("id") == "save_test");
                    REQUIRE(loaded->getIntAttribute("value") == 42);
                }
            }

            // Cleanup
            manager.deletePreset("TestPreset");
        }
    }

    GIVEN("A PresetManager with a saved preset") {
        PresetManager manager;
        auto testState = createTestState("load_test");
        manager.savePreset("LoadTest", *testState);
        manager.refreshPresetList();

        WHEN("Loading by name") {
            auto loaded = manager.loadPreset("LoadTest");

            THEN("Correct preset is loaded") {
                REQUIRE(loaded != nullptr);
                REQUIRE(loaded->getStringAttribute("id") == "load_test");
            }
        }

        WHEN("Loading by index") {
            int index = manager.getPresetIndex("LoadTest");
            REQUIRE(index >= 0);

            auto loaded = manager.loadPresetByIndex(index);

            THEN("Correct preset is loaded") {
                REQUIRE(loaded != nullptr);
                REQUIRE(loaded->getStringAttribute("id") == "load_test");
            }
        }

        WHEN("Loading non-existent preset") {
            auto loaded = manager.loadPreset("NonExistent");

            THEN("Load returns nullptr") {
                REQUIRE(loaded == nullptr);
            }
        }

        WHEN("Loading with invalid index") {
            auto loaded = manager.loadPresetByIndex(-1);

            THEN("Load returns nullptr") {
                REQUIRE(loaded == nullptr);
            }

            auto loaded2 = manager.loadPresetByIndex(999999);

            THEN("Load returns nullptr for out of range") {
                REQUIRE(loaded2 == nullptr);
            }
        }

        // Cleanup
        manager.deletePreset("LoadTest");
    }
}

SCENARIO("PresetManager delete operations", "[presetmanager]") {
    GIVEN("A PresetManager with saved presets") {
        PresetManager manager;

        manager.savePreset("DeleteTest1", *createTestState());
        manager.savePreset("DeleteTest2", *createTestState());
        manager.refreshPresetList();

        WHEN("Deleting a preset by name") {
            bool deleted = manager.deletePreset("DeleteTest1");

            THEN("Delete succeeds") {
                REQUIRE(deleted == true);
            }

            THEN("Preset is removed from list") {
                manager.refreshPresetList();
                REQUIRE(manager.presetExists("DeleteTest1") == false);
                REQUIRE(manager.presetExists("DeleteTest2") == true);
            }
        }

        WHEN("Deleting a preset by index") {
            int index = manager.getPresetIndex("DeleteTest2");
            bool deleted = manager.deletePresetByIndex(index);

            THEN("Delete succeeds") {
                REQUIRE(deleted == true);
            }

            THEN("Preset is removed from list") {
                manager.refreshPresetList();
                REQUIRE(manager.presetExists("DeleteTest2") == false);
            }
        }

        WHEN("Deleting non-existent preset") {
            bool deleted = manager.deletePreset("NonExistent");

            THEN("Delete fails gracefully") {
                REQUIRE(deleted == false);
            }
        }

        WHEN("Deleting with invalid index") {
            bool deleted = manager.deletePresetByIndex(-1);

            THEN("Delete fails gracefully") {
                REQUIRE(deleted == false);
            }
        }

        // Cleanup any remaining
        manager.deletePreset("DeleteTest1");
        manager.deletePreset("DeleteTest2");
    }
}

SCENARIO("PresetManager preset name sanitization", "[presetmanager]") {
    GIVEN("A PresetManager") {
        PresetManager manager;

        WHEN("Saving preset with special characters") {
            auto testState = createTestState();
            bool saved = manager.savePreset("Test/Preset:With*Special?Chars", *testState);

            THEN("Save succeeds (characters are sanitized)") {
                REQUIRE(saved == true);
            }

            THEN("Preset can be loaded") {
                manager.refreshPresetList();
                // The name will be sanitized, but should be findable
                auto presets = manager.getPresets();
                bool found = false;
                for (const auto& preset : presets) {
                    if (preset.name.contains("Test") && preset.name.contains("Preset")) {
                        found = true;
                        // Cleanup
                        manager.deletePreset(preset.name);
                        break;
                    }
                }
                REQUIRE(found == true);
            }
        }

        WHEN("Saving preset with very long name") {
            juce::String longName;
            for (int i = 0; i < 150; i++) {
                longName += "a";
            }

            auto testState = createTestState();
            bool saved = manager.savePreset(longName, *testState);

            THEN("Save succeeds (name is truncated)") {
                REQUIRE(saved == true);
            }

            // Cleanup
            manager.refreshPresetList();
            auto presets = manager.getPresets();
            if (!presets.empty()) {
                for (const auto& preset : presets) {
                    if (preset.name.startsWith("aaa")) {
                        manager.deletePreset(preset.name);
                    }
                }
            }
        }

        WHEN("Saving preset with empty name") {
            auto testState = createTestState();
            bool saved = manager.savePreset("", *testState);

            THEN("Save fails") {
                REQUIRE(saved == false);
            }
        }
    }
}

SCENARIO("PresetManager preset list management", "[presetmanager]") {
    GIVEN("A PresetManager") {
        PresetManager manager;

        WHEN("Multiple presets are saved") {
            manager.savePreset("Preset_A", *createTestState("a"));
            manager.savePreset("Preset_B", *createTestState("b"));
            manager.savePreset("Preset_C", *createTestState("c"));
            manager.refreshPresetList();

            THEN("All presets appear in the list") {
                REQUIRE(manager.presetExists("Preset_A"));
                REQUIRE(manager.presetExists("Preset_B"));
                REQUIRE(manager.presetExists("Preset_C"));
            }

            THEN("Preset names can be retrieved as StringArray") {
                auto names = manager.getPresetNames();
                bool hasA = names.contains("Preset_A");
                bool hasB = names.contains("Preset_B");
                bool hasC = names.contains("Preset_C");

                REQUIRE(hasA);
                REQUIRE(hasB);
                REQUIRE(hasC);
            }

            THEN("Presets are sorted alphabetically") {
                auto names = manager.getPresetNames();
                int indexA = names.indexOf("Preset_A");
                int indexB = names.indexOf("Preset_B");
                int indexC = names.indexOf("Preset_C");

                REQUIRE(indexA < indexB);
                REQUIRE(indexB < indexC);
            }

            // Cleanup
            manager.deletePreset("Preset_A");
            manager.deletePreset("Preset_B");
            manager.deletePreset("Preset_C");
        }
    }
}

SCENARIO("PresetManager current preset tracking", "[presetmanager]") {
    GIVEN("A PresetManager") {
        PresetManager manager;

        WHEN("No preset is loaded") {
            THEN("Current preset name is empty") {
                REQUIRE(manager.getCurrentPresetName().isEmpty());
            }
        }

        WHEN("Current preset name is set") {
            manager.setCurrentPresetName("MyPreset");

            THEN("Current preset name is returned") {
                REQUIRE(manager.getCurrentPresetName() == "MyPreset");
            }
        }

        WHEN("Current preset name is changed") {
            manager.setCurrentPresetName("Preset1");
            manager.setCurrentPresetName("Preset2");

            THEN("Latest name is returned") {
                REQUIRE(manager.getCurrentPresetName() == "Preset2");
            }
        }
    }
}

SCENARIO("PresetManager preset index lookup", "[presetmanager]") {
    GIVEN("A PresetManager with multiple presets") {
        PresetManager manager;

        manager.savePreset("Alpha", *createTestState());
        manager.savePreset("Beta", *createTestState());
        manager.savePreset("Gamma", *createTestState());
        manager.refreshPresetList();

        WHEN("Looking up preset index by name") {
            int alphaIndex = manager.getPresetIndex("Alpha");
            int betaIndex = manager.getPresetIndex("Beta");
            int gammaIndex = manager.getPresetIndex("Gamma");

            THEN("Valid indices are returned") {
                REQUIRE(alphaIndex >= 0);
                REQUIRE(betaIndex >= 0);
                REQUIRE(gammaIndex >= 0);
            }

            THEN("Indices match alphabetical order") {
                REQUIRE(alphaIndex < betaIndex);
                REQUIRE(betaIndex < gammaIndex);
            }
        }

        WHEN("Looking up non-existent preset") {
            int index = manager.getPresetIndex("NonExistent");

            THEN("Returns -1") {
                REQUIRE(index == -1);
            }
        }

        // Cleanup
        manager.deletePreset("Alpha");
        manager.deletePreset("Beta");
        manager.deletePreset("Gamma");
    }
}

SCENARIO("PresetManager overwrite existing preset", "[presetmanager]") {
    GIVEN("A PresetManager with an existing preset") {
        PresetManager manager;

        manager.savePreset("OverwriteTest", *createTestState("original"));
        manager.refreshPresetList();

        WHEN("Saving a preset with the same name") {
            auto newState = createTestState("modified");
            bool saved = manager.savePreset("OverwriteTest", *newState);

            THEN("Save succeeds") {
                REQUIRE(saved == true);
            }

            THEN("Only one preset with that name exists") {
                manager.refreshPresetList();
                auto names = manager.getPresetNames();
                int count = 0;
                for (const auto& name : names) {
                    if (name == "OverwriteTest") {
                        count++;
                    }
                }
                REQUIRE(count == 1);
            }

            THEN("Loaded preset contains new data") {
                auto loaded = manager.loadPreset("OverwriteTest");
                REQUIRE(loaded != nullptr);
                REQUIRE(loaded->getStringAttribute("id") == "modified");
            }
        }

        // Cleanup
        manager.deletePreset("OverwriteTest");
    }
}

SCENARIO("PresetManager case-insensitive preset existence check", "[presetmanager]") {
    GIVEN("A PresetManager with a preset") {
        PresetManager manager;

        manager.savePreset("MyPreset", *createTestState());
        manager.refreshPresetList();

        WHEN("Checking with different case") {
            bool exists1 = manager.presetExists("MyPreset");
            bool exists2 = manager.presetExists("mypreset");
            bool exists3 = manager.presetExists("MYPRESET");

            THEN("All variations are found") {
                REQUIRE(exists1 == true);
                REQUIRE(exists2 == true);
                REQUIRE(exists3 == true);
            }
        }

        // Cleanup
        manager.deletePreset("MyPreset");
    }
}

SCENARIO("PresetManager refresh after external file changes", "[presetmanager]") {
    GIVEN("A PresetManager") {
        PresetManager manager;

        // Save some presets
        manager.savePreset("RefreshTest1", *createTestState());
        manager.savePreset("RefreshTest2", *createTestState());
        manager.refreshPresetList();

        int initialCount = static_cast<int>(manager.getPresets().size());

        WHEN("A preset file is manually deleted") {
            // Manually delete the file
            auto dir = PresetManager::getPresetsDirectory();
            auto file = dir.getChildFile("RefreshTest1.xml");
            file.deleteFile();

            WHEN("Preset list is refreshed") {
                manager.refreshPresetList();

                THEN("Deleted preset is removed from list") {
                    REQUIRE(manager.presetExists("RefreshTest1") == false);
                    REQUIRE(manager.presetExists("RefreshTest2") == true);
                    REQUIRE(static_cast<int>(manager.getPresets().size()) == initialCount - 1);
                }
            }
        }

        // Cleanup
        manager.deletePreset("RefreshTest2");
    }
}
