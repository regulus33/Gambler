#pragma once

#include "../data/Params.h"
#include "HeldNotes.h"
#include "Scale.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <bitset>

/**
 * ScaleQuantizer - A view over APVTS for scale/range state and note quantization.
 *
 * Reads scale notes, range window, and destructive mode settings from APVTS.
 * Provides quantization (on-the-fly for playback) and baking (destructive storage).
 *
 * A note is "allowed" when it is in the scale and inside at least one note range
 * (there can be up to ParamConstants::MAX_NOTE_RANGES of them). Quantizing moves a
 * note to the nearest allowed one, preferring the lower note on a tie.
 *
 * Range follow: a chord held on MIDI input can stand in for the ranges.
 *   Span  - one range from the chord's lowest to highest note (scale still applies)
 *   Exact - only the chord's notes are allowed (the scale is ignored)
 * Stored notes are moved proportionally from the manual range onto the chord
 * rather than to the nearest chord note, so a melody keeps its shape and spreads
 * over the whole chord the way an arpeggiator would. Editing while a chord is
 * held stores the inverse, so the note lands where it was put.
 * It affects playback, editing and random notes, never baked data, and it turns
 * off Bake range so a chord can't rewrite stored notes.
 *
 * Usage:
 *   ScaleQuantizer sq(apvts);
 *   int quantized = sq.quantize(rawNote);
 *   int baked = sq.bake(rawNote);
 */
class ScaleQuantizer {
public:
    using APVTS = juce::AudioProcessorValueTreeState;

    explicit ScaleQuantizer(APVTS& apvts) : apvts(apvts) {}

    // ========================================
    // SCALE NOTE STATE
    // ========================================

    bool scaleNote(int noteIndex) const {
        if (noteIndex < 0 || noteIndex >= ParamConstants::NUM_SCALE_NOTES) return true;
        return getBool(Params::Keys::ID::globalScaleNote(noteIndex));
    }

    void setScaleNote(int noteIndex, bool enabled) {
        if (noteIndex >= 0 && noteIndex < ParamConstants::NUM_SCALE_NOTES)
            setBool(Params::Keys::ID::globalScaleNote(noteIndex), enabled);
    }

    std::array<int, 12> scaleNotes() const {
        std::array<int, 12> notes{};
        for (int i = 0; i < 12; i++)
            notes[i] = scaleNote(i) ? 1 : 0;
        return notes;
    }

    void setScaleNotes(const std::array<int, 12>& notes) {
        for (int i = 0; i < 12; i++)
            setScaleNote(i, notes[i] != 0);
    }

    bool isInScale(int noteNumber) const {
        if (noteNumber < 0 || noteNumber > 127) return false;
        return scaleNote(noteNumber % 12);
    }

    bool anyScaleNoteEnabled() const {
        for (int i = 0; i < 12; i++)
            if (scaleNote(i)) return true;
        return false;
    }

    Scale asScale() const {
        Scale scale;
        scale.setScaleNotes(scaleNotes());
        scale.setRangeMin(rangeMin());
        scale.setRangeMax(rangeMax());
        return scale;
    }

    // ========================================
    // NOTE RANGES
    // ========================================

    struct NoteRange {
        int min = 0;
        int max = 127;
    };

    int windowOffset() const {
        return getInt(Params::Keys::ID::SCALE_WINDOW_OFFSET);
    }

    void setWindowOffset(int offset) {
        setInt(Params::Keys::ID::SCALE_WINDOW_OFFSET, juce::jlimit(0, 127, offset));
    }

    int windowSize() const {
        return getInt(Params::Keys::ID::SCALE_WINDOW_SIZE);
    }

    void setWindowSize(int size) {
        setInt(Params::Keys::ID::SCALE_WINDOW_SIZE, juce::jlimit(1, 128, size));
    }

    /** Bounds of the first range (the scale window). */
    int rangeMin() const {
        return Scale::computeMinFromWindow(windowOffset(), windowSize());
    }

    int rangeMax() const {
        return Scale::computeMaxFromWindow(windowOffset(), windowSize());
    }

    void setRangeMin(int min) {
        int max = rangeMax();
        setWindowOffset(Scale::computeOffsetFromRange(min, max));
        setWindowSize(Scale::computeSizeFromRange(min, max));
    }

    void setRangeMax(int max) {
        int min = rangeMin();
        setWindowOffset(Scale::computeOffsetFromRange(min, max));
        setWindowSize(Scale::computeSizeFromRange(min, max));
    }

    int rangeCount() const {
        return juce::jlimit(1, ParamConstants::MAX_NOTE_RANGES, getInt(Params::Keys::ID::NOTE_RANGE_COUNT));
    }

    NoteRange range(int index) const {
        if (index <= 0)
            return { rangeMin(), rangeMax() };
        const int a = getInt(Params::Keys::ID::extraNoteRangeMin(index));
        const int b = getInt(Params::Keys::ID::extraNoteRangeMax(index));
        return { juce::jmin(a, b), juce::jmax(a, b) };
    }

    void setRange(int index, int minNote, int maxNote) {
        if (index < 0 || index >= ParamConstants::MAX_NOTE_RANGES) return;
        const int lo = juce::jlimit(0, 127, juce::jmin(minNote, maxNote));
        const int hi = juce::jlimit(0, 127, juce::jmax(minNote, maxNote));
        if (index == 0) {
            setWindowSize(Scale::computeSizeFromRange(lo, hi));
            setWindowOffset(Scale::computeOffsetFromRange(lo, hi));
        } else {
            setInt(Params::Keys::ID::extraNoteRangeMin(index), lo);
            setInt(Params::Keys::ID::extraNoteRangeMax(index), hi);
        }
    }

    /** Add a range one octave wide, above the highest existing one if there is room. */
    bool addRange() {
        const int count = rangeCount();
        if (count >= ParamConstants::MAX_NOTE_RANGES) return false;

        int top = 0, bottom = 127;
        for (int i = 0; i < count; ++i) {
            top = juce::jmax(top, range(i).max);
            bottom = juce::jmin(bottom, range(i).min);
        }

        if (top + 13 <= 127)        setRange(count, top + 2, top + 13);
        else if (bottom - 13 >= 0)  setRange(count, bottom - 13, bottom - 2);
        else                        setRange(count, 60, 71);

        setInt(Params::Keys::ID::NOTE_RANGE_COUNT, count + 1);
        return true;
    }

    /** Remove a range; later ranges move down one slot. The last range cannot be removed. */
    bool removeRange(int index) {
        const int count = rangeCount();
        if (count <= 1 || index < 0 || index >= count) return false;
        for (int i = index; i < count - 1; ++i) {
            const auto next = range(i + 1);
            setRange(i, next.min, next.max);
        }
        setInt(Params::Keys::ID::NOTE_RANGE_COUNT, count - 1);
        return true;
    }

    bool isInAnyRange(int noteNumber) const {
        const int count = rangeCount();
        for (int i = 0; i < count; ++i) {
            const auto r = range(i);
            if (noteNumber >= r.min && noteNumber <= r.max) return true;
        }
        return false;
    }

    // ========================================
    // RANGE FOLLOWS CHORD
    // ========================================

    enum class Follow { Off = 0, Span = 1, Exact = 2 };

    Follow followMode() const {
        return static_cast<Follow>(juce::jlimit(0, 2, getInt(Params::Keys::ID::RANGE_FOLLOW)));
    }

    HeldNotes& heldNotes() { return held; }
    const HeldNotes& heldNotes() const { return held; }

    /**
     * The chord currently standing in for the ranges, or an empty set when it should not.
     * With Latch off this is the keys down right now; with Latch on, the last chord played.
     */
    HeldNotes::Set followChord() const {
        if (followMode() == Follow::Off) return {};
        return getBool(Params::Keys::ID::SCALE_LATCH) ? held.latched() : held.held();
    }

    // ========================================
    // DESTRUCTIVE MODE
    // ========================================

    bool destructiveScale() const {
        return getBool(Params::Keys::ID::PREF_DESTRUCTIVE_SCALE);
    }

    void setDestructiveScale(bool enabled) {
        setBool(Params::Keys::ID::PREF_DESTRUCTIVE_SCALE, enabled);
    }

    bool destructiveRange() const {
        return getBool(Params::Keys::ID::PREF_DESTRUCTIVE_RANGE);
    }

    void setDestructiveRange(bool enabled) {
        setBool(Params::Keys::ID::PREF_DESTRUCTIVE_RANGE, enabled);
    }

    // ========================================
    // QUANTIZATION
    // ========================================

    /**
     * The set of allowed MIDI notes, captured once so a batch of notes can be
     * quantized without re-reading every parameter per note.
     */
    struct Filter {
        std::bitset<128> allowed;

        // Set when a held chord stands in for the ranges: notes are first projected
        // from the manual range [fromLo, fromHi] onto the chord.
        bool remap = false;
        int fromLo = 0, fromHi = 127;
        int toLo = 0, toHi = 127;                // Span: the chord's lowest and highest note
        std::array<int, 128> chordNotes{};       // Exact: the chord, low to high
        int chordCount = 0;

        /** Where a stored note lands on the chord, before snapping to an allowed note. */
        int project(int noteNumber) const {
            const double t = position(noteNumber, fromLo, fromHi);
            if (chordCount > 0)
                return chordNotes[static_cast<size_t>(juce::roundToInt(t * (chordCount - 1)))];
            return toLo + juce::roundToInt(t * (toHi - toLo));
        }

        /** The stored note that plays as `shownNote` (the inverse of project). */
        int unproject(int shownNote) const {
            if (!remap) return shownNote;
            double t = 0.5;
            if (chordCount > 0) {
                int best = 0;
                for (int i = 1; i < chordCount; ++i)
                    if (std::abs(chordNotes[static_cast<size_t>(i)] - shownNote)
                        < std::abs(chordNotes[static_cast<size_t>(best)] - shownNote)) best = i;
                if (chordCount > 1) t = best / static_cast<double>(chordCount - 1);
            } else {
                t = position(shownNote, toLo, toHi);
            }
            return fromLo + juce::roundToInt(t * (fromHi - fromLo));
        }

        static double position(int note, int lo, int hi) {
            if (hi <= lo) return 0.5;
            return juce::jlimit(0.0, 1.0, (note - lo) / static_cast<double>(hi - lo));
        }

        /** Nearest allowed note (lower one wins a tie), or -1 if nothing is allowed. */
        int nearest(int noteNumber) const {
            noteNumber = juce::jlimit(0, 127, noteNumber);
            if (allowed.test(static_cast<size_t>(noteNumber))) return noteNumber;
            for (int d = 1; d <= 127; d++) {
                const int down = noteNumber - d;
                if (down >= 0 && allowed.test(static_cast<size_t>(down))) return down;
                const int up = noteNumber + d;
                if (up <= 127 && allowed.test(static_cast<size_t>(up))) return up;
            }
            return -1;
        }

        int quantize(int noteNumber) const {
            if (noteNumber < 0) return noteNumber;
            return nearest(remap ? project(noteNumber) : noteNumber);
        }
    };

    Filter makeFilter(bool applyScale, bool applyRange, bool useChord = true) const {
        Filter f;

        std::array<NoteRange, ParamConstants::MAX_NOTE_RANGES> ranges{};
        int count = rangeCount();
        for (int i = 0; i < count; ++i) ranges[static_cast<size_t>(i)] = range(i);

        const auto chord = useChord ? followChord() : HeldNotes::Set{};
        if (chord.any()) {
            f.remap = true;
            f.fromLo = 127;
            f.fromHi = 0;
            for (int i = 0; i < count; ++i) {
                f.fromLo = juce::jmin(f.fromLo, ranges[static_cast<size_t>(i)].min);
                f.fromHi = juce::jmax(f.fromHi, ranges[static_cast<size_t>(i)].max);
            }
            for (int n = 0; n <= 127; ++n)
                if (chord.test(static_cast<size_t>(n))) f.chordNotes[static_cast<size_t>(f.chordCount++)] = n;
            f.toLo = f.chordNotes[0];
            f.toHi = f.chordNotes[static_cast<size_t>(f.chordCount - 1)];

            if (followMode() == Follow::Exact) {
                f.allowed = chord;
                return f;
            }

            // Span: one range from the lowest to the highest chord note
            f.chordCount = 0;
            ranges[0] = { f.toLo, f.toHi };
            count = 1;
            applyRange = true;
        }

        std::array<bool, 12> scale{};
        for (int i = 0; i < 12; i++) scale[static_cast<size_t>(i)] = scaleNote(i);

        for (int n = 0; n <= 127; ++n) {
            bool ok = !applyScale || scale[static_cast<size_t>(n % 12)];
            if (ok && applyRange) {
                ok = false;
                for (int i = 0; i < count && !ok; ++i)
                    ok = n >= ranges[static_cast<size_t>(i)].min && n <= ranges[static_cast<size_t>(i)].max;
            }
            f.allowed.set(static_cast<size_t>(n), ok);
        }
        return f;
    }

    /** What stored notes sound as. Constraints already baked into the data are skipped. */
    Filter playbackFilter() const { return makeFilter(!destructiveScale(), !rangeIsBaked()); }

    /** Bake range only takes effect while range follow is off. */
    bool rangeIsBaked() const { return destructiveRange() && followMode() == Follow::Off; }

    /** Scale and ranges both applied, regardless of destructive mode (for editing gestures). */
    Filter editFilter() const { return makeFilter(true, true); }

    /**
     * Every note inside any manual range, ignoring the scale (for random generation).
     * Random notes are made in the manual range; playback projects them onto a held chord.
     */
    Filter rangeFilter() const { return makeFilter(false, true, false); }

    /**
     * Quantize a note to the global scale and note ranges.
     * Skips scale/range that was already baked (destructive mode).
     * Returns -1 if no note is allowed.
     */
    int quantize(int noteNumber) const {
        return playbackFilter().quantize(noteNumber);
    }

    /**
     * Bake destructive quantization into a note before storage.
     * Returns the note unchanged if destructive mode is off.
     */
    int bake(int noteNumber) const {
        if (noteNumber < 0) return noteNumber;

        const bool bakeR = rangeIsBaked();
        const bool bakeS = destructiveScale();
        if (!bakeR && !bakeS) return noteNumber;

        return makeFilter(bakeS, bakeR, false).nearest(noteNumber);
    }

private:
    APVTS& apvts;
    HeldNotes held;

    int getInt(const juce::String& id) const {
        if (auto* p = apvts.getRawParameterValue(id))
            return static_cast<int>(p->load());
        return 0;
    }

    void setInt(const juce::String& id, int value) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(value)));
    }

    bool getBool(const juce::String& id) const {
        if (auto* p = apvts.getRawParameterValue(id))
            return p->load() >= 0.5f;
        return false;
    }

    void setBool(const juce::String& id, bool value) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(value ? 1.0f : 0.0f);
    }
};
