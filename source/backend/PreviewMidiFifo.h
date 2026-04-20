#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

/**
 This is a really cool way we can send midi from UI thread to Audio thread.
    So first of all, if you want to just send a little piece of data in Juce between threads
    you can just have a shared atomic variable (lock free) and have a 30hz timer that basically polls
    it for changes by the reader... Sounds like a bad idea but this is to illustrate a point.
    What if you know that var (let's say its just a midi note number to an `int`) will change very often
    and what if that var is supposed to be processed in bursts (polyphonic midi).
    Now you have a problem, if you write to the variable faster than 30hz, you'll drop values.
    That's why we need a buffer. A buffer gives your write and reader slack to catch up and dramatically
    reduces the likelihood of dropped packets.
    The important thing about a fifo buffer isn't the fifo part, that just means the order in which you send
    the data means that its processed logically. The first note I send is the first that's read.
    The important thing is that we have a read and write head at different spots and that's what creates
    the buffer "zone"
       R       W
    [0,1,2,3,5,6]
    If we read from R's position we read "1" and W just wrote 6.
    Our indexes will always loop back around, but this ordering (as long as R starts out at a smaller index
    than W) ensures that this follows fifo. The distance between the 2 heads/pointers/indexes.
    This distance is known as Buffer Occupancy or Fill Level.
    https://claude.ai/chat/d77b186c-9ed0-42ab-8326-0156358269df
   https://claude.ai/chat/1195b503-6704-4fad-b665-e3f622c17628
 **/
// We only care about note-on|off and note number. We're not dealing with real midi here, its
// synthetic notes sent to the backend when the user interacts with a component that generates it
// as a preview.
class PreviewMidiFifo {
public:
    struct Message {
        int noteNumber = 0;
        bool isNoteOn = false;
        juce::uint8 velocity = 0;
    };

    static constexpr int capacity = 32;

    /** Push a single message (call from UI thread). */
    void push (Message msg)
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
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
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
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
            addMsg (buffer[static_cast<size_t> (start1) + static_cast<size_t> (i)]);

        for (int i = 0; i < size2; ++i)
            addMsg (buffer[static_cast<size_t> (start2) + static_cast<size_t> (i)]);

        int total = size1 + size2;
        if (total > 0)
            fifo.finishedRead (total);

        return total;
    }

private:
    juce::AbstractFifo fifo { capacity };
    std::array<Message, capacity> buffer;
};
