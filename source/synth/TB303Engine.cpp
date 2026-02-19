#include "TB303Engine.h"

//==============================================================================
TB303Engine::TB303Engine()
{
    // Initialise reverb params
    reverbParams.roomSize   = 0.5f;
    reverbParams.damping    = 0.5f;
    reverbParams.wetLevel   = 0.0f;
    reverbParams.dryLevel   = 1.0f;
    reverbParams.width      = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    // Instant (no smoothing) initial values — will be set on first step
    sCutoff.setCurrentAndTargetValue(64.0f);
    sResonance.setCurrentAndTargetValue(64.0f);
    sEnvMod.setCurrentAndTargetValue(64.0f);
    sDecay.setCurrentAndTargetValue(64.0f);
    sAccent.setCurrentAndTargetValue(0.0f);
    sWaveform.setCurrentAndTargetValue(0.0f);
}

//==============================================================================
void TB303Engine::prepare(double sr, int blockSize)
{
    sampleRate = sr;
    open303.setSampleRate(sr);
    reverb.reset();
    monoBuffer.setSize(1, blockSize);
}

//==============================================================================
void TB303Engine::setStepParams(int cutoff, int resonance, int envMod,
                                 int decay, int accent, int waveform, int slewMidi)
{
    // Convert slew MIDI value (0-127) to ms, then to samples
    const float slewMs     = static_cast<float>(slewMidi) * (500.0f / 127.0f);
    const int   slewSamples = juce::jmax(1, juce::roundToInt(slewMs * 0.001f * (float)sampleRate));

    sCutoff.reset(slewSamples);
    sResonance.reset(slewSamples);
    sEnvMod.reset(slewSamples);
    sDecay.reset(slewSamples);
    sAccent.reset(slewSamples);
    sWaveform.reset(slewSamples);

    sCutoff   .setTargetValue(static_cast<float>(juce::jlimit(0, 127, cutoff)));
    sResonance.setTargetValue(static_cast<float>(juce::jlimit(0, 127, resonance)));
    sEnvMod   .setTargetValue(static_cast<float>(juce::jlimit(0, 127, envMod)));
    sDecay    .setTargetValue(static_cast<float>(juce::jlimit(0, 127, decay)));
    sAccent   .setTargetValue(static_cast<float>(juce::jlimit(0, 127, accent)));
    sWaveform .setTargetValue(static_cast<float>(juce::jlimit(0, 127, waveform)));
}

//==============================================================================
void TB303Engine::setTuning(float semitones)
{
    const double a4Hz = 440.0 * std::pow(2.0, static_cast<double>(semitones) / 12.0);
    open303.setTuning(a4Hz);
}

void TB303Engine::setVolume(float gain)
{
    masterGain = gain;
}

void TB303Engine::setDistDrive(float drive)
{
    distDrive = drive;
}

void TB303Engine::setDistMix(float mix)
{
    distMix = mix;
}

void TB303Engine::setReverbWet(float wet)
{
    reverbParams.wetLevel = wet;
    reverbParams.dryLevel = 1.0f - wet;
    reverb.setParameters(reverbParams);
}

//==============================================================================
void TB303Engine::processBlock(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
{
    const int numSamples = buffer.getNumSamples();

    // Resize mono buffer if needed
    if (monoBuffer.getNumSamples() < numSamples)
        monoBuffer.setSize(1, numSamples, false, false, true);

    float* mono = monoBuffer.getWritePointer(0);

    // Walk MIDI events and samples together
    auto midiIt  = midi.cbegin();
    auto midiEnd = midi.cend();

    for (int i = 0; i < numSamples; ++i) {
        // Handle any MIDI events that occur at this sample position
        while (midiIt != midiEnd) {
            const auto metadata = *midiIt;
            if (metadata.samplePosition > i)
                break;

            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                open303.noteOn(msg.getNoteNumber(), msg.getVelocity(), 0.0);
            else if (msg.isNoteOff())
                open303.noteOn(msg.getNoteNumber(), 0, 0.0);  // velocity 0 = note off

            ++midiIt;
        }

        // Advance smoothers and apply to engine
        const float cv  = sCutoff   .getNextValue();
        const float rv  = sResonance.getNextValue();
        const float ev  = sEnvMod   .getNextValue();
        const float dv  = sDecay    .getNextValue();
        const float av  = sAccent   .getNextValue();
        const float wv  = sWaveform .getNextValue();

        open303.setCutoff   (cutoffToHz    (cv));
        open303.setResonance(resonanceToPct(rv));
        open303.setEnvMod   (envModToPct   (ev));
        open303.setDecay    (decayToMs     (dv));
        open303.setAccent   (accentToPct   (av));
        open303.setWaveform (waveformToBlend(wv));

        // Generate one sample
        float s = static_cast<float>(open303.getSample());

        // Apply distortion
        if (distMix > 0.001f)
            s = s * (1.0f - distMix) + applyDist(s) * distMix;

        mono[i] = s;
    }

    // Apply reverb to mono signal
    reverb.processMono(mono, numSamples);

    // Apply master gain and write to output buffer (stereo — both channels get the mono signal)
    const int numOut = buffer.getNumChannels();
    if (numOut >= 1) {
        juce::FloatVectorOperations::addWithMultiply(buffer.getWritePointer(0), mono,
                                                     masterGain, numSamples);
    }
    if (numOut >= 2) {
        juce::FloatVectorOperations::addWithMultiply(buffer.getWritePointer(1), mono,
                                                     masterGain, numSamples);
    }
}
