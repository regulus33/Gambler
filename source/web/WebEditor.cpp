#include "WebEditor.h"
#include "BinaryData.h"
#include "../backend/MidiExporter.h"
#include "../native/iOSNativeDialogs.h"
#include "../util/PlatformUtils.h"

namespace {
    constexpr juce::uint32 backgroundColour = 0xFF1E1E1E;  // matches --bg in web/css/style.css
    constexpr int defaultWidth = 1180;
    constexpr int defaultHeight = 820;

    const char* mimeTypeFor (const juce::String& fileName) {
        if (fileName.endsWithIgnoreCase (".html")) return "text/html";
        if (fileName.endsWithIgnoreCase (".css"))  return "text/css";
        if (fileName.endsWithIgnoreCase (".js"))   return "text/javascript";
        if (fileName.endsWithIgnoreCase (".ttf"))  return "font/ttf";
        if (fileName.endsWithIgnoreCase (".svg"))  return "image/svg+xml";
        if (fileName.endsWithIgnoreCase (".png"))  return "image/png";
        return "application/octet-stream";
    }
}

WebEditor::WebEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      bridge (p.getUiBridge()),
      webView (juce::WebBrowserComponent::Options()
                   .withNativeIntegrationEnabled()
                   .withKeepPageLoadedWhenBrowserIsHidden()
                   .withResourceProvider ([] (const juce::String& url) { return provideResource (url); })
                   .withEventListener ("action", [this] (const juce::var& message) {
                       bridge.handleAction (message["action"].toString(), message["payload"]);
                   })) {
    addAndMakeVisible (webView);

    bridge.onEvent = [this] (const juce::Identifier& event, const juce::var& payload) {
        webView.emitEventIfBrowserIsVisible (event, payload);
    };
    bridge.onExportRequested = [this] { exportMidi(); };

    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // The layout is fluid: no fixed aspect ratio. On iOS the host decides the size.
    setResizable (true, ! PlatformUtils::isIOS());
    setResizeLimits (480, 220, 4096, 4096);
    setSize (defaultWidth, defaultHeight);
}

WebEditor::~WebEditor() {
    bridge.onEvent = nullptr;
    bridge.onExportRequested = nullptr;
}

void WebEditor::paint (juce::Graphics& g) {
    g.fillAll (juce::Colour (backgroundColour));  // shown for a moment before the page paints
}

void WebEditor::resized() {
    webView.setBounds (getLocalBounds());
}

// The web files are compiled into BinaryData with their directories flattened, so
// a request for "css/style.css" is answered by the resource originally named "style.css".
std::optional<WebEditor::Resource> WebEditor::provideResource (const juce::String& url) {
    auto path = url.upToFirstOccurrenceOf ("?", false, false);
    auto fileName = path.fromLastOccurrenceOf ("/", false, false);
    if (fileName.isEmpty()) fileName = "index.html";

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i) {
        if (fileName != BinaryData::originalFilenames[i]) continue;

        int size = 0;
        const auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
        if (data == nullptr) break;

        std::vector<std::byte> bytes (static_cast<size_t> (size));
        std::memcpy (bytes.data(), data, bytes.size());
        return Resource { std::move (bytes), mimeTypeFor (fileName) };
    }
    return std::nullopt;
}

void WebEditor::exportMidi() {
    if (PlatformUtils::isIOS()) {
        iOSNativeDialogs::showTextInputAlert (
            this, "Export MIDI", "File name", "Gambler-Export",
            [this] (const juce::String& fileName) {
                const auto name = fileName.trim().isEmpty() ? juce::String ("Gambler-Export") : fileName.trim();
                const auto midiFile = MidiExporter::createMidiFile (processorRef.getState());
                exportedFile = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (name + ".mid");
                MidiExporter::writeToFile (midiFile, exportedFile);
                iOSNativeDialogs::shareFile (this, exportedFile, [] {});
            });
        return;
    }

    const auto midiFile = MidiExporter::createMidiFile (processorRef.getState());
    fileChooser = std::make_unique<juce::FileChooser> (
        "Save MIDI File",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Gambler-Export.mid"),
        "*.mid");
    fileChooser->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [midiFile] (const juce::FileChooser& chooser) {
            const auto result = chooser.getResult();
            if (result != juce::File()) MidiExporter::writeToFile (midiFile, result);
        });
}
