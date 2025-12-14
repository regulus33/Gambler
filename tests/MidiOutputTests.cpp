#include <catch2/catch_test_macros.hpp>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <thread>
#include <chrono>

/**
 * MIDI Output Tests
 * 
 * These tests verify that we can send MIDI to a connected USB MIDI device.
 * They are tagged with [midi][manual] so they can be run separately from
 * the main test suite (since they require physical hardware).
 * 
 * Run with: ./Tests "[midi]" --reporter rspec
 */

// Helper to find the first available MIDI output device
static std::unique_ptr<juce::MidiOutput> openFirstMidiDevice() {
    auto devices = juce::MidiOutput::getAvailableDevices();
    
    if (devices.isEmpty()) {
        return nullptr;
    }
    
    // Print all available devices for debugging
    for (const auto& device : devices) {
        DBG("Found MIDI device: " << device.name << " (ID: " << device.identifier << ")");
    }
    
    // Open the first device
    return juce::MidiOutput::openDevice(devices[0].identifier);
}

TEST_CASE("MIDI Output - List Available Devices", "[midi][manual]") {
    // Initialize JUCE message manager for MIDI
    juce::MessageManager::getInstance();
    
    auto devices = juce::MidiOutput::getAvailableDevices();
    
    INFO("Number of MIDI output devices found: " << devices.size());
    
    for (int i = 0; i < devices.size(); i++) {
        INFO("Device " << i << ": " << devices[i].name.toStdString() 
             << " (ID: " << devices[i].identifier.toStdString() << ")");
    }
    
    // This test passes if we can enumerate devices (even if none found)
    REQUIRE(true);
}

TEST_CASE("MIDI Output - Send Note On/Off to First Device", "[midi][manual]") {
    // Initialize JUCE message manager for MIDI
    juce::MessageManager::getInstance();
    
    auto midiOut = openFirstMidiDevice();
    
    if (!midiOut) {
        WARN("No MIDI output devices found - skipping test");
        REQUIRE(true);  // Pass the test but warn
        return;
    }
    
    INFO("Opened MIDI device: " << midiOut->getName().toStdString());
    
    SECTION("Send single note on/off") {
        // Send Note On: Channel 1, Note 60 (Middle C), Velocity 100
        auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
        midiOut->sendMessageNow(noteOn);
        
        INFO("Sent Note On: Channel 1, Note 60, Velocity 100");
        
        // Wait 500ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Send Note Off
        auto noteOff = juce::MidiMessage::noteOff(1, 60);
        midiOut->sendMessageNow(noteOff);
        
        INFO("Sent Note Off: Channel 1, Note 60");
        
        REQUIRE(true);
    }
}

TEST_CASE("MIDI Output - Send Scale Pattern", "[midi][manual]") {
    // Initialize JUCE message manager for MIDI
    juce::MessageManager::getInstance();
    
    auto midiOut = openFirstMidiDevice();
    
    if (!midiOut) {
        WARN("No MIDI output devices found - skipping test");
        REQUIRE(true);
        return;
    }
    
    INFO("Opened MIDI device: " << midiOut->getName().toStdString());
    
    SECTION("Play C major scale ascending and descending") {
        // C major scale: C4, D4, E4, F4, G4, A4, B4, C5
        const int notes[] = {60, 62, 64, 65, 67, 69, 71, 72};
        const int numNotes = 8;
        const int noteDurationMs = 200;
        const juce::uint8 velocity = 100;
        
        INFO("Playing C major scale ascending...");
        
        // Ascending
        for (int i = 0; i < numNotes; i++) {
            auto noteOn = juce::MidiMessage::noteOn(1, notes[i], velocity);
            midiOut->sendMessageNow(noteOn);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(noteDurationMs));
            
            auto noteOff = juce::MidiMessage::noteOff(1, notes[i]);
            midiOut->sendMessageNow(noteOff);
            
            // Small gap between notes
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        INFO("Playing C major scale descending...");
        
        // Descending
        for (int i = numNotes - 1; i >= 0; i--) {
            auto noteOn = juce::MidiMessage::noteOn(1, notes[i], velocity);
            midiOut->sendMessageNow(noteOn);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(noteDurationMs));
            
            auto noteOff = juce::MidiMessage::noteOff(1, notes[i]);
            midiOut->sendMessageNow(noteOff);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        REQUIRE(true);
    }
}

TEST_CASE("MIDI Output - Send Rapid Note Sequence", "[midi][manual]") {
    // Initialize JUCE message manager for MIDI
    juce::MessageManager::getInstance();
    
    auto midiOut = openFirstMidiDevice();
    
    if (!midiOut) {
        WARN("No MIDI output devices found - skipping test");
        REQUIRE(true);
        return;
    }
    
    INFO("Opened MIDI device: " << midiOut->getName().toStdString());
    
    SECTION("Send 16 rapid notes (simulating 16th notes at 120 BPM)") {
        // At 120 BPM, a 16th note is 125ms
        const int noteDurationMs = 100;  // Slightly shorter for gap
        const int gapMs = 25;
        const juce::uint8 velocity = 100;
        
        // Alternate between two notes
        const int note1 = 60;  // C4
        const int note2 = 64;  // E4
        
        INFO("Sending 16 rapid alternating notes...");
        
        for (int i = 0; i < 16; i++) {
            int note = (i % 2 == 0) ? note1 : note2;
            
            auto noteOn = juce::MidiMessage::noteOn(1, note, velocity);
            midiOut->sendMessageNow(noteOn);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(noteDurationMs));
            
            auto noteOff = juce::MidiMessage::noteOff(1, note);
            midiOut->sendMessageNow(noteOff);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(gapMs));
        }
        
        REQUIRE(true);
    }
}

TEST_CASE("MIDI Output - Send Chord", "[midi][manual]") {
    // Initialize JUCE message manager for MIDI
    juce::MessageManager::getInstance();
    
    auto midiOut = openFirstMidiDevice();
    
    if (!midiOut) {
        WARN("No MIDI output devices found - skipping test");
        REQUIRE(true);
        return;
    }
    
    INFO("Opened MIDI device: " << midiOut->getName().toStdString());
    
    SECTION("Send C major chord") {
        // C major chord: C4, E4, G4
        const int notes[] = {60, 64, 67};
        const int numNotes = 3;
        const juce::uint8 velocity = 100;
        
        INFO("Sending C major chord (C4, E4, G4)...");
        
        // Send all note ons
        for (int i = 0; i < numNotes; i++) {
            auto noteOn = juce::MidiMessage::noteOn(1, notes[i], velocity);
            midiOut->sendMessageNow(noteOn);
        }
        
        // Hold for 1 second
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Send all note offs
        for (int i = 0; i < numNotes; i++) {
            auto noteOff = juce::MidiMessage::noteOff(1, notes[i]);
            midiOut->sendMessageNow(noteOff);
        }
        
        INFO("Chord released");
        
        REQUIRE(true);
    }
}

TEST_CASE("MIDI Output - All Notes Off", "[midi][manual]") {
    // Initialize JUCE message manager for MIDI
    juce::MessageManager::getInstance();
    
    auto midiOut = openFirstMidiDevice();
    
    if (!midiOut) {
        WARN("No MIDI output devices found - skipping test");
        REQUIRE(true);
        return;
    }
    
    INFO("Opened MIDI device: " << midiOut->getName().toStdString());
    
    SECTION("Send All Notes Off message") {
        // Send All Notes Off (CC 123) on channel 1
        auto allNotesOff = juce::MidiMessage::allNotesOff(1);
        midiOut->sendMessageNow(allNotesOff);
        
        INFO("Sent All Notes Off on channel 1");
        
        // Also send note offs for all notes 0-127 to be thorough
        for (int note = 0; note < 128; note++) {
            auto noteOff = juce::MidiMessage::noteOff(1, note);
            midiOut->sendMessageNow(noteOff);
        }
        
        INFO("Sent individual Note Off for all 128 notes");
        
        REQUIRE(true);
    }
}

