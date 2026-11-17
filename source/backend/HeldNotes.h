#pragma once

#include <atomic>
#include <bitset>
#include <cstdint>

/**
 * HeldNotes - the chord played on MIDI input (unmapped notes only).
 *
 * Two sets of absolute MIDI notes:
 *  - held:  keys down right now
 *  - chord: the chord in use. A press takes effect at once and replaces the chord
 *           with exactly the keys now down, so an earlier chord never leaks into the
 *           next one. A release only takes effect once no key has changed for a short
 *           grace period: the gap between one chord's note-offs and the next chord's
 *           note-ons (or a slightly uneven release) never empties the chord.
 *           When every key is up after the grace period, the chord is kept with
 *           Latch on and cleared with Latch off.
 *
 * Time is a sample count supplied by the caller. Note events and update() run on the
 * audio thread; readers on any thread see the two sets through atomic 64-bit words,
 * so at worst a reader catches a chord half-updated for one block, which is harmless.
 */
class HeldNotes {
public:
    using Set = std::bitset<128>;

    /** Press. Returns true if the chord changed. */
    bool noteOn (int note) {
        if (note < 0 || note > 127) return false;
        setBit (heldWords, note, true);
        pendingSince.store (-1, std::memory_order_relaxed);
        return setChord (held());
    }

    /** Release: the chord shrinks only after the grace period (see update). */
    void noteOff (int note, std::int64_t now) {
        if (note < 0 || note > 127) return;
        setBit (heldWords, note, false);
        pendingSince.store (now, std::memory_order_relaxed);
    }

    /**
     * Apply releases once `graceSamples` have passed with no key change. Call once per
     * block. With `keepWhenReleased` (Latch on) a chord whose keys are all up is kept.
     * Returns true if the chord changed.
     */
    bool update (std::int64_t now, std::int64_t graceSamples, bool keepWhenReleased) {
        const auto since = pendingSince.load (std::memory_order_relaxed);
        if (since >= 0 && now - since < graceSamples) return false;
        pendingSince.store (-1, std::memory_order_relaxed);
        const auto down = held();
        if (down.any()) return setChord (down);
        return keepWhenReleased ? false : setChord ({});
    }

    /** Apply pending releases now, without waiting for the grace period. */
    bool applyReleasesNow (bool keepWhenReleased) {
        pendingSince.store (-1, std::memory_order_relaxed);
        return update (0, 0, keepWhenReleased);
    }

    /** Forget every key (stuck notes after a host stop). Returns true if the chord changed. */
    bool releaseAll (bool keepChord) {
        clearWords (heldWords);
        pendingSince.store (-1, std::memory_order_relaxed);
        return keepChord ? false : setChord ({});
    }

    Set held() const { return read (heldWords); }
    Set chord() const { return read (chordWords); }

private:
    using Words = std::atomic<std::uint64_t>[2];

    bool setChord (const Set& next) {
        if (next == chord()) return false;
        for (int w = 0; w < 2; ++w) {
            std::uint64_t bits = 0;
            for (int b = 0; b < 64; ++b)
                if (next.test (static_cast<size_t> (w * 64 + b))) bits |= std::uint64_t { 1 } << b;
            chordWords[w].store (bits, std::memory_order_relaxed);
        }
        return true;
    }

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
    Words chordWords {};
    std::atomic<std::int64_t> pendingSince { -1 };
};
