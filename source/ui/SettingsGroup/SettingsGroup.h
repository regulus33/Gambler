#pragma once

#include "SettingsGroupConstants.h"
#include "LabeledControlContainer.h"
#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * SettingsGroup
 *
 * A reusable component that displays a titled box with rounded corners
 * and a lighter background for grouping related settings.
 */
class SettingsGroup : public juce::Component {
public:
    explicit SettingsGroup(const juce::String& title) : titleText(title) {
        interBoldFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Inter_24ptBold_ttf,
            BinaryData::Inter_24ptBold_ttfSize);
    }

    void paint(juce::Graphics& g) override {
        paintBorder(g);
        paintTitle(g);
    }

    void resized() override {
        using namespace SettingsGroupConstants;

        auto contentArea = getLocalBounds();
        contentArea.removeFromTop(titleHeight + contentPaddingTop);
        contentArea = contentArea.reduced(contentPaddingHorizontal, 0);
        contentArea.removeFromBottom(contentPaddingBottom);

        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::column;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;

        for (auto& item : controlItems)
            fb.items.add(item);

        fb.performLayout(contentArea.toFloat());
    }

    void addControl(juce::Component& control, int height = 30, int spacing = 8) {
        addAndMakeVisible(control);
        controlItems.add(makeFlexItem(control, height, spacing));
    }

    void addLabeledControl(juce::Component& label, juce::Component& control,
                           int height = 30, int labelWidth = 120, int spacing = 8) {
        auto* container = new LabeledControlContainer(label, control, labelWidth);
        addAndMakeVisible(container);
        containers.add(container);

        controlItems.add(makeFlexItem(*container, height, spacing));
    }

    void addCustomControl(juce::Component& control, int height, int spacing = 8) {
        addAndMakeVisible(control);
        controlItems.add(makeFlexItem(control, height, spacing));
    }

    int getMinimumHeight() const {
        using namespace SettingsGroupConstants;
        float totalHeight = titleHeight + contentPaddingTop + contentPaddingBottom;
        for (const auto& item : controlItems)
            totalHeight += item.height + item.margin.bottom;
        return static_cast<int>(totalHeight);
    }

private:
    static juce::FlexItem makeFlexItem(juce::Component& c, int height, int spacing) {
        return juce::FlexItem(c)
            .withHeight(static_cast<float>(height))
            .withMargin(juce::FlexItem::Margin(0, 0, static_cast<float>(spacing), 0));
    }

    void paintBorder(juce::Graphics& g) {
        using namespace SettingsGroupConstants;
        auto bounds = getLocalBounds().toFloat().reduced(borderWidth * 0.5f);
        g.setColour(juce::Colours::white);
        float dashes[] = { dashLength, gapLength };
        juce::Path borderPath;
        borderPath.addRectangle(bounds);
        juce::PathStrokeType strokeType(borderWidth);
        juce::Path dashedPath;
        strokeType.createDashedStroke(dashedPath, borderPath, dashes, 2);
        g.fillPath(dashedPath);
    }

    void paintTitle(juce::Graphics& g) {
        using namespace SettingsGroupConstants;
        auto titleBounds = getLocalBounds().toFloat().removeFromTop(static_cast<float>(titleHeight));
        g.setFont(juce::FontOptions(interBoldFont).withHeight(titleFontSize));
        g.drawText(titleText, titleBounds.reduced(static_cast<float>(contentPaddingHorizontal), 0),
                   juce::Justification::centredLeft, false);
    }

    juce::String titleText;
    juce::Typeface::Ptr interBoldFont;
    juce::Array<juce::FlexItem> controlItems;
    juce::OwnedArray<juce::Component> containers;
};
