#pragma once
#include <PluginEditor.h>
#include <PluginProcessor.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <cstdlib>
#include <ctime>

using std::make_unique;

// ============================================================================
// JUCE MessageManager RAII initialiser — required for any test that creates
// UI components. Instantiate once (static or local) before building any UI.
// ============================================================================
class GamblerTestInitialiser {
public:
    GamblerTestInitialiser() { juce::MessageManager::getInstance(); }
    ~GamblerTestInitialiser() { juce::MessageManager::deleteInstance(); }
};

// ============================================================================
// Test fixture — creates a fully initialised PluginProcessor + PluginEditor.
// Requires a live GamblerTestInitialiser to exist before construction.
// ============================================================================
class PluginTestFixture {
public:
    PluginTestFixture() : editor (processor) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    }
    ~PluginTestFixture() {
        juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    }

    PluginProcessor& getProcessor() { return processor; }
    PluginEditor& getEditor() { return editor; }
    SequenceState& getState() { return processor.getState(); }
    MainScreenComponent& getMainScreen() {
        return editor.getApplicationComponent().getMainScreen();
    }
    NavBarComponent& getNavBar() {
        return editor.getApplicationComponent().getNavBar();
    }
    void processMessages (int timeoutMs = 50) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil (timeoutMs);
    }
    void simulateAudioBlock (int numSamples = 512) {
        juce::AudioBuffer<float> buffer (2, numSamples);
        juce::MidiBuffer midiBuffer;
        processor.processBlock (buffer, midiBuffer);
    }

private:
    PluginProcessor processor;
    PluginEditor editor;
};

// ============================================================================
// Snapshot Testing Helper
// ============================================================================
//
// Visual regression testing for JUCE components. Compares rendered component
// against a "golden" reference image stored in tests/snapshots/.
//
// Usage:
//   SnapshotTester snap("my_test_name");              // Default 1% tolerance
//   SnapshotTester snap("my_test_name", 1.0f, 0.0f);  // Exact match (0% tolerance)
//   snap.capture(myComponent);
//   REQUIRE(snap.matches());  // Fails if different from golden
//
// Parameters:
//   - testName: Unique name for this snapshot
//   - scale: Render scale (1.0f = normal, 2.0f = 2x for retina)
//   - tolerance: Percentage of pixels allowed to differ (0.0 - 1.0)
//                Default 0.01 (1%) handles anti-aliasing & compression artifacts
//
// First run: Creates golden image (test passes, golden saved)
// Later runs: Compares against golden (fails if different)
//
// To update goldens after intentional changes:
//   UPDATE_SNAPSHOTS=1 ./build/Tests
//
// ============================================================================

class SnapshotTester {
public:
    explicit SnapshotTester(const juce::String& testName, float scale = 1.0f, float tolerance = 0.01f)
        : name(testName), renderScale(scale), pixelTolerance(tolerance) {
        // Golden snapshots stored in tests/snapshots/
        snapshotDir = juce::File(REPO_ROOT_PATH).getChildFile("tests/snapshots");
        snapshotDir.createDirectory();
    }

    // Capture the current state of a component
    void capture(juce::Component& component) {
        currentImage = component.createComponentSnapshot(
            component.getLocalBounds(),
            true,  // paintEntireComponent
            renderScale
        );
    }

    // Check if current snapshot matches the golden reference
    // Returns true if: images match, OR no golden exists (first run)
    bool matches() {
        juce::File goldenFile = snapshotDir.getChildFile(name + ".png");

        // Check if we should update goldens (env var)
        bool shouldUpdate = (std::getenv("UPDATE_SNAPSHOTS") != nullptr);

        if (!goldenFile.existsAsFile() || shouldUpdate) {
            // No golden exists or update requested - save current as golden
            saveImage(currentImage, goldenFile);
            DBG("Snapshot saved: " + goldenFile.getFullPathName());
            return true;  // First run always passes
        }

        // Load golden and compare
        juce::PNGImageFormat pngFormat;
        juce::FileInputStream inputStream(goldenFile);
        if (!inputStream.openedOk()) {
            DBG("Failed to open golden snapshot: " + goldenFile.getFullPathName());
            return false;
        }

        goldenImage = pngFormat.decodeImage(inputStream);
        return imagesMatch(currentImage, goldenImage);
    }

    // Get the difference count (useful for debugging)
    int getDifferenceCount() const { return differenceCount; }

    // Save the current image for debugging when test fails
    void saveFailureArtifacts() {
        juce::File actualFile = snapshotDir.getChildFile(name + "_ACTUAL.png");
        juce::File diffFile = snapshotDir.getChildFile(name + "_DIFF.png");

        saveImage(currentImage, actualFile);

        // Create a diff image highlighting differences
        if (goldenImage.isValid() && currentImage.isValid()) {
            auto diffImage = createDiffImage(goldenImage, currentImage);
            saveImage(diffImage, diffFile);
        }

        DBG("Failure artifacts saved:");
        DBG("  Expected: " + snapshotDir.getChildFile(name + ".png").getFullPathName());
        DBG("  Actual:   " + actualFile.getFullPathName());
        DBG("  Diff:     " + diffFile.getFullPathName());
    }

private:
    juce::String name;
    float renderScale;
    float pixelTolerance;  // Percentage of pixels allowed to differ (0.0 - 1.0)
    juce::File snapshotDir;
    juce::Image currentImage;
    juce::Image goldenImage;
    int differenceCount = 0;

    bool imagesMatch(const juce::Image& a, const juce::Image& b) {
        // Size must match
        if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight()) {
            DBG("Snapshot size mismatch: " +
                juce::String(a.getWidth()) + "x" + juce::String(a.getHeight()) + " vs " +
                juce::String(b.getWidth()) + "x" + juce::String(b.getHeight()));
            differenceCount = -1;  // Indicate size mismatch
            return false;
        }

        // Compare pixels with tolerance
        differenceCount = 0;
        int totalPixels = a.getWidth() * a.getHeight();

        for (int y = 0; y < a.getHeight(); ++y) {
            for (int x = 0; x < a.getWidth(); ++x) {
                auto pixelA = a.getPixelAt(x, y);
                auto pixelB = b.getPixelAt(x, y);

                // Check if pixels are different enough to count
                if (!pixelsAreSimilar(pixelA, pixelB)) {
                    differenceCount++;
                }
            }
        }

        // Calculate percentage of different pixels
        float differencePercentage = static_cast<float>(differenceCount) / static_cast<float>(totalPixels);

        if (differenceCount > 0) {
            DBG("Snapshot difference: " + juce::String(differenceCount) + " pixels (" +
                juce::String(differencePercentage * 100.0f, 2) + "%)");
        }

        return differencePercentage <= pixelTolerance;
    }

    // Helper to check if two pixels are similar enough (allows for minor rendering differences)
    bool pixelsAreSimilar(juce::Colour a, juce::Colour b, int threshold = 2) const {
        // Allow small differences in each channel (helps with anti-aliasing, compression artifacts)
        return std::abs(a.getRed() - b.getRed()) <= threshold &&
               std::abs(a.getGreen() - b.getGreen()) <= threshold &&
               std::abs(a.getBlue() - b.getBlue()) <= threshold &&
               std::abs(a.getAlpha() - b.getAlpha()) <= threshold;
    }

    juce::Image createDiffImage(const juce::Image& expected, const juce::Image& actual) {
        int w = juce::jmax(expected.getWidth(), actual.getWidth());
        int h = juce::jmax(expected.getHeight(), actual.getHeight());

        juce::Image diff(juce::Image::ARGB, w, h, true);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                auto expPixel = (x < expected.getWidth() && y < expected.getHeight())
                    ? expected.getPixelAt(x, y) : juce::Colours::black;
                auto actPixel = (x < actual.getWidth() && y < actual.getHeight())
                    ? actual.getPixelAt(x, y) : juce::Colours::black;

                if (expPixel != actPixel) {
                    // Highlight differences in red
                    diff.setPixelAt(x, y, juce::Colours::red);
                } else {
                    // Keep matching pixels but dimmed
                    diff.setPixelAt(x, y, actPixel.withAlpha(0.3f));
                }
            }
        }
        return diff;
    }

    void saveImage(const juce::Image& image, const juce::File& file) {
        file.deleteFile();
        juce::FileOutputStream stream(file);
        if (stream.openedOk()) {
            juce::PNGImageFormat pngFormat;
            pngFormat.writeImageToStream(image, stream);
        }
    }
};


/* This is a helper function to run tests within the context of a plugin editor.
 *
 * Read more here: https://github.com/sudara/pamplejuce/issues/18#issuecomment-1425836807
 *
 * Example usage (screenshots the plugin)
 *
  runWithinPluginEditor ([&] (PluginProcessor& plugin) {
    auto snapshot = plugin.getActiveEditor()->createComponentSnapshot (plugin.getActiveEditor()->getLocalBounds(), true, 2.0f);
    auto file = juce::File::getSpecialLocation (juce::File::SpecialLocationType::userDocumentsDirectory).getChildFile ("snapshot.jpeg");
    file.deleteFile();
    juce::FileOutputStream stream (file);
    juce::JPEGImageFormat jpeg;
    jpeg.writeImageToStream (snapshot, stream);

    REQUIRE (file.existsAsFile());
   });

 */
[[maybe_unused]] static void runWithinPluginEditor (const std::function<void (PluginProcessor& plugin)>& testCode)
{
    PluginProcessor plugin;
    const auto editor = plugin.createEditorIfNeeded();

    testCode (plugin);

    plugin.editorBeingDeleted (editor);
    delete editor;
}

// Warning don't call this too much bc it seeds on every call which is slow I think. For high performance seed once at start.
static int getRandFromZeroToExcluding(int max) {
    srand(time(0));
    return rand() % max;
}
