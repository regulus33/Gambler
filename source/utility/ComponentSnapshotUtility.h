#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class ComponentSnapshotUtility
{
public:
    /**
     * Saves a PNG snapshot of a component to the specified directory.
     * Only works if TAKE_SNAPSHOTS_OF_COMPONENTS is defined and set to 1.
     *
     * @param component The component to capture
     * @param outputDirectory The directory where the PNG will be saved
     * @param filename The name of the PNG file (without extension)
     * @param scale Scale factor for the snapshot (1.0 = actual size, 2.0 = retina, etc.)
     * @return true if the snapshot was saved successfully
     */
    static bool saveSnapshot(juce::Component& component,
                            const juce::String& outputDirectory,
                            const juce::String& filename,
                            float scale = 1.0f)
    {
#if defined(TAKE_SNAPSHOTS_OF_COMPONENTS) && TAKE_SNAPSHOTS_OF_COMPONENTS == 1
        // Create the snapshot
        juce::Image snapshot = component.createComponentSnapshot(
            component.getLocalBounds(),
            true,
            scale
        );

        // Construct the output path
        juce::File outputFile = juce::File(outputDirectory)
            .getChildFile(filename + ".png");

        // Ensure the directory exists
        outputFile.getParentDirectory().createDirectory();

        // Write the PNG
        juce::FileOutputStream outputStream(outputFile);

        if (outputStream.openedOk())
        {
            juce::PNGImageFormat pngFormat;
            bool success = pngFormat.writeImageToStream(snapshot, outputStream);

            if (success)
            {
                DBG("Snapshot saved to: " + outputFile.getFullPathName());
            }
            else
            {
                DBG("Failed to write snapshot to: " + outputFile.getFullPathName());
            }

            return success;
        }

        DBG("Failed to open output stream for: " + outputFile.getFullPathName());
        return false;
#else
        juce::ignoreUnused(component, outputDirectory, filename, scale);
        return false;
#endif
    }

    /**
     * Saves a PNG snapshot using the compile-time SNAPSHOT_DESTINATION path.
     * Only works if both TAKE_SNAPSHOTS_OF_COMPONENTS and SNAPSHOT_DESTINATION are defined.
     */
    static bool saveSnapshotToDestination(juce::Component& component,
                                         const juce::String& filename,
                                         float scale = 1.0f)
    {
#if defined(TAKE_SNAPSHOTS_OF_COMPONENTS) && TAKE_SNAPSHOTS_OF_COMPONENTS == 1 && defined(SNAPSHOT_DESTINATION)
        return saveSnapshot(component, SNAPSHOT_DESTINATION, filename, scale);
#else
        juce::ignoreUnused(component, filename, scale);
        return false;
#endif
    }

    /**
     * Saves a snapshot with an auto-generated timestamped filename.
     * Useful for capturing multiple snapshots without overwriting.
     */
    static bool saveSnapshotWithTimestamp(juce::Component& component,
                                         const juce::String& outputDirectory,
                                         const juce::String& prefix = "snapshot",
                                         float scale = 1.0f)
    {
#if defined(TAKE_SNAPSHOTS_OF_COMPONENTS) && TAKE_SNAPSHOTS_OF_COMPONENTS == 1
        juce::Time now = juce::Time::getCurrentTime();
        juce::String timestamp = now.formatted("%Y%m%d_%H%M%S");
        juce::String filename = prefix + "_" + timestamp;

        return saveSnapshot(component, outputDirectory, filename, scale);
#else
        juce::ignoreUnused(component, outputDirectory, prefix, scale);
        return false;
#endif
    }

    /**
     * Quick snapshot saver - uses component class name as filename.
     * Perfect for one-off debugging snapshots.
     */
    static bool saveQuickSnapshot(juce::Component& component,
                                 const juce::String& outputDirectory,
                                 float scale = 1.0f)
    {
#if defined(TAKE_SNAPSHOTS_OF_COMPONENTS) && TAKE_SNAPSHOTS_OF_COMPONENTS == 1
        juce::String componentName = juce::String(typeid(component).name());
        // Clean up the typename for use as a filename
        componentName = componentName.replaceCharacter(' ', '_')
                                    .replaceCharacter(':', '_');

        return saveSnapshot(component, outputDirectory, componentName, scale);
#else
        juce::ignoreUnused(component, outputDirectory, scale);
        return false;
#endif
    }

    /**
     * Quick snapshot using the compile-time SNAPSHOT_DESTINATION.
     */
    static bool saveQuickSnapshotToDestination(juce::Component& component,
                                              float scale = 1.0f)
    {
#if defined(TAKE_SNAPSHOTS_OF_COMPONENTS) && TAKE_SNAPSHOTS_OF_COMPONENTS == 1 && defined(SNAPSHOT_DESTINATION)
        juce::String componentName = juce::String(typeid(component).name());
        componentName = componentName.replaceCharacter(' ', '_')
                                    .replaceCharacter(':', '_');

        return saveSnapshot(component, SNAPSHOT_DESTINATION, componentName, scale);
#else
        juce::ignoreUnused(component, scale);
        return false;
#endif
    }
};