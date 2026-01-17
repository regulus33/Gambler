#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

namespace MusicConstants {

// --- Sizes ---
inline constexpr int NUM_CHROMATIC_NOTES = 12;
inline constexpr int NUM_OCTAVES = 10;

// --- Chromatic note names (C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B) ---
inline const std::array<juce::String, NUM_CHROMATIC_NOTES> CHROMATIC_NOTE_NAMES = {
    juce::String ("C"),
    juce::String ("Db"),
    juce::String ("D"),
    juce::String ("Eb"),
    juce::String ("E"),
    juce::String ("F"),
    juce::String ("Gb"),
    juce::String ("G"),
    juce::String ("Ab"),
    juce::String ("A"),
    juce::String ("Bb"),
    juce::String ("B")
};

// --- Per-note colours for the chromatic scale (matching the design) ---
inline const std::array<juce::Colour, NUM_CHROMATIC_NOTES> NOTE_COLOURS = {
    juce::Colour (0xFFFF6B6B), // C   - Red
    juce::Colour (0xFFFF9F6B), // Db  - Orange
    juce::Colour (0xFFFFD66B), // D   - Yellow
    juce::Colour (0xFFFFE66B), // Eb  - Light yellow
    juce::Colour (0xFFE6FF6B), // E   - Yellow-green
    juce::Colour (0xFF6BFFB4), // F   - Light green
    juce::Colour (0xFF6BFFD6), // Gb  - Cyan-green
    juce::Colour (0xFF6BD6FF), // G   - Cyan
    juce::Colour (0xFF6BB4FF), // Ab  - Light blue
    juce::Colour (0xFF9F6BFF), // A   - Purple
    juce::Colour (0xFFD66BFF), // Bb  - Magenta
    juce::Colour (0xFFFF6BD6)  // B   - Pink
};

// --- Octave labels and colours used by MainControls ---
inline const std::array<juce::String, NUM_OCTAVES> OCTAVE_LABELS = {
    juce::String ("1"),
    juce::String ("2"),
    juce::String ("3"),
    juce::String ("4"),
    juce::String ("5"),
    juce::String ("6"),
    juce::String ("7"),
    juce::String ("8"),
    juce::String ("9"),
    juce::String ("10")
};

// For now all octaves share the same accent colour (matching existing UI)
inline const std::array<juce::Colour, NUM_OCTAVES> OCTAVE_COLOURS = {
    juce::Colour (0xFFFF6B6B),
    juce::Colour (0xFFFF6B6B),
    juce::Colour (0xFFFF6B6B),
    juce::Colour (0xFFFF6B6B),
    juce::Colour (0xFFFF6B6B),
    juce::Colour (0xFFFF6B6B),
    juce::Colour (0xFFFF6B6B),
    juce::Colour (0xFFFF6B6B),
    juce::Colour (0xFFFF6B6B),
    juce::Colour (0xFFFF6B6B)
};

// --- Helper to convert std::array -> juce::Array ---

template <typename T, std::size_t N>
inline juce::Array<T> toJuceArray (const std::array<T, N>& src) {
    juce::Array<T> dst;
    dst.addArray (src.data(), (int) N);
    return dst;
}

// --- Helper functions ---

/**
 * Get the color for a chromatic note (0-11)
 */
inline juce::Colour getNoteColor(int chromaticNote) {
    return NOTE_COLOURS[chromaticNote % NUM_CHROMATIC_NOTES];
}

/**
 * Convert MIDI note number to note name (e.g., 60 -> "C4", 67 -> "G4")
 */
inline juce::String midiNoteToNoteName(int midiNote) {
    int chromaticNote = midiNote % NUM_CHROMATIC_NOTES;
    int octave = (midiNote / 12) - 1; // MIDI note 60 = C4
    return CHROMATIC_NOTE_NAMES[chromaticNote] + juce::String(octave);
}

} // namespace MusicConstants

