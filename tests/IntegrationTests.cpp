#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../source/backend/Clock.h"
#include "../source/backend/ProcessorCommand.h"
#include "../source/backend/Scale.h"

/**
 * Integration Tests
 *
 * These tests verify that multiple components work correctly together,
 * testing the integration points between domain classes.
 */

SCENARIO("Clock mode switching integration", "[integration]") {
    GIVEN("A Clock in INTERNAL mode") {
        Clock clock;
        clock.setMode(ClockMode::INTERNAL);
        clock.setSampleRate(48000.0);
        clock.setBpm(120.0);
        clock.start();

        // Process some samples to accumulate state
        clock.processInternalClock(1000);

        WHEN("Switching to DAW_SYNC mode") {
            clock.setMode(ClockMode::DAW_SYNC);

            WHEN("Updating with DAW position") {
                clock.updateDawSync(1);

                THEN("Clock ticks based on DAW position") {
                    REQUIRE(clock.shouldTick() == true);
                }
            }
        }

        WHEN("Switching back to INTERNAL mode") {
            clock.setMode(ClockMode::DAW_SYNC);
            clock.setMode(ClockMode::INTERNAL);

            THEN("Can process samples again") {
                clock.processInternalClock(6000);  // Enough for one tick at 120 BPM
                REQUIRE(clock.shouldTick() == true);
            }
        }
    }
}

SCENARIO("ProcessorCommandQueue basic operations", "[integration]") {
    GIVEN("A ProcessorCommandQueue") {
        ProcessorCommandQueue queue;
        int counter = 0;

        WHEN("Queueing multiple commands") {
            queue.push(ProcessorCommand([&counter]() {
                counter++;
            }));

            queue.push(ProcessorCommand([&counter]() {
                counter += 10;
            }));

            queue.push(ProcessorCommand([&counter]() {
                counter *= 2;
            }));

            THEN("All commands can be executed in order") {
                int executed = queue.executeAll();
                REQUIRE(executed == 3);
                REQUIRE(counter == 22);  // (0 + 1 + 10) * 2 = 22
            }
        }
    }
}

SCENARIO("Scale quantization edge cases", "[integration]") {
    GIVEN("A pentatonic scale (C, D, E, G, A)") {
        Scale scale;
        scale.setScaleFromNames({"C", "D", "E", "G", "A"});

        WHEN("Quantizing notes not in scale") {
            THEN("F (65) quantizes to E (64)") {
                REQUIRE(scale.quantize(65) == 64);
            }

            THEN("Bb (70) quantizes to A (69)") {
                REQUIRE(scale.quantize(70) == 69);
            }

            THEN("Gb (66) quantizes to G (67)") {
                REQUIRE(scale.quantize(66) == 67);
            }
        }
    }
}

SCENARIO("Clock timing accuracy", "[integration]") {
    GIVEN("A Clock at different BPMs") {
        Clock clock;
        clock.setMode(ClockMode::INTERNAL);
        clock.setSampleRate(48000.0);
        clock.setSubdivision(Subdivision::SIXTEENTH);

        WHEN("Running at 60 BPM") {
            clock.setBpm(60.0);
            clock.start();

            // At 60 BPM with 16th notes, we get 4 pulses per second
            // 48000 / 4 = 12000 samples per pulse
            int pulseCount = 0;
            for (int i = 0; i < 48; i++) {  // 48 blocks of 1000 samples = 48000 = 1 second
                clock.processInternalClock(1000);
                if (clock.shouldTick()) {
                    pulseCount++;
                }
            }

            THEN("Generates 4 pulses per second") {
                REQUIRE(pulseCount == 4);
            }
        }

        WHEN("Running at 180 BPM") {
            clock.setBpm(180.0);
            clock.start();

            // At 180 BPM with 16th notes, we get 12 pulses per second
            // 48000 / 12 = 4000 samples per pulse
            int pulseCount = 0;
            for (int i = 0; i < 48; i++) {  // 48 blocks of 1000 samples = 48000 = 1 second
                clock.processInternalClock(1000);
                if (clock.shouldTick()) {
                    pulseCount++;
                }
            }

            THEN("Generates 12 pulses per second") {
                REQUIRE(pulseCount == 12);
            }
        }
    }
}
