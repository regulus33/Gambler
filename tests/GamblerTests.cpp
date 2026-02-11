#include "backend/Clock.h"
#include "backend/ClockMode.h"
#include "backend/DawSync.h"
#include "backend/InternalClock.h"
#include "backend/Scale.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <vector>

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
// Scale - Additional Coverage
// ============================================================================

TEST_CASE("Scale additional coverage") {
    SECTION("isNoteInRange()") {
        SECTION("Given range 60-72, note 65 is in range") {
            Scale scale;
            scale.setRange(60, 72);
            REQUIRE(scale.isNoteInRange(65) == true);
        }

        SECTION("Given range 60-72, note 59 is not in range") {
            Scale scale;
            scale.setRange(60, 72);
            REQUIRE(scale.isNoteInRange(59) == false);
        }

        SECTION("Given range 60-72, note 73 is not in range") {
            Scale scale;
            scale.setRange(60, 72);
            REQUIRE(scale.isNoteInRange(73) == false);
        }

        SECTION("Given range 60-72, boundary values are in range") {
            Scale scale;
            scale.setRange(60, 72);
            REQUIRE(scale.isNoteInRange(60) == true);
            REQUIRE(scale.isNoteInRange(72) == true);
        }
    }

    SECTION("isNoteInScale() boundary cases") {
        SECTION("Given chromatic scale, note -1 is not in scale") {
            Scale scale;
            REQUIRE(scale.isNoteInScale(-1) == false);
        }

        SECTION("Given chromatic scale, note 128 is not in scale") {
            Scale scale;
            REQUIRE(scale.isNoteInScale(128) == false);
        }

        SECTION("Given chromatic scale, note 0 is in scale") {
            Scale scale;
            REQUIRE(scale.isNoteInScale(0) == true);
        }

        SECTION("Given chromatic scale, note 127 is in scale") {
            Scale scale;
            REQUIRE(scale.isNoteInScale(127) == true);
        }
    }

    SECTION("setScaleNote()") {
        SECTION("Given chromatic scale, disabling one note") {
            Scale scale;
            scale.setScaleNote(3, 0);  // Disable Eb
            REQUIRE(scale.getScaleNotes()[3] == 0);
            REQUIRE(scale.getScaleNotes()[2] == 1);  // D still enabled
        }

        SECTION("Given empty scale, enabling one note") {
            Scale scale;
            scale.setScaleFromNames({});  // All disabled
            scale.setScaleNote(0, 1);  // Enable C
            REQUIRE(scale.getScaleNotes()[0] == 1);
        }

        SECTION("Given out-of-range index, nothing changes") {
            Scale scale;
            scale.setScaleNote(-1, 0);
            scale.setScaleNote(12, 0);
            // Should not crash; all notes still enabled
            for (int i = 0; i < 12; i++) {
                REQUIRE(scale.getScaleNotes()[i] == 1);
            }
        }
    }

    SECTION("setRangeMin() and setRangeMax() individually") {
        SECTION("Given default scale, setRangeMin to 24") {
            Scale scale;
            scale.setRangeMin(24);
            REQUIRE(scale.getRangeMin() == 24);
            REQUIRE(scale.getRangeMax() == 127);
        }

        SECTION("Given default scale, setRangeMax to 96") {
            Scale scale;
            scale.setRangeMax(96);
            REQUIRE(scale.getRangeMin() == 0);
            REQUIRE(scale.getRangeMax() == 96);
        }

        SECTION("setRangeMin above current max is rejected") {
            Scale scale;
            scale.setRange(60, 72);
            scale.setRangeMin(80);
            REQUIRE(scale.getRangeMin() == 60);  // Unchanged
        }

        SECTION("setRangeMax below current min is rejected") {
            Scale scale;
            scale.setRange(60, 72);
            scale.setRangeMax(50);
            REQUIRE(scale.getRangeMax() == 72);  // Unchanged
        }

        SECTION("setRangeMin to negative is rejected") {
            Scale scale;
            scale.setRangeMin(-1);
            REQUIRE(scale.getRangeMin() == 0);
        }

        SECTION("setRangeMax above 127 is rejected") {
            Scale scale;
            scale.setRangeMax(200);
            REQUIRE(scale.getRangeMax() == 127);
        }
    }

    SECTION("quantize() with no enabled notes returns -1") {
        Scale scale;
        scale.setScaleFromNames({});  // All disabled
        int result = scale.quantize(60);
        REQUIRE(result == -1);
    }

    SECTION("quantize() at MIDI boundaries") {
        Scale scale;
        scale.setScaleFromNames({"C"});  // Only C enabled

        SECTION("Quantize note 0 (C) stays at 0") {
            REQUIRE(scale.quantize(0) == 0);
        }

        SECTION("Quantize note 1 (Db) goes to C0 = 0") {
            REQUIRE(scale.quantize(1) == 0);
        }

        SECTION("Quantize note 127 (G9) goes to nearest C") {
            int result = scale.quantize(127);
            REQUIRE(result % 12 == 0);  // Must be a C
            REQUIRE(result <= 127);
        }
    }

    SECTION("Static window utility functions") {
        SECTION("computeMinFromWindow returns offset clamped to valid range") {
            REQUIRE(Scale::computeMinFromWindow(60, 24) == 60);
            REQUIRE(Scale::computeMinFromWindow(-5, 24) == 0);
            REQUIRE(Scale::computeMinFromWindow(200, 24) == 127);
        }

        SECTION("computeMaxFromWindow returns offset+size-1 clamped to valid range") {
            REQUIRE(Scale::computeMaxFromWindow(60, 24) == 83);  // 60 + 24 - 1
            REQUIRE(Scale::computeMaxFromWindow(120, 20) == 127);  // Clamped
            REQUIRE(Scale::computeMaxFromWindow(0, 1) == 0);  // 0 + 1 - 1
        }

        SECTION("computeOffsetFromRange returns min clamped to valid range") {
            REQUIRE(Scale::computeOffsetFromRange(60, 72) == 60);
            REQUIRE(Scale::computeOffsetFromRange(-5, 72) == 0);
        }

        SECTION("computeSizeFromRange returns max-min+1 clamped") {
            REQUIRE(Scale::computeSizeFromRange(60, 72) == 13);  // 72 - 60 + 1
            REQUIRE(Scale::computeSizeFromRange(0, 127) == 128);
            REQUIRE(Scale::computeSizeFromRange(60, 60) == 1);  // Single note range
        }
    }
}

// ============================================================================
// InternalClock - Additional Coverage
// ============================================================================

TEST_CASE("InternalClock additional coverage") {
    SECTION("setSampleRate() with invalid values") {
        InternalClock clock;

        SECTION("Zero sample rate is rejected") {
            clock.setSampleRate(0.0);
            REQUIRE(clock.getSampleRate() == 44100.0);  // Default unchanged
        }

        SECTION("Negative sample rate is rejected") {
            clock.setSampleRate(-48000.0);
            REQUIRE(clock.getSampleRate() == 44100.0);  // Default unchanged
        }

        SECTION("Positive sample rate is accepted") {
            clock.setSampleRate(96000.0);
            REQUIRE(clock.getSampleRate() == 96000.0);
        }
    }

    SECTION("Multiple pulse triggers accumulate remainder correctly") {
        InternalClock clock;
        clock.setSampleRate(48000.0);
        clock.setBpm(120.0);
        clock.setSubdivision(Subdivision::SIXTEENTH);
        clock.start();

        // Process a huge buffer that covers multiple pulses
        // At 120 BPM, 16th notes: 6000 samples per pulse
        // 18000 samples = 3 pulses
        int pulseCount = 0;
        // Process in one big block (larger than one pulse)
        bool triggered = clock.processSamples(18000);
        if (triggered) pulseCount++;
        // The clock only returns true once per call, so additional pulses are lost
        // unless processed in smaller blocks
        REQUIRE(pulseCount == 1);  // Only one trigger per call
    }

    SECTION("calculateSamplesPerPulse() for various subdivisions") {
        InternalClock clock;
        clock.setSampleRate(48000.0);
        clock.setBpm(120.0);

        SECTION("Quarter notes: 48000 / (120 * 1 / 60) = 24000") {
            clock.setSubdivision(Subdivision::QUARTER);
            REQUIRE(clock.calculateSamplesPerPulse() == Catch::Approx(24000.0));
        }

        SECTION("Eighth notes: 48000 / (120 * 2 / 60) = 12000") {
            clock.setSubdivision(Subdivision::EIGHTH);
            REQUIRE(clock.calculateSamplesPerPulse() == Catch::Approx(12000.0));
        }

        SECTION("32nd notes: 48000 / (120 * 8 / 60) = 3000") {
            clock.setSubdivision(Subdivision::THIRTY_SECOND);
            REQUIRE(clock.calculateSamplesPerPulse() == Catch::Approx(3000.0));
        }
    }
}

// ============================================================================
// Clock - Additional Coverage
// ============================================================================

TEST_CASE("Clock additional coverage") {
    SECTION("calculateSamplesPerStep() in INTERNAL mode") {
        Clock clock;
        clock.setMode(ClockMode::INTERNAL);
        clock.setSampleRate(48000.0);
        clock.setBpm(120.0);
        clock.setSubdivision(Subdivision::SIXTEENTH);

        double samplesPerStep = clock.calculateSamplesPerStep(0.0, 48000.0);
        REQUIRE(samplesPerStep == Catch::Approx(6000.0));
    }

    SECTION("calculateSamplesPerStep() in DAW_SYNC mode") {
        Clock clock;
        clock.setMode(ClockMode::DAW_SYNC);
        clock.setSubdivision(Subdivision::SIXTEENTH);

        double samplesPerStep = clock.calculateSamplesPerStep(120.0, 48000.0);
        REQUIRE(samplesPerStep == Catch::Approx(6000.0));
    }

    SECTION("calculateSamplesPerStep() with invalid DAW params") {
        Clock clock;
        clock.setMode(ClockMode::DAW_SYNC);

        REQUIRE(clock.calculateSamplesPerStep(0.0, 48000.0) == 0.0);
        REQUIRE(clock.calculateSamplesPerStep(120.0, 0.0) == 0.0);
        REQUIRE(clock.calculateSamplesPerStep(-1.0, 48000.0) == 0.0);
    }

    SECTION("DAW sync loop detection (pulse count goes backwards)") {
        Clock clock;
        clock.setMode(ClockMode::DAW_SYNC);

        // Normal advancement
        clock.updateDawSync(1);
        REQUIRE(clock.shouldTick() == true);

        clock.updateDawSync(2);
        REQUIRE(clock.shouldTick() == true);

        // Simulate DAW loop: pulse count goes backwards
        clock.updateDawSync(0);
        // After reset, advancing should trigger again
        clock.updateDawSync(1);
        REQUIRE(clock.shouldTick() == true);
    }

    SECTION("MIDI clock pulse accumulation") {
        Clock clock;
        clock.setMode(ClockMode::INTERNAL);
        clock.setSampleRate(48000.0);
        clock.setBpm(120.0);
        clock.setSubdivision(Subdivision::SIXTEENTH);
        clock.start();

        // Initially no pulses accumulated
        REQUIRE(clock.getMidiClockPulsesToSend() == 0);

        // Accumulate a pulse
        clock.accumulateMidiClockPulse();
        int pulses = clock.getMidiClockPulsesToSend();
        REQUIRE(pulses == 6);  // 16th note subdivision: 24/4 = 6 MIDI clocks

        // After reading, counter is reset
        REQUIRE(clock.getMidiClockPulsesToSend() == 0);
    }

    SECTION("MIDI clock pulses not sent in DAW_SYNC mode") {
        Clock clock;
        clock.setMode(ClockMode::DAW_SYNC);
        clock.accumulateMidiClockPulse();

        REQUIRE(clock.getMidiClockPulsesToSend() == 0);
    }

    SECTION("start() in DAW_SYNC mode does nothing") {
        Clock clock;
        clock.setMode(ClockMode::DAW_SYNC);
        clock.start();
        REQUIRE(clock.isRunning() == false);
    }

    SECTION("stop() in DAW_SYNC mode does nothing") {
        Clock clock;
        clock.setMode(ClockMode::DAW_SYNC);
        clock.stop();
        REQUIRE(clock.isRunning() == false);
    }
}

// ============================================================================
// DawSync - Additional Coverage
// ============================================================================

TEST_CASE("DawSync additional coverage") {
    SECTION("calculatePulseCount() for all subdivisions") {
        DawSync dawSync;

        SECTION("Quarter note subdivision at 4.0 PPQ = 4 pulses") {
            dawSync.setSubdivision(Subdivision::QUARTER);
            REQUIRE(dawSync.calculatePulseCount(4.0) == 4);
        }

        SECTION("32nd note subdivision at 1.0 PPQ = 8 pulses") {
            dawSync.setSubdivision(Subdivision::THIRTY_SECOND);
            REQUIRE(dawSync.calculatePulseCount(1.0) == 8);
        }

        SECTION("Zero PPQ position = 0 pulses") {
            dawSync.setSubdivision(Subdivision::SIXTEENTH);
            REQUIRE(dawSync.calculatePulseCount(0.0) == 0);
        }
    }

    SECTION("update() with no position info") {
        DawSync dawSync;

        juce::Optional<juce::AudioPlayHead::PositionInfo> noPosition;
        int pulseCount = dawSync.update(noPosition);

        REQUIRE(pulseCount == 0);
        REQUIRE(dawSync.getIsPlaying() == false);
    }

    SECTION("update() with playing DAW") {
        DawSync dawSync;
        dawSync.setSubdivision(Subdivision::SIXTEENTH);

        juce::AudioPlayHead::PositionInfo pos;
        pos.setIsPlaying(true);
        pos.setPpqPosition(1.0);  // 1 quarter note in

        int pulseCount = dawSync.update(juce::makeOptional(pos));

        REQUIRE(dawSync.getIsPlaying() == true);
        REQUIRE(pulseCount == 4);  // 1.0 * 4 = 4 pulses for 16th notes
    }

    SECTION("update() with DAW stop resets state") {
        DawSync dawSync;
        dawSync.setSubdivision(Subdivision::SIXTEENTH);

        // Start playing
        juce::AudioPlayHead::PositionInfo playing;
        playing.setIsPlaying(true);
        playing.setPpqPosition(2.0);
        dawSync.update(juce::makeOptional(playing));
        REQUIRE(dawSync.getIsPlaying() == true);

        // Stop
        juce::AudioPlayHead::PositionInfo stopped;
        stopped.setIsPlaying(false);
        dawSync.update(juce::makeOptional(stopped));

        REQUIRE(dawSync.getIsPlaying() == false);
        REQUIRE(dawSync.getCurrentPulseCount() == 0);
        REQUIRE(dawSync.getLastPpqPosition() == 0.0);
    }

    SECTION("update() with advancing playhead") {
        DawSync dawSync;
        dawSync.setSubdivision(Subdivision::SIXTEENTH);

        juce::AudioPlayHead::PositionInfo pos;
        pos.setIsPlaying(true);

        // Frame 1: at PPQ 0.0
        pos.setPpqPosition(0.0);
        int pulse1 = dawSync.update(juce::makeOptional(pos));
        REQUIRE(pulse1 == 0);

        // Frame 2: at PPQ 0.5 (half a quarter note)
        pos.setPpqPosition(0.5);
        int pulse2 = dawSync.update(juce::makeOptional(pos));
        REQUIRE(pulse2 == 2);  // 0.5 * 4 = 2

        // Frame 3: at PPQ 1.0
        pos.setPpqPosition(1.0);
        int pulse3 = dawSync.update(juce::makeOptional(pos));
        REQUIRE(pulse3 == 4);  // 1.0 * 4 = 4
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
