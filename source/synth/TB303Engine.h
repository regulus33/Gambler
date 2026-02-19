#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Suppress warnings from vendored open303 headers
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wmissing-exception-specification",
                                     "-Wshadow",
                                     "-Wdeprecated-declarations",
                                     "-Wunused-function",
                                     "-Wunused-variable",
                                     "-Wsign-compare")
#include "open303/rosic_Open303.h"
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

/**
 * TB303Engine — thin wrapper around rosic::Open303.
 *
 * Responsibilities:
 *  - Sample-accurate MIDI note-on/off forwarding to open303
 *  - Per-step parameter smoothing via juce::SmoothedValue (slew / glide between step states)
 *  - Post-processing: distortion waveshaper + reverb
 *  - Global params: tuning, master volume, dist drive, dist mix, reverb wet
 *
 * Usage:
 *   tb303.prepare(sampleRate, blockSize);
 *   // On step advance:
 *   tb303.setStepParams(cutoff, res, envMod, decay, accent, waveform, slewMs);
 *   // Each block:
 *   tb303.setGlobalParams(tuning, volume, distDrive, distMix, reverbWet);
 *   tb303.processBlock(audioBuffer, midiBuffer); // adds audio to buffer
 */
class TB303Engine {
public:
    TB303Engine();

    void prepare(double sampleRate, int blockSize);

    /**
     * Process one block of audio.
     * Reads MIDI note-on/off events from midiBuffer and synthesises audio.
     * Output is ADDED to the existing content of audioBuffer (both channels).
     * Call buffer.clear() before this if you want only synth audio.
     */
    void processBlock(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi);

    /**
     * Called when the sequencer advances to a new step.
     * All params are in 0–127 integer range (the raw stepModValue).
     * Slew controls how long the parameter ramp takes (0 = instant, 127 = 500 ms).
     */
    void setStepParams(int cutoff, int resonance, int envMod,
                       int decay, int accent, int waveform, int slewMidi);

    // ---- Global params (set once per block from APVTS) ----

    /** Tuning offset in semitones relative to A4=440 Hz. Range: -12..+12 */
    void setTuning(float semitones);

    /** Master output gain, linear 0..1. */
    void setVolume(float gain);

    /** Distortion drive amount, 0..1. */
    void setDistDrive(float drive);

    /** Dry/wet mix for distortion, 0..1. */
    void setDistMix(float mix);

    /** Reverb wet level, 0..1. */
    void setReverbWet(float wet);

private:
    rosic::Open303 open303;
    double sampleRate = 44100.0;

    // Per-step parameter smoothers (raw 0-127 domain, converted to physical at each sample)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sCutoff;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sResonance;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sEnvMod;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sDecay;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sAccent;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sWaveform;

    // Global params (set each block)
    float masterGain  = 0.75f;
    float distDrive   = 0.0f;
    float distMix     = 0.0f;

    // Reverb
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;

    // Temporary mono processing buffer
    juce::AudioBuffer<float> monoBuffer;

    // ---- Helper conversions ----

    /** Map 0-127 cutoff to Hz on a log scale (20 Hz – 8000 Hz). */
    static double cutoffToHz(float v) noexcept {
        return 20.0 * std::pow(400.0, static_cast<double>(v) / 127.0);
    }

    /** Map 0-127 resonance to 0.0–0.99. */
    static double resonanceToPct(float v) noexcept {
        return (static_cast<double>(v) / 127.0) * 0.99;
    }

    /** Map 0-127 env mod to 0–100 %. */
    static double envModToPct(float v) noexcept {
        return (static_cast<double>(v) / 127.0) * 100.0;
    }

    /** Map 0-127 decay to 20–2000 ms on a log scale. */
    static double decayToMs(float v) noexcept {
        return 20.0 * std::pow(100.0, static_cast<double>(v) / 127.0);
    }

    /** Map 0-127 accent to 0–100 %. */
    static double accentToPct(float v) noexcept {
        return (static_cast<double>(v) / 127.0) * 100.0;
    }

    /** Map 0-127 waveform to 0.0 (SAW) – 1.0 (SQR). */
    static double waveformToBlend(float v) noexcept {
        return static_cast<double>(v) / 127.0;
    }

    /** Soft-clip distortion waveshaper.  drive=0 → bypass, drive=1 → heavy saturation. */
    float applyDist(float x) const noexcept {
        if (distDrive < 0.001f) return x;
        const float k = distDrive * 39.999f + 0.001f;   // 0.001 .. 40
        return std::tanh(k * x) / std::tanh(k);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TB303Engine)
};
