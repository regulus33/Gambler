#pragma once

#include "../data/Params.h"
#include "Scale.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

/**
 * ScaleQuantizer - A view over APVTS for scale/range state and note quantization.
 *
 * Reads scale notes, range window, and destructive mode settings from APVTS.
 * Provides quantization (on-the-fly for playback) and baking (destructive storage).
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
    // RANGE WINDOW
    // ========================================

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
     * Quantize a note to the global scale and clamp to range.
     * Skips scale/range that was already baked (destructive mode).
     * Returns -1 if no scale notes are enabled.
     */
    int quantize(int noteNumber) const {
        bool skipRange = destructiveRange();
        bool skipScale = destructiveScale();

        int rMin = rangeMin();
        int rMax = rangeMax();

        if (!skipRange)
            noteNumber = juce::jlimit(rMin, rMax, noteNumber);

        if (skipScale)
            return noteNumber;

        if (!anyScaleNoteEnabled()) return -1;
        if (isInScale(noteNumber)) return noteNumber;

        int searchMin = skipRange ? 0 : rMin;
        int searchMax = skipRange ? 127 : rMax;

        for (int d = 1; d <= 127; d++) {
            int down = noteNumber - d;
            if (down >= searchMin && isInScale(down)) return down;
            int up = noteNumber + d;
            if (up <= searchMax && isInScale(up)) return up;
            if (down < searchMin && up > searchMax) break;
        }

        return -1;
    }

    /**
     * Bake destructive quantization into a note before storage.
     * Returns the note unchanged if destructive mode is off.
     */
    int bake(int noteNumber) const {
        if (noteNumber < 0) return noteNumber;

        bool bakeR = destructiveRange();
        bool bakeS = destructiveScale();

        if (!bakeR && !bakeS) return noteNumber;

        if (bakeR)
            noteNumber = juce::jlimit(rangeMin(), rangeMax(), noteNumber);

        if (bakeS) {
            if (!anyScaleNoteEnabled()) return -1;

            if (!isInScale(noteNumber)) {
                int sMin = bakeR ? rangeMin() : 0;
                int sMax = bakeR ? rangeMax() : 127;
                for (int d = 1; d <= 127; d++) {
                    int down = noteNumber - d;
                    if (down >= sMin && isInScale(down)) return down;
                    int up = noteNumber + d;
                    if (up <= sMax && isInScale(up)) return up;
                    if (down < sMin && up > sMax) break;
                }
                return -1;
            }
        }

        return noteNumber;
    }

private:
    APVTS& apvts;

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
