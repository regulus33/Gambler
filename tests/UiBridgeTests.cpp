/**
 * UiBridgeTests.cpp - the contract between the sequencer and the web UI.
 *
 * These drive the engine exactly the way the page does: actions in, snapshot out.
 * No editor or web view is involved.
 */

#include "helpers/test_helpers.h"
#include <bridge/UiBridge.h>
#include <catch2/catch_test_macros.hpp>
#include <set>

namespace {
    juce::var payload (std::initializer_list<std::pair<const char*, juce::var>> fields) {
        auto* object = new juce::DynamicObject();
        for (const auto& [key, value] : fields) object->setProperty (key, value);
        return juce::var (object);
    }

    struct BridgeFixture {
        GamblerTestInitialiser juceInit;
        PluginProcessor processor;
        UiBridge& bridge { processor.getUiBridge() };
        std::vector<juce::String> events;

        BridgeFixture() {
            processor.stopSequencer();
            bridge.onEvent = [this] (const juce::Identifier& event, const juce::var&) { events.push_back (event.toString()); };
        }

        void send (const juce::String& action, const juce::var& data = {}) { bridge.handleAction (action, data); }
        juce::var snapshot() const { return bridge.buildSnapshot(); }
        SequenceState& state() { return processor.getState(); }

        int shownNote (int step, int sub = 0) const { return snapshot()["steps"][step]["notes"][sub]; }

        void setScale (std::set<int> pitchClasses) {
            for (int pc = 0; pc < 12; ++pc)
                send ("setScaleNote", payload ({ { "pc", pc }, { "on", pitchClasses.count (pc) > 0 } }));
        }

        void midi (const juce::MidiMessage& message) {
            juce::AudioBuffer<float> buffer (2, 64);
            juce::MidiBuffer midiBuffer;
            midiBuffer.addEvent (message, 0);
            processor.processBlock (buffer, midiBuffer);
        }
    };
}

TEST_CASE ("Snapshot describes a fresh sequence", "[bridge]") {
    BridgeFixture f;
    const auto s = f.snapshot();

    REQUIRE (static_cast<int> (s["length"]) == ParamConstants::DEFAULT_LENGTH);
    REQUIRE (s["steps"].size() == ParamConstants::MAX_STEPS);
    REQUIRE (s["steps"][0]["notes"].size() == ParamConstants::MAX_SUBSTEPS);
    REQUIRE (static_cast<int> (s["steps"][0]["notes"][0]) == ParamConstants::DEFAULT_NOTE);
    REQUIRE (s["scale"].size() == 12);
    REQUIRE (s["ranges"].size() == 1);
    REQUIRE (s["prob"].size() == 64);
    REQUIRE (s["colours"].size() == 12);
    REQUIRE (s["colours"][0].toString().startsWith ("#"));
    REQUIRE (s["colours"][0].toString().length() == 7);
    REQUIRE (s["transport"].toString() == "stopped");
    REQUIRE_FALSE (static_cast<bool> (s["canUndo"]));
    REQUIRE_FALSE (static_cast<bool> (s["learn"]["active"]));
}

TEST_CASE ("State is pushed after an action, coalesced into one event", "[bridge]") {
    BridgeFixture f;
    f.bridge.flush();
    f.events.clear();

    f.send ("setLength", payload ({ { "length", 12 } }));
    f.send ("setLength", payload ({ { "length", 16 } }));
    f.bridge.flush();

    REQUIRE (f.events == std::vector<juce::String> { "state" });
    REQUIRE (static_cast<int> (f.snapshot()["length"]) == 16);
}

TEST_CASE ("shiftNote moves by scale degrees and stops at the range edge", "[bridge][notes]") {
    BridgeFixture f;
    f.setScale ({ 0, 3, 5, 7, 10 });  // C minor pentatonic
    f.send ("setRange", payload ({ { "index", 0 }, { "min", 48 }, { "max", 72 } }));

    f.send ("shiftNote", payload ({ { "step", 0 }, { "sub", 0 }, { "delta", 1 } }));
    REQUIRE (f.shownNote (0) == 63);  // C4 -> Eb4

    f.send ("shiftNote", payload ({ { "step", 0 }, { "sub", 0 }, { "delta", 2 } }));
    REQUIRE (f.shownNote (0) == 67);  // -> F4 -> G4

    f.send ("shiftNote", payload ({ { "step", 0 }, { "sub", 0 }, { "delta", -3 } }));
    REQUIRE (f.shownNote (0) == 60);

    f.send ("shiftNote", payload ({ { "step", 0 }, { "sub", 0 }, { "delta", 40 } }));
    REQUIRE (f.shownNote (0) == 72);  // top of the range
}

TEST_CASE ("shiftNote gives a rest a note", "[bridge][notes]") {
    BridgeFixture f;
    f.send ("selectStep", payload ({ { "step", 1 }, { "audition", false } }));
    f.send ("clear", payload ({ { "scope", "step" } }));
    REQUIRE (f.shownNote (1) == -1);

    f.send ("shiftNote", payload ({ { "step", 1 }, { "sub", 0 }, { "delta", 1 } }));
    REQUIRE (f.shownNote (1) >= 0);
}

TEST_CASE ("The first range round-trips through the window parameters", "[bridge][ranges]") {
    BridgeFixture f;
    for (int lo = 0; lo <= 127; lo += 7) {
        for (int hi = lo; hi <= 127; hi += 5) {
            f.state().setNoteRange (0, lo, hi);
            REQUIRE (f.state().getNoteRange (0).min == lo);
            REQUIRE (f.state().getNoteRange (0).max == hi);
        }
    }
}

TEST_CASE ("A range can be added while the first one covers everything", "[bridge][ranges]") {
    BridgeFixture f;
    f.send ("addRange");
    const auto ranges = f.snapshot()["ranges"];
    REQUIRE (ranges.size() == 2);
    REQUIRE (static_cast<int> (ranges[0]["min"]) == 0);
    REQUIRE (static_cast<int> (ranges[0]["max"]) == 127);
    REQUIRE (static_cast<int> (ranges[1]["min"]) == 60);
    REQUIRE (static_cast<int> (ranges[1]["max"]) == 71);
}

TEST_CASE ("Several note ranges act as one combined filter", "[bridge][ranges]") {
    BridgeFixture f;
    f.send ("setRange", payload ({ { "index", 0 }, { "min", 48 }, { "max", 52 } }));
    f.send ("addRange");
    f.send ("setRange", payload ({ { "index", 1 }, { "min", 72 }, { "max", 76 } }));

    SECTION ("the snapshot lists them") {
        const auto ranges = f.snapshot()["ranges"];
        REQUIRE (ranges.size() == 2);
        REQUIRE (static_cast<int> (ranges[1]["min"]) == 72);
        REQUIRE (static_cast<int> (ranges[1]["max"]) == 76);
    }

    SECTION ("a note between ranges snaps to the nearer one") {
        f.state().setStepNote (0, 0, 60);  // 8 above the low range, 12 below the high one
        REQUIRE (f.shownNote (0) == 52);
        f.state().setStepNote (0, 0, 66);
        REQUIRE (f.shownNote (0) == 72);
    }

    SECTION ("shiftNote crosses the gap") {
        f.state().setStepNote (0, 0, 52);
        f.send ("shiftNote", payload ({ { "step", 0 }, { "sub", 0 }, { "delta", 1 } }));
        REQUIRE (f.shownNote (0) == 72);
        f.send ("shiftNote", payload ({ { "step", 0 }, { "sub", 0 }, { "delta", -1 } }));
        REQUIRE (f.shownNote (0) == 52);
    }

    SECTION ("random notes land in both ranges and nowhere else") {
        std::set<int> seen;
        for (int i = 0; i < 400; ++i) seen.insert (f.state().generateRandomNoteUnquantized());
        for (int note : seen)
            REQUIRE (((note >= 48 && note <= 52) || (note >= 72 && note <= 76)));
        REQUIRE (*seen.begin() <= 52);
        REQUIRE (*seen.rbegin() >= 72);
    }

    SECTION ("there can be at most three, and never fewer than one") {
        f.send ("addRange");
        f.send ("addRange");
        REQUIRE (f.snapshot()["ranges"].size() == ParamConstants::MAX_NOTE_RANGES);

        f.send ("removeRange", payload ({ { "index", 0 } }));
        REQUIRE (f.snapshot()["ranges"].size() == 2);
        REQUIRE (static_cast<int> (f.snapshot()["ranges"][0]["min"]) == 72);  // later ranges move down

        f.send ("removeRange", payload ({ { "index", 0 } }));
        f.send ("removeRange", payload ({ { "index", 0 } }));
        REQUIRE (f.snapshot()["ranges"].size() == 1);
    }
}

TEST_CASE ("Cycle notes can be added and removed", "[bridge][cycle]") {
    BridgeFixture f;
    f.state().setStepNote (0, 0, 62);
    f.send ("addSub", payload ({ { "step", 0 } }));
    f.send ("addSub", payload ({ { "step", 0 } }));

    auto step = f.snapshot()["steps"][0];
    REQUIRE (static_cast<int> (step["count"]) == 3);
    REQUIRE (static_cast<int> (step["sel"]) == 2);
    REQUIRE (static_cast<int> (step["notes"][2]) == 62);  // new notes start as a copy

    f.state().setStepNote (0, 1, 65);
    f.send ("removeSub", payload ({ { "step", 0 }, { "sub", 0 } }));
    step = f.snapshot()["steps"][0];
    REQUIRE (static_cast<int> (step["count"]) == 2);
    REQUIRE (static_cast<int> (step["notes"][0]) == 65);

    f.send ("removeSub", payload ({ { "step", 0 }, { "sub", 0 } }));
    f.send ("removeSub", payload ({ { "step", 0 }, { "sub", 0 } }));
    REQUIRE (static_cast<int> (f.snapshot()["steps"][0]["count"]) == 1);
}

TEST_CASE ("Destructive actions can be undone", "[bridge][undo]") {
    BridgeFixture f;
    for (int i = 0; i < 8; ++i) f.state().setStepNote (i, 0, 60 + i);

    SECTION ("clear all") {
        f.send ("clear", payload ({ { "scope", "all" } }));
        REQUIRE (f.shownNote (3) == -1);
        REQUIRE (static_cast<bool> (f.snapshot()["canUndo"]));
        f.send ("undo");
        for (int i = 0; i < 8; ++i) REQUIRE (f.shownNote (i) == 60 + i);
        REQUIRE_FALSE (static_cast<bool> (f.snapshot()["canUndo"]));
    }

    SECTION ("duplicate doubles the length and undo restores it") {
        f.send ("duplicate");
        REQUIRE (static_cast<int> (f.snapshot()["length"]) == 16);
        REQUIRE (f.shownNote (11) == 63);
        f.send ("undo");
        REQUIRE (static_cast<int> (f.snapshot()["length"]) == 8);
    }

    SECTION ("nudge rotates and undo puts it back") {
        f.send ("nudge", payload ({ { "dir", 1 } }));
        REQUIRE (f.shownNote (0) == 67);
        REQUIRE (f.shownNote (1) == 60);
        f.send ("undo");
        REQUIRE (f.shownNote (0) == 60);
    }

    SECTION ("randomize and shuffle keep the same number of steps") {
        f.send ("randomize", payload ({ { "scope", "all" } }));
        f.send ("shuffle");
        REQUIRE (static_cast<int> (f.snapshot()["length"]) == 8);
        f.send ("undo");
        f.send ("undo");
        for (int i = 0; i < 8; ++i) REQUIRE (f.shownNote (i) == 60 + i);
    }
}

TEST_CASE ("Transport and settings actions reach the engine", "[bridge]") {
    BridgeFixture f;

    f.send ("playPause");
    REQUIRE (f.snapshot()["transport"].toString() == "playing");
    f.send ("playPause");
    REQUIRE (f.snapshot()["transport"].toString() == "paused");
    f.send ("stop");
    REQUIRE (f.snapshot()["transport"].toString() == "stopped");

    f.send ("setParam", payload ({ { "id", "bpm" }, { "value", 97 } }));
    f.send ("setParam", payload ({ { "id", "subdivision" }, { "value", 1 } }));
    f.send ("setParam", payload ({ { "id", "latch" }, { "value", true } }));
    f.send ("setParam", payload ({ { "id", "followStep" }, { "value", true } }));
    const auto s = f.snapshot();
    REQUIRE (static_cast<int> (s["bpm"]) == 97);
    REQUIRE (static_cast<int> (s["subdivision"]) == 1);
    REQUIRE (static_cast<bool> (s["latch"]));
    REQUIRE (static_cast<bool> (s["followStep"]));

    f.send ("roll", payload ({ { "on", true } }));
    REQUIRE (static_cast<bool> (f.snapshot()["roll"]));
    f.send ("roll", payload ({ { "on", false } }));
    REQUIRE_FALSE (static_cast<bool> (f.snapshot()["roll"]));

    f.send ("setColour", payload ({ { "pc", 4 }, { "colour", "#123abc" } }));
    REQUIRE (f.snapshot()["colours"][4].toString() == "#123abc");
}

TEST_CASE ("Chance grid: percentage, painting and division", "[bridge][chance]") {
    BridgeFixture f;
    f.send ("setProbPercent", payload ({ { "percent", 50 } }));
    REQUIRE (static_cast<int> (f.snapshot()["probPercent"]) == 50);

    f.send ("setProbPercent", payload ({ { "percent", 0 } }));
    f.send ("probPaint", payload ({ { "cell", 9 }, { "on", true } }));
    REQUIRE (static_cast<bool> (f.snapshot()["prob"][9]));
    REQUIRE (static_cast<int> (f.snapshot()["probPercent"]) == 2);  // 1 of 64

    f.send ("probDivision");
    REQUIRE (static_cast<int> (f.snapshot()["probDim"]) == 4);
}

TEST_CASE ("MIDI learn maps a controller to a flat action button", "[bridge][learn]") {
    BridgeFixture f;
    f.send ("learnToggle");
    f.send ("learnSelect", payload ({ { "id", "duplicate" } }));
    REQUIRE (f.snapshot()["learn"]["selected"].toString() == "duplicate");

    f.midi (juce::MidiMessage::controllerEvent (1, 21, 127));
    auto learn = f.snapshot()["learn"];
    REQUIRE (learn["selected"].isVoid());
    REQUIRE (learn["mappings"]["duplicate"].toString() == "CH1-CC21");

    f.send ("learnToggle");
    REQUIRE (static_cast<int> (f.snapshot()["length"]) == 8);
    f.midi (juce::MidiMessage::controllerEvent (1, 21, 127));
    REQUIRE (static_cast<int> (f.snapshot()["length"]) == 16);

    f.send ("learnClear", payload ({ { "id", "duplicate" } }));
    REQUIRE (f.snapshot()["learn"]["mappings"]["duplicate"].isVoid());
}

TEST_CASE ("Held MIDI notes define the scale when latch is off", "[bridge][midi-scale]") {
    BridgeFixture f;
    f.send ("setParam", payload ({ { "id", "latch" }, { "value", false } }));
    f.setScale ({});

    f.midi (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100));  // C3
    f.midi (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100));  // C4
    f.midi (juce::MidiMessage::noteOn (1, 64, (juce::uint8) 100));  // E4
    REQUIRE (static_cast<bool> (f.snapshot()["scale"][0]));
    REQUIRE (static_cast<bool> (f.snapshot()["scale"][4]));

    SECTION ("a key stays on until every note of that pitch class is released") {
        f.midi (juce::MidiMessage::noteOff (1, 48));
        REQUIRE (static_cast<bool> (f.snapshot()["scale"][0]));
        f.midi (juce::MidiMessage::noteOff (1, 60));
        REQUIRE_FALSE (static_cast<bool> (f.snapshot()["scale"][0]));
        REQUIRE (static_cast<bool> (f.snapshot()["scale"][4]));
    }

    SECTION ("a note mapped to a control no longer plays the scale") {
        f.midi (juce::MidiMessage::noteOff (1, 48));
        f.midi (juce::MidiMessage::noteOff (1, 60));
        f.midi (juce::MidiMessage::noteOff (1, 64));

        f.send ("learnToggle");
        f.send ("learnSelect", payload ({ { "id", "roll" } }));
        f.midi (juce::MidiMessage::noteOn (1, 62, (juce::uint8) 100));  // D4 becomes Roll
        f.send ("learnToggle");
        f.midi (juce::MidiMessage::noteOff (1, 62));

        f.midi (juce::MidiMessage::noteOn (1, 62, (juce::uint8) 100));
        REQUIRE (static_cast<bool> (f.snapshot()["roll"]));
        REQUIRE_FALSE (static_cast<bool> (f.snapshot()["scale"][2]));
        f.midi (juce::MidiMessage::noteOn (1, 74, (juce::uint8) 100));  // D5 is not mapped
        REQUIRE (static_cast<bool> (f.snapshot()["scale"][2]));
    }
}

TEST_CASE ("A tick is reported to the UI", "[bridge]") {
    BridgeFixture f;
    f.send ("setParam", payload ({ { "id", "clockMode" }, { "value", 0 } }));
    f.send ("playPause");
    f.bridge.flush();
    f.events.clear();

    juce::AudioBuffer<float> buffer (2, 512);
    f.processor.prepareToPlay (44100.0, 512);
    for (int i = 0; i < 40; ++i) {
        juce::MidiBuffer midiBuffer;
        f.processor.processBlock (buffer, midiBuffer);
    }
    f.bridge.flush();

    REQUIRE (std::count (f.events.begin(), f.events.end(), juce::String ("tick")) == 1);
    REQUIRE (static_cast<int> (f.snapshot()["currentStep"]) >= 0);
}
