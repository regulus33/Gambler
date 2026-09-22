#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

/**
 * Manages note-off scheduling for the mono sequencer.
 *
 * Tracks the currently-playing note and an optional sample-accurate
 * scheduled note-off time so the caller doesn't need to juggle five
 * separate member variables.
 *
 * All methods are intended to be called from the audio thread (or,
 * for the preview helpers, from any single thread at a time).
 */
class NoteOffScheduler
{
public:
    /** Call once per processBlock to check for and emit a scheduled note-off.
     *  Must be called *before* advanceSamplePosition().
     *
     *  @param midiMessages   Buffer to add the note-off event into
     *  @param midiChannel    MIDI channel (1-16)
     *  @param midiOutput     Optional standalone MIDI output (may be nullptr)
     *  @param currentSamplePos  The sample position at the start of this buffer
     *  @param numSamples     Number of samples in this buffer
     *  @return true if a note-off was emitted (caller can fire onNoteOff callback)
     */
    bool processScheduledNoteOff (juce::MidiBuffer& midiMessages,
                                  int midiChannel,
                                  juce::MidiOutput* midiOutput,
                                  juce::int64 currentSamplePos,
                                  int numSamples)
    {
        if (scheduledNoteOffSample < 0)
            return false;

        juce::int64 bufferEndSample = currentSamplePos + numSamples;

        // Past-due targets (scheduled inside the block that created them,
        // which is always processed one block later) fire at offset 0 —
        // a few ms late beats never, which left the note ringing until
        // the next step's immediate note-off.
        if (scheduledNoteOffSample < bufferEndSample)
        {
            int sampleOffset = static_cast<int> (scheduledNoteOffSample - currentSamplePos);
            sampleOffset = juce::jlimit (0, numSamples - 1, sampleOffset);

            auto noteOff = juce::MidiMessage::noteOff (midiChannel, scheduledNoteOffNote, scheduledNoteOffVelocity);
            midiMessages.addEvent (noteOff, sampleOffset);

            if (midiOutput)
                midiOutput->sendMessageNow (noteOff);

            DBG ("Scheduled MIDI Note OFF: " << scheduledNoteOffNote
                 << " at sample " << scheduledNoteOffSample
                 << " (offset " << sampleOffset << " in buffer)");

            clearScheduled();
            return true;
        }

        return false;
    }

    /** Send an immediate note-off for the last played note (sample offset 0).
     *  Used when a new step starts and the previous note must stop right away.
     *
     *  @param midiMessages  Buffer to add the event into
     *  @param midiChannel   MIDI channel (1-16)
     *  @param midiOutput    Optional standalone output (may be nullptr)
     */
    void sendImmediateNoteOff (juce::MidiBuffer& midiMessages,
                               int midiChannel,
                               juce::MidiOutput* midiOutput,
                               int sampleOffsetInBlock = 0)
    {
        cancelScheduled();

        if (lastPlayedNote >= 0)
        {
            auto noteOff = juce::MidiMessage::noteOff (midiChannel, lastPlayedNote, (juce::uint8) 0);
            midiMessages.addEvent (noteOff, sampleOffsetInBlock);

            if (midiOutput)
                midiOutput->sendMessageNow (noteOff);

            DBG ("MIDI Note OFF: " << lastPlayedNote);
        }
    }

    /** Send a note-off for the last played note via standalone or preview FIFO.
     *  Used by triggerStepMidi / triggerCurrentStepMidi (UI preview path).
     *
     *  @param midiChannel   MIDI channel (1-16)
     *  @param midiOutput    Standalone output (may be nullptr — uses FIFO path)
     *  @param previewFifo   FIFO to push note-off into when midiOutput is null
     */
    template <typename FifoType>
    void sendPreviewNoteOff (int midiChannel,
                             juce::MidiOutput* midiOutput,
                             FifoType& previewFifo)
    {
        if (lastPlayedNote >= 0)
        {
            if (midiOutput)
            {
                auto noteOff = juce::MidiMessage::noteOff (midiChannel, lastPlayedNote, (juce::uint8) 0);
                midiOutput->sendMessageNow (noteOff);
                DBG ("Preview MIDI Note OFF (standalone): " << lastPlayedNote);
            }
            else
            {
                previewFifo.push ({ lastPlayedNote, false, 0 });
                DBG ("Preview MIDI Note OFF queued (DAW): " << lastPlayedNote);
            }
        }
    }

    /** Schedule a future note-off at an exact sample position. */
    void scheduleNoteOff (int note, juce::int64 noteOffSample, juce::uint8 velocity = 0)
    {
        scheduledNoteOffSample    = noteOffSample;
        scheduledNoteOffNote      = note;
        scheduledNoteOffVelocity  = velocity;
    }

    /** Cancel any pending scheduled note-off (without sending anything). */
    void cancelScheduled()
    {
        scheduledNoteOffSample = -1;
        scheduledNoteOffNote   = -1;
    }

    /** Record which note is currently playing (for future note-off). */
    void setLastPlayedNote (int note) { lastPlayedNote = note; }

    /** Get the last note that was played (-1 if none). */
    int getLastPlayedNote() const { return lastPlayedNote; }

    /** Clear all state — last played note + any scheduled note-off. */
    void reset()
    {
        lastPlayedNote = -1;
        clearScheduled();
    }

    /** Send a final note-off for any playing note and reset.
     *  Used when the sequencer stops. Only sends via midiOutput (standalone)
     *  since there is no MidiBuffer available outside processBlock. */
    void stopAndClear (int midiChannel, juce::MidiOutput* midiOutput)
    {
        if (lastPlayedNote >= 0)
        {
            auto noteOff = juce::MidiMessage::noteOff (midiChannel, lastPlayedNote, (juce::uint8) 0);
            if (midiOutput)
                midiOutput->sendMessageNow (noteOff);
        }

        reset();
    }

private:
    void clearScheduled()
    {
        scheduledNoteOffSample   = -1;
        scheduledNoteOffNote     = -1;
        scheduledNoteOffVelocity = 0;
    }

    int          lastPlayedNote          = -1;
    juce::int64  scheduledNoteOffSample  = -1;
    int          scheduledNoteOffNote    = -1;
    juce::uint8  scheduledNoteOffVelocity = 0;
};
