#pragma once

#include <array>
#include <string>
#include <vector>
#include <juce_core/juce_core.h>

/**
 * Scale - Handles note quantization and range limiting
 * 
 * Manages which notes are in the current scale and provides
 * quantization for random note generation. Also enforces
 * min/max range constraints.
 */
class Scale {
public:
    static constexpr int NUM_CHROMATIC_NOTES = 12;
    static constexpr int MIN_MIDI_NOTE = 0;
    static constexpr int MAX_MIDI_NOTE = 127;
    
    // Note names in chromatic order
    static const std::array<std::string, NUM_CHROMATIC_NOTES> NOTE_NAMES;
    
    Scale() {
        // Default: all notes enabled (chromatic scale)
        scaleNotes.fill(1);
    }
    
    // Getters
    int getRangeMin() const { return rangeMin; }
    int getRangeMax() const { return rangeMax; }

    // Window-based helpers (static utility functions)
    //
    // The APVTS params SCALE_WINDOW_OFFSET (0-127) and SCALE_WINDOW_SIZE (1-128) are
    // user-facing knob values, NOT direct MIDI note indices.
    //
    // SCALE_WINDOW_SIZE maps linearly to the number of MIDI notes in the window (1-128).
    //
    // SCALE_WINDOW_OFFSET is a percentage-style value: it maps 0-127 across the
    // available headroom [0, 128 - size], so the window can never exceed note 127
    // regardless of the offset knob position. This means the two knobs are fully
    // independent — turning the offset knob never shrinks the window.
    //
    // Actual MIDI min = round(offset / 127.0 * (128 - size))
    // Actual MIDI max = min + size - 1

    static int computeActualOffset(int rawOffset, int size) {
        int headroom = 128 - size;          // how many start positions are valid (0..headroom)
        if (headroom <= 0) return 0;        // size == 128: only one valid start position
        double t = static_cast<double>(rawOffset) / 127.0;
        return static_cast<int>(std::round(t * static_cast<double>(headroom)));
    }

    static int computeMinFromWindow(int rawOffset, int size) {
        return juce::jlimit(MIN_MIDI_NOTE, MAX_MIDI_NOTE, computeActualOffset(rawOffset, size));
    }

    static int computeMaxFromWindow(int rawOffset, int size) {
        int minNote = computeMinFromWindow(rawOffset, size);
        return juce::jlimit(MIN_MIDI_NOTE, MAX_MIDI_NOTE, minNote + size - 1);
    }

    // Inverse helpers: given a real MIDI min/max, compute the raw param values.
    // Used when setting range programmatically (e.g. from MIDI CC).
    static int computeOffsetFromRange(int minNote, int maxNote) {
        int size = computeSizeFromRange(minNote, maxNote);
        int headroom = 128 - size;
        if (headroom <= 0) return 0;
        double t = static_cast<double>(juce::jlimit(MIN_MIDI_NOTE, MAX_MIDI_NOTE, minNote))
                   / static_cast<double>(headroom);
        return juce::jlimit(0, 127, static_cast<int>(std::round(t * 127.0)));
    }

    static int computeSizeFromRange(int minNote, int maxNote) {
        int size = maxNote - minNote + 1;  // +1 because range is inclusive
        return juce::jlimit(1, MAX_MIDI_NOTE + 1, size);
    }
    
    const std::array<int, NUM_CHROMATIC_NOTES>& getScaleNotes() const {
        return scaleNotes;
    }
    
    bool isNoteInScale(int noteNumber) const {
        if (noteNumber < MIN_MIDI_NOTE || noteNumber > MAX_MIDI_NOTE) {
            return false;
        }
        
        int pitchClass = noteNumber % NUM_CHROMATIC_NOTES;
        return scaleNotes[pitchClass] == 1;
    }
    
    bool isNoteInRange(int noteNumber) const {
        return noteNumber >= rangeMin && noteNumber <= rangeMax;
    }
    
    // Setters
    void setRangeMin(int min) {
        if (min >= MIN_MIDI_NOTE && min <= MAX_MIDI_NOTE && min <= rangeMax) {
            rangeMin = min;
        }
    }
    
    void setRangeMax(int max) {
        if (max >= MIN_MIDI_NOTE && max <= MAX_MIDI_NOTE && max >= rangeMin) {
            rangeMax = max;
        }
    }
    
    void setRange(int min, int max) {
        if (min >= MIN_MIDI_NOTE && max <= MAX_MIDI_NOTE && min <= max) {
            rangeMin = min;
            rangeMax = max;
        }
    }
    
    /**
     * Set which notes are in the scale
     * @param notes Array of 12 ints (0 or 1) representing C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B
     */
    void setScaleNotes(const std::array<int, NUM_CHROMATIC_NOTES>& notes) {
        for (int i = 0; i < NUM_CHROMATIC_NOTES; i++) {
            scaleNotes[i] = (notes[i] != 0) ? 1 : 0;
        }
    }
    
    /**
     * Set individual scale note
     * @param index 0-11 (C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B)
     * @param enabled 0 or 1
     */
    void setScaleNote(int index, int enabled) {
        if (index >= 0 && index < NUM_CHROMATIC_NOTES) {
            scaleNotes[index] = (enabled != 0) ? 1 : 0;
        }
    }
    
    /**
     * Set scale from note names
     * @param noteNames Vector of note names like {"C", "D", "E", "F", "G", "A", "B"}
     */
    void setScaleFromNames(const std::vector<std::string>& noteNames) {
        // Reset all to 0
        scaleNotes.fill(0);
        
        // Enable specified notes
        for (const auto& name : noteNames) {
            for (int i = 0; i < NUM_CHROMATIC_NOTES; i++) {
                if (NOTE_NAMES[i] == name) {
                    scaleNotes[i] = 1;
                    break;
                }
            }
        }
    }
    
    /**
     * Quantize a MIDI note to the nearest note in the scale
     * If no notes are in the scale, returns -1 (rest)
     *
     * @param noteNumber MIDI note to quantize (0-127)
     * @return Quantized MIDI note, or -1 if no scale notes are enabled
     */
    int quantize(int noteNumber) const {
        // Clamp to valid MIDI range first
        if (noteNumber < MIN_MIDI_NOTE) noteNumber = MIN_MIDI_NOTE;
        if (noteNumber > MAX_MIDI_NOTE) noteNumber = MAX_MIDI_NOTE;

        // Check if any notes are enabled in scale
        bool anyEnabled = false;
        for (int i = 0; i < NUM_CHROMATIC_NOTES; i++) {
            if (scaleNotes[i] == 1) {
                anyEnabled = true;
                break;
            }
        }

        // If no scale is set, return -1 (rest) to indicate "no note should play"
        if (!anyEnabled) {
            return -1;
        }

        // If note is already in scale, return it
        if (isNoteInScale(noteNumber)) {
            return noteNumber;
        }

        // Find nearest note in scale
        // When equidistant, prefer going down (lower note)
        int searchRadius = NUM_CHROMATIC_NOTES;
        for (int distance = 1; distance <= searchRadius; distance++) {
            // Check down first (prefer lower note when equidistant)
            int downNote = noteNumber - distance;
            if (downNote >= MIN_MIDI_NOTE && isNoteInScale(downNote)) {
                return downNote;
            }

            // Then check up
            int upNote = noteNumber + distance;
            if (upNote <= MAX_MIDI_NOTE && isNoteInScale(upNote)) {
                return upNote;
            }
        }

        // Fallback (shouldn't reach here if anyEnabled is true)
        return noteNumber;
    }
    
    /**
     * Serialize to XML
     */
    std::unique_ptr<juce::XmlElement> toXml() const {
        auto xml = std::make_unique<juce::XmlElement>("Scale");
        xml->setAttribute("rangeMin", rangeMin);
        xml->setAttribute("rangeMax", rangeMax);

        // Serialize scale notes as comma-separated string
        juce::String scaleNotesStr;
        for (int i = 0; i < NUM_CHROMATIC_NOTES; i++) {
            if (i > 0) scaleNotesStr += ",";
            scaleNotesStr += juce::String(scaleNotes[i]);
        }
        xml->setAttribute("scaleNotes", scaleNotesStr);

        return xml;
    }

    /**
     * Deserialize from XML
     */
    void fromXml(const juce::XmlElement& xml) {
        if (xml.getTagName() != "Scale") return;

        rangeMin = xml.getIntAttribute("rangeMin", MIN_MIDI_NOTE);
        rangeMax = xml.getIntAttribute("rangeMax", MAX_MIDI_NOTE);

        // Parse scale notes from comma-separated string
        juce::String scaleNotesStr = xml.getStringAttribute("scaleNotes");
        juce::StringArray tokens = juce::StringArray::fromTokens(scaleNotesStr, ",", "");

        for (int i = 0; i < NUM_CHROMATIC_NOTES && i < tokens.size(); i++) {
            scaleNotes[i] = tokens[i].getIntValue();
        }
    }

private:
    std::array<int, NUM_CHROMATIC_NOTES> scaleNotes;
    int rangeMin = MIN_MIDI_NOTE;
    int rangeMax = MAX_MIDI_NOTE;
};

// Static member definition
inline const std::array<std::string, Scale::NUM_CHROMATIC_NOTES> Scale::NOTE_NAMES = {
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
};

