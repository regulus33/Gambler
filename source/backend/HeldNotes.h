#pragma once

#include <atomic>
#include <bitset>
#include <cstdint>

/**
 * HeldNotes - which incoming MIDI notes are down, for "range follows chord".
 *
 * Two sets of absolute MIDI notes:
 *  - held:    keys down right now
 *  - latched: the last chord. A press after every key was released starts a new
 *             chord; presses while keys are down add to it; releases never shrink it.
 *
 * Written on the audio thread (note on/off), read on both threads. Each set is
 * two atomic 64-bit words, so a reader can see a chord half-updated for one block
 * at worst, which is harmless here.
 */
class HeldNotes {
public:
    using Set = std::bitset<128>;

    void noteOn (int note) {
        if (note < 0 || note > 127) return;
        const bool startsNewChord = held().none();
        setBit (heldWords, note, true);
        if (startsNewChord) clearWords (latchedWords);
        setBit (latchedWords, note, true);
    }

    void noteOff (int note) {
        if (note < 0 || note > 127) return;
        setBit (heldWords, note, false);
    }

    /** Forget keys that are down (stuck notes after a host stop); keep the latched chord. */
    void releaseAll() { clearWords (heldWords); }

    Set held() const { return read (heldWords); }
    Set latched() const { return read (latchedWords); }

private:
    using Words = std::atomic<std::uint64_t>[2];

    static void setBit (Words& words, int note, bool on) {
        const auto mask = std::uint64_t { 1 } << (note % 64);
        if (on) words[note / 64].fetch_or (mask, std::memory_order_relaxed);
        else    words[note / 64].fetch_and (~mask, std::memory_order_relaxed);
    }

    static void clearWords (Words& words) {
        words[0].store (0, std::memory_order_relaxed);
        words[1].store (0, std::memory_order_relaxed);
    }

    static Set read (const Words& words) {
        Set set;
        for (int w = 0; w < 2; ++w) {
            const auto bits = words[w].load (std::memory_order_relaxed);
            for (int b = 0; b < 64; ++b)
                if ((bits >> b) & 1u) set.set (static_cast<size_t> (w * 64 + b));
        }
        return set;
    }

    Words heldWords {};
    Words latchedWords {};
};
