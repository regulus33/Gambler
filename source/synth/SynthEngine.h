#pragma once

#if GAMBLER_HAS_SYNTH

#include "SynthVoice.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

/**
 * 8-voice polyphonic synth engine with sample-accurate MIDI dispatch.
 * Reads parameters from APVTS, processes MIDI from the buffer the sequencer just filled.
 */
class SynthEngine
{
public:
    explicit SynthEngine (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate, int blockSize);
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

private:
    static constexpr int NUM_VOICES = 8;

    juce::AudioProcessorValueTreeState& apvts;
    std::array<SynthVoice, NUM_VOICES> voices;
    juce::uint32 noteOnCounter = 0;
    float masterGain = 0.7f;

    void handleNoteOn (int midiNote, float velocity);
    void handleNoteOff (int midiNote);
    void renderVoices (float* outL, float* outR, int numSamples);
    void updateVoiceParams();

    SynthVoice* findFreeVoice();
};

#endif // GAMBLER_HAS_SYNTH
