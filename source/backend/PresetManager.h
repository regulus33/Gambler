#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <vector>
#include <functional>

namespace PresetManagerConstants {
    constexpr int maxPresetNameLength = 100;
    const juce::String presetFileExtension = ".xml";
    const juce::String presetsFolderName = "Presets";
}

/**
 * PresetManager
 * 
 * Handles preset storage, loading, and saving for the plugin.
 * Presets are stored as XML files in a dedicated presets folder
 * within the plugin's application data directory.
 * 
 * Thread Safety:
 * - File operations should be called from the message thread
 * - The manager maintains a list of available presets that can be queried
 */
class PresetManager {
public:
    /**
     * Represents a single preset entry
     */
    struct PresetInfo {
        juce::String name;      // Display name (without extension)
        juce::File file;        // Full path to the preset file
        
        bool operator==(const PresetInfo& other) const {
            return name == other.name && file == other.file;
        }
    };
    
    PresetManager() {
        // Initialize presets directory
        presetsDirectory = getPresetsDirectory();
        
        // Create directory if it doesn't exist
        if (!presetsDirectory.exists()) {
            presetsDirectory.createDirectory();
        }
        
        // Scan for existing presets
        refreshPresetList();
    }
    
    /**
     * Get the presets directory path
     * Uses JUCE's standard application data location
     */
    static juce::File getPresetsDirectory() {
        auto appDataDir = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory);
        
        #if JUCE_MAC
            // On macOS: ~/Library/Application Support/Gambler/Presets
            return appDataDir.getChildFile("Gambler")
                            .getChildFile(PresetManagerConstants::presetsFolderName);
        #elif JUCE_WINDOWS
            // On Windows: %APPDATA%/Gambler/Presets
            return appDataDir.getChildFile("Gambler")
                            .getChildFile(PresetManagerConstants::presetsFolderName);
        #else
            // On Linux: ~/.config/Gambler/Presets
            return appDataDir.getChildFile("Gambler")
                            .getChildFile(PresetManagerConstants::presetsFolderName);
        #endif
    }
    
    /**
     * Refresh the list of available presets by scanning the presets directory
     */
    void refreshPresetList() {
        presets.clear();
        
        if (!presetsDirectory.exists()) {
            return;
        }
        
        // Find all XML files in the presets directory
        auto files = presetsDirectory.findChildFiles(
            juce::File::findFiles, 
            false, 
            "*" + PresetManagerConstants::presetFileExtension);
        
        // Sort alphabetically
        files.sort();
        
        for (const auto& file : files) {
            PresetInfo info;
            info.name = file.getFileNameWithoutExtension();
            info.file = file;
            presets.push_back(info);
        }
        
        DBG("PresetManager: Found " + juce::String(presets.size()) + " presets");
    }
    
    /**
     * Get the list of available presets
     */
    const std::vector<PresetInfo>& getPresets() const {
        return presets;
    }
    
    /**
     * Get preset names as a StringArray (useful for ComboBox)
     */
    juce::StringArray getPresetNames() const {
        juce::StringArray names;
        for (const auto& preset : presets) {
            names.add(preset.name);
        }
        return names;
    }
    
    /**
     * Check if a preset name already exists
     */
    bool presetExists(const juce::String& name) const {
        for (const auto& preset : presets) {
            if (preset.name.equalsIgnoreCase(name)) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * Save a preset with the given name
     * @param name The preset name (will be sanitized for filesystem)
     * @param stateXml The XML element containing the full plugin state
     * @return true if save was successful
     */
    bool savePreset(const juce::String& name, const juce::XmlElement& stateXml) {
        if (name.isEmpty()) {
            DBG("PresetManager: Cannot save preset with empty name");
            return false;
        }
        
        // Sanitize the name for filesystem
        juce::String sanitizedName = sanitizePresetName(name);
        
        // Create the file path
        juce::File presetFile = presetsDirectory.getChildFile(
            sanitizedName + PresetManagerConstants::presetFileExtension);
        
        // Ensure directory exists
        if (!presetsDirectory.exists()) {
            presetsDirectory.createDirectory();
        }
        
        // Write XML to file
        if (stateXml.writeTo(presetFile)) {
            DBG("PresetManager: Saved preset '" + name + "' to " + presetFile.getFullPathName());
            refreshPresetList();
            return true;
        }
        
        DBG("PresetManager: Failed to save preset '" + name + "'");
        return false;
    }
    
    /**
     * Load a preset by name
     * @param name The preset name
     * @return The XML element containing the preset state, or nullptr if not found
     */
    std::unique_ptr<juce::XmlElement> loadPreset(const juce::String& name) {
        for (const auto& preset : presets) {
            if (preset.name == name) {
                return loadPresetFromFile(preset.file);
            }
        }
        
        DBG("PresetManager: Preset '" + name + "' not found");
        return nullptr;
    }
    
    /**
     * Load a preset by index
     * @param index The index in the presets list
     * @return The XML element containing the preset state, or nullptr if invalid index
     */
    std::unique_ptr<juce::XmlElement> loadPresetByIndex(int index) {
        if (index < 0 || index >= static_cast<int>(presets.size())) {
            DBG("PresetManager: Invalid preset index " + juce::String(index));
            return nullptr;
        }
        
        return loadPresetFromFile(presets[static_cast<size_t>(index)].file);
    }
    
    /**
     * Delete a preset by name
     * @param name The preset name
     * @return true if deletion was successful
     */
    bool deletePreset(const juce::String& name) {
        for (const auto& preset : presets) {
            if (preset.name == name) {
                if (preset.file.deleteFile()) {
                    DBG("PresetManager: Deleted preset '" + name + "'");
                    refreshPresetList();
                    return true;
                }
            }
        }

        DBG("PresetManager: Failed to delete preset '" + name + "'");
        return false;
    }

    /**
     * Delete a preset by index
     * @param index The index in the presets list
     * @return true if deletion was successful
     */
    bool deletePresetByIndex(int index) {
        if (index < 0 || index >= static_cast<int>(presets.size())) {
            DBG("PresetManager: Invalid preset index " + juce::String(index));
            return false;
        }

        return deletePreset(presets[static_cast<size_t>(index)].name);
    }
    
    /**
     * Get the index of a preset by name
     * @return The index, or -1 if not found
     */
    int getPresetIndex(const juce::String& name) const {
        for (size_t i = 0; i < presets.size(); ++i) {
            if (presets[i].name == name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    /**
     * Get the currently loaded preset name
     */
    const juce::String& getCurrentPresetName() const {
        return currentPresetName;
    }
    
    /**
     * Set the currently loaded preset name
     */
    void setCurrentPresetName(const juce::String& name) {
        currentPresetName = name;
    }
    
private:
    /**
     * Sanitize a preset name for use as a filename
     * Removes/replaces characters that are invalid in filenames
     */
    static juce::String sanitizePresetName(const juce::String& name) {
        juce::String sanitized = name.trim();
        
        // Replace invalid filename characters
        sanitized = sanitized.replaceCharacters("/\\:*?\"<>|", "_________");
        
        // Limit length
        if (sanitized.length() > PresetManagerConstants::maxPresetNameLength) {
            sanitized = sanitized.substring(0, PresetManagerConstants::maxPresetNameLength);
        }
        
        return sanitized;
    }
    
    /**
     * Load preset XML from a file
     */
    std::unique_ptr<juce::XmlElement> loadPresetFromFile(const juce::File& file) {
        if (!file.existsAsFile()) {
            DBG("PresetManager: File does not exist: " + file.getFullPathName());
            return nullptr;
        }
        
        auto xml = juce::parseXML(file);
        if (xml) {
            DBG("PresetManager: Loaded preset from " + file.getFullPathName());
        } else {
            DBG("PresetManager: Failed to parse XML from " + file.getFullPathName());
        }
        
        return xml;
    }
    
    juce::File presetsDirectory;
    std::vector<PresetInfo> presets;
    juce::String currentPresetName;
};

