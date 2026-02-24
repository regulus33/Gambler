#pragma once

#if GAMBLER_HAS_SYNTH

#include "Oscillator.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/**
 * Single synth voice: VectorOscillator -> LadderFilter (with envelope) -> Amp envelope.
 * Mono signal path, stereo output (mono duplicated to L/R).
 */
class SynthVoice
{
public:
    SynthVoice() = default;

    void prepare (double sampleRate, int blockSize)
    {
        currentSampleRate = sampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (blockSize);
        spec.numChannels = 1;
        filter.prepare (spec);
        filter.setMode (juce::dsp::LadderFilterMode::LPF24);

        ampEnv.setSampleRate (sampleRate);
        filterEnv.setSampleRate (sampleRate);
    }

    void noteOn (int midiNote, float velocity, juce::uint32 order)
    {
        noteNumber = midiNote;
        noteVelocity = velocity;
        noteOnOrder = order;
        active = true;

        double freq = 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
        osc.setFrequency (freq, currentSampleRate);
        osc.reset();

        ampEnv.noteOn();
        filterEnv.noteOn();
    }

    void noteOff()
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
    }

    void updateParams (float blend, float cutoff, float reso, float drive,
                       juce::ADSR::Parameters ampParams, juce::ADSR::Parameters fltParams,
                       float envDepth)
    {
        osc.setBlend (blend);
        baseCutoff = cutoff;
        filterEnvDepth = envDepth;

        filter.setResonance (reso);
        filter.setDrive (drive);

        ampEnv.setParameters (ampParams);
        filterEnv.setParameters (fltParams);
    }

    void renderBlock (float* outL, float* outR, int numSamples)
    {
        if (!active)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            float envVal = filterEnv.getNextSample();
            float modulatedCutoff = baseCutoff * std::pow (2.0f, envVal * filterEnvDepth * 4.0f);
            modulatedCutoff = juce::jlimit (20.0f, 20000.0f, modulatedCutoff);
            filter.setCutoffFrequencyHz (modulatedCutoff);

            filterSampleBuffer = osc.getNextSample();

            // Process 1-sample block through ladder filter
            auto block = juce::dsp::AudioBlock<float> (&filterSamplePtr, 1, 1);
            filter.process (juce::dsp::ProcessContextReplacing<float> (block));

            float ampVal = ampEnv.getNextSample();
            float out = filterSampleBuffer * ampVal * noteVelocity;

            outL[i] += out;
            outR[i] += out;
        }

        // Voice is done when amp envelope finishes release
        if (!ampEnv.isActive())
        {
            active = false;
            filter.reset();
        }
    }

    bool isActive() const { return active; }
    bool isInRelease() const { return active && !ampEnv.isActive(); }
    int getNoteNumber() const { return noteNumber; }
    juce::uint32 getNoteOnOrder() const { return noteOnOrder; }

private:
    VectorOscillator osc;
    juce::ADSR ampEnv;
    juce::ADSR filterEnv;
    juce::dsp::LadderFilter<float> filter;

    double currentSampleRate = 44100.0;
    float baseCutoff = 8000.0f;
    float filterEnvDepth = 0.5f;
    float noteVelocity = 1.0f;
    int noteNumber = -1;
    juce::uint32 noteOnOrder = 0;
    bool active = false;

    // Scratch space for single-sample filter processing
    float filterSampleBuffer = 0.0f;
    float* filterSamplePtr = &filterSampleBuffer;
};

#endif // GAMBLER_HAS_SYNTH
