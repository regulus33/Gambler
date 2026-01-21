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
