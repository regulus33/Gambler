#pragma once

#include "../ClickFlashMixin.h"
#include "../Icons.h"
#include "../ToggleIconButton.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * ExportButton
 *
 * A button that supports both click-to-save and drag-to-export.
 * - Click (mouseUp without drag): fires onClicked callback (opens file chooser)
 * - Drag (mouseDrag past threshold): fires onDragStarted to get a temp file,
 *   then initiates external file drag-and-drop into a DAW.
 *
 * Renders the export icon using the same style as ToggleIconButton.
 */
class ExportButton : public juce::Component {
public:
    ExportButton() {}

    void paint(juce::Graphics& g) override {
        using namespace ToggleIconButtonConstants;
        auto path = Icons::createExportIcon();
        float boundsSize = static_cast<float>(juce::jmin(getWidth(), getHeight()));
        float paddingRatio = static_cast<float>(padding) / static_cast<float>(totalSize);
        float actualPadding = boundsSize * paddingRatio;
        float actualContent = boundsSize - actualPadding * 2.0f;
        auto iconArea = juce::Rectangle<float>(actualPadding, actualPadding, actualContent, actualContent);
        Icons::strokeIcon(g, path, iconArea, juce::Colours::white, 1.5f, true, 1.0f);

        clickFlash_.paint(g, getLocalBounds());
    }

    void mouseDown(const juce::MouseEvent&) override {
        didDrag = false;
    }

    void mouseDrag(const juce::MouseEvent& event) override {
        if (!didDrag && event.getDistanceFromDragStart() > 8) {
            didDrag = true;
            if (onDragStarted) {
                auto tempFile = onDragStarted();
                if (tempFile != juce::File()) {
                    juce::DragAndDropContainer::performExternalDragDropOfFiles(
                        {tempFile.getFullPathName()}, false, this);
                }
            }
        }
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (!didDrag && onClicked) {
            clickFlash_.trigger();
            onClicked();
        }
    }

    void mouseEnter(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void parentHierarchyChanged() override {
        if (getTopLevelComponent() != nullptr)
            clickFlash_.attachTo(this);
    }

    std::function<void()> onClicked;
    std::function<juce::File()> onDragStarted;

private:
    bool didDrag = false;
    ClickFlash clickFlash_;
};
