#if GAMBLER_HAS_SYNTH

#include "SynthEngine.h"

SynthEngine::SynthEngine (juce::AudioProcessorValueTreeState& state)
    : apvts (state)
{
}

void SynthEngine::prepare (double sampleRate, int blockSize)
{
    for (auto& voice : voices)
        voice.prepare (sampleRate, blockSize);
}

void SynthEngine::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Read params once per block
    updateVoiceParams();

    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : outL;

    int numSamples = buffer.getNumSamples();

    // Sample-accurate MIDI dispatch: render sub-blocks between MIDI events
    int currentSample = 0;

    for (const auto& metadata : midiMessages)
    {
        int eventSample = metadata.samplePosition;

        // Render audio up to this event
        if (eventSample > currentSample)
        {
            renderVoices (outL + currentSample, outR + currentSample, eventSample - currentSample);
            currentSample = eventSample;
        }

        auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            float velocity = msg.getFloatVelocity();
            handleNoteOn (msg.getNoteNumber(), velocity);
        }
        else if (msg.isNoteOff())
        {
            handleNoteOff (msg.getNoteNumber());
        }
    }

    // Render remaining samples after last MIDI event
    if (currentSample < numSamples)
    {
        renderVoices (outL + currentSample, outR + currentSample, numSamples - currentSample);
    }

    // Apply master gain
    if (masterGain != 1.0f)
    {
        buffer.applyGain (masterGain);
    }
}

void SynthEngine::updateVoiceParams()
{
    auto getFloat = [this] (const juce::String& id) -> float {
        if (auto* p = apvts.getRawParameterValue (id))
            return p->load();
        return 0.0f;
    };

    float blend = getFloat ("synth_osc_blend");
    float cutoff = getFloat ("synth_filter_cutoff");
    float reso = getFloat ("synth_filter_reso");
    float drive = getFloat ("synth_filter_drive");
    float envDepth = getFloat ("synth_flt_env_depth");
    masterGain = getFloat ("synth_master_gain");

    juce::ADSR::Parameters ampParams {
        getFloat ("synth_amp_attack"),
        getFloat ("synth_amp_decay"),
        getFloat ("synth_amp_sustain"),
        getFloat ("synth_amp_release")
    };

    juce::ADSR::Parameters fltParams {
        getFloat ("synth_flt_attack"),
        getFloat ("synth_flt_decay"),
        getFloat ("synth_flt_sustain"),
        getFloat ("synth_flt_release")
    };

    for (auto& voice : voices)
    {
        if (voice.isActive())
            voice.updateParams (blend, cutoff, reso, drive, ampParams, fltParams, envDepth);
    }
}

void SynthEngine::handleNoteOn (int midiNote, float velocity)
{
    // Check if this note is already playing — retrigger it
    for (auto& voice : voices)
    {
        if (voice.isActive() && voice.getNoteNumber() == midiNote)
        {
            voice.noteOn (midiNote, velocity, noteOnCounter++);
            return;
        }
    }

    auto* voice = findFreeVoice();
    if (voice != nullptr)
    {
        // Update params before starting the voice
        auto getFloat = [this] (const juce::String& id) -> float {
            if (auto* p = apvts.getRawParameterValue (id))
                return p->load();
            return 0.0f;
        };

        float blend = getFloat ("synth_osc_blend");
        float cutoff = getFloat ("synth_filter_cutoff");
        float reso = getFloat ("synth_filter_reso");
        float drive = getFloat ("synth_filter_drive");
        float envDepth = getFloat ("synth_flt_env_depth");

        juce::ADSR::Parameters ampParams {
            getFloat ("synth_amp_attack"),
            getFloat ("synth_amp_decay"),
            getFloat ("synth_amp_sustain"),
            getFloat ("synth_amp_release")
        };

        juce::ADSR::Parameters fltParams {
            getFloat ("synth_flt_attack"),
            getFloat ("synth_flt_decay"),
            getFloat ("synth_flt_sustain"),
            getFloat ("synth_flt_release")
        };

        voice->updateParams (blend, cutoff, reso, drive, ampParams, fltParams, envDepth);
        voice->noteOn (midiNote, velocity, noteOnCounter++);
    }
}

void SynthEngine::handleNoteOff (int midiNote)
{
    for (auto& voice : voices)
    {
        if (voice.isActive() && voice.getNoteNumber() == midiNote)
        {
            voice.noteOff();
            return;
        }
    }
}

void SynthEngine::renderVoices (float* outL, float* outR, int numSamples)
{
    for (auto& voice : voices)
    {
        if (voice.isActive())
            voice.renderBlock (outL, outR, numSamples);
    }
}

SynthVoice* SynthEngine::findFreeVoice()
{
    // Priority 1: free (inactive) voice
    for (auto& voice : voices)
    {
        if (!voice.isActive())
            return &voice;
    }

    // Priority 2: voice in release phase
    for (auto& voice : voices)
    {
        if (voice.isInRelease())
            return &voice;
    }

    // Priority 3: oldest active voice (lowest noteOnOrder)
    SynthVoice* oldest = &voices[0];
    for (auto& voice : voices)
    {
        if (voice.getNoteOnOrder() < oldest->getNoteOnOrder())
            oldest = &voice;
    }
    return oldest;
}

#endif // GAMBLER_HAS_SYNTH
