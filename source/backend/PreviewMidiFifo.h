#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

/**
 * Thread-safe FIFO for passing preview MIDI messages from the UI thread
 * to the audio thread. Wraps juce::AbstractFifo + a fixed-size buffer.
 */
class PreviewMidiFifo {
public:
    struct Message {
        int noteNumber;
        bool isNoteOn;
        juce::uint8 velocity;
    };

    static constexpr int capacity = 32;

    /** Push a single message (call from UI thread). */
    void push (Message msg)
    {
        int start1, size1, start2, size2;
        fifo.prepareToWrite (1, start1, size1, start2, size2);
        if (size1 > 0) {
            buffer[static_cast<size_t> (start1)] = msg;
            fifo.finishedWrite (1);
        }
    }

    /** Drain all queued messages into a MidiBuffer at sample offset 0.
     *  Returns the number of messages drained. */
    int drainInto (juce::MidiBuffer& midiMessages, int midiChannel)
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead (fifo.getNumReady(), start1, size1, start2, size2);

        auto addMsg = [&] (const Message& msg) {
            if (msg.isNoteOn) {
                midiMessages.addEvent (
                    juce::MidiMessage::noteOn (midiChannel, msg.noteNumber, msg.velocity), 0);
            } else {
                midiMessages.addEvent (
                    juce::MidiMessage::noteOff (midiChannel, msg.noteNumber, msg.velocity), 0);
            }
        };

        for (int i = 0; i < size1; ++i)
            addMsg (buffer[static_cast<size_t> (start1 + i)]);

        for (int i = 0; i < size2; ++i)
            addMsg (buffer[static_cast<size_t> (start2 + i)]);

        int total = size1 + size2;
        if (total > 0)
            fifo.finishedRead (total);

        return total;
    }

private:
    juce::AbstractFifo fifo { capacity };
    std::array<Message, capacity> buffer;
};
