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

    /**
     * Recursively snapshots all components in a hierarchy.
     * Saves each component with a numbered prefix and type name.
     * Shows a message box with the result.
     *
     * @param rootComponent The root component to start from
     * @param outputDirectory The directory where snapshots will be saved
     * @param scale Scale factor for snapshots (default 2.0 for retina)
     * @return Number of snapshots saved
     */
    static int snapshotAllComponentsRecursively(juce::Component* rootComponent,
                                               const juce::String& outputDirectory,
                                               float scale = 2.0f)
    {
#if defined(TAKE_SNAPSHOTS_OF_COMPONENTS) && TAKE_SNAPSHOTS_OF_COMPONENTS == 1
        if (!rootComponent)
        {
            return 0;
        }

        // Create the output directory
        juce::File outputDir(outputDirectory);
        outputDir.createDirectory();

        // Start recursive snapshot
        int count = 0;
        snapshotComponentRecursive(rootComponent, outputDirectory, count, scale);

        // Show result message
        DBG("Saved " + juce::String(count) + " component snapshots to: " + outputDirectory);

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Snapshots Saved",
            "Saved " + juce::String(count) + " component snapshots to:\n" + outputDirectory,
            "OK");

        return count;
#else
        juce::ignoreUnused(rootComponent, outputDirectory, scale);
        return 0;
#endif
    }

    /**
     * Recursively snapshots all components using the compile-time SNAPSHOT_DESTINATION.
     * Shows a message box with the result.
     *
     * @param rootComponent The root component to start from
     * @param scale Scale factor for snapshots (default 2.0 for retina)
     * @return Number of snapshots saved
     */
    static int snapshotAllComponentsToDestination(juce::Component* rootComponent,
                                                 float scale = 2.0f)
    {
#if defined(TAKE_SNAPSHOTS_OF_COMPONENTS) && TAKE_SNAPSHOTS_OF_COMPONENTS == 1 && defined(SNAPSHOT_DESTINATION)
        return snapshotAllComponentsRecursively(rootComponent, SNAPSHOT_DESTINATION, scale);
#else
        juce::ignoreUnused(rootComponent, scale);
        DBG("Component snapshots disabled - set TAKE_SNAPSHOTS_OF_COMPONENTS=1 and define SNAPSHOT_DESTINATION");
        return 0;
#endif
    }

private:
    /**
     * Internal recursive helper for snapshotting component hierarchies.
     */
    static void snapshotComponentRecursive(juce::Component* component,
                                          const juce::String& basePath,
                                          int& count,
                                          float scale)
    {
#if defined(TAKE_SNAPSHOTS_OF_COMPONENTS) && TAKE_SNAPSHOTS_OF_COMPONENTS == 1
        if (!component || component->getWidth() <= 0 || component->getHeight() <= 0)
        {
            return;
        }

        // Get component type name
        juce::String componentType = juce::String(typeid(*component).name());

        // Clean up the typename
        componentType = componentType.fromLastOccurrenceOf("::", false, false);  // Remove namespace
        componentType = componentType.replaceCharacter(' ', '_');
        componentType = componentType.replaceCharacter('<', '_');
        componentType = componentType.replaceCharacter('>', '_');

        // Add width and height
        auto width = component->getWidth();
        auto height = component->getHeight();
        juce::String widthHeightString = "__" + juce::String(width) + "X" + juce::String(height);

        // Create filename with count for uniqueness
        juce::String filename = juce::String(count).paddedLeft('0', 3) + "_" + componentType + widthHeightString;

        // Take the snapshot
        saveSnapshot(*component, basePath, filename, scale);

        count++;

        // Recursively snapshot all children
        for (int i = 0; i < component->getNumChildComponents(); ++i)
        {
            snapshotComponentRecursive(component->getChildComponent(i), basePath, count, scale);
        }
#else
        juce::ignoreUnused(component, basePath, count, scale);
#endif
    }
};