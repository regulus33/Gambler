#pragma once

#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace SettingsGroupConstants {
    constexpr float cornerRadius = 0.0f;
    constexpr float borderWidth = 1.5f;
    constexpr float dashLength = 4.0f;
    constexpr float gapLength = 3.0f;
    constexpr float titleFontSize = 15.0f;
    constexpr int titleHeight = 28;
    constexpr int contentPaddingTop = 8;
    constexpr int contentPaddingHorizontal = 20;
    constexpr int contentPaddingBottom = 14;
}

/**
 * LabeledControlContainer
 *
 * A simple container that lays out a label and control horizontally
 */
class LabeledControlContainer : public juce::Component {
public:
    LabeledControlContainer(juce::Component& label, juce::Component& control, int labelWidth)
        : label_(label), control_(control), labelWidth_(labelWidth) {
        addAndMakeVisible(label_);
        addAndMakeVisible(control_);
    }

    void resized() override {
        auto area = getLocalBounds();
        label_.setBounds(area.removeFromLeft(labelWidth_));
        control_.setBounds(area);
    }

private:
    juce::Component& label_;
    juce::Component& control_;
    int labelWidth_;
};

/**
 * SettingsGroup
 * 
 * A reusable component that displays a titled box with rounded corners
 * and a lighter background for grouping related settings.
 * 
 * Usage:
 *   auto group = std::make_unique<SettingsGroup>("Group Title");
 *   group->addControl(myToggle);
 *   group->addControl(mySlider, myLabel);
 */
class SettingsGroup : public juce::Component {
public:
    explicit SettingsGroup(const juce::String& title) : titleText(title) {
        // Load Inter font
        interBoldFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Inter_24ptBold_ttf,
            BinaryData::Inter_24ptBold_ttfSize);
    }

    void paint(juce::Graphics& g) override {
        using namespace SettingsGroupConstants;

        auto bounds = getLocalBounds().toFloat().reduced(borderWidth * 0.5f);

        // Draw dotted border
        g.setColour(juce::Colours::white);
        float dashes[] = { dashLength, gapLength };
        juce::Path borderPath;
        borderPath.addRectangle(bounds);
        juce::PathStrokeType strokeType(borderWidth);
        juce::Path dashedPath;
        strokeType.createDashedStroke(dashedPath, borderPath, dashes, 2);
        g.fillPath(dashedPath);

        // Draw title
        auto titleBounds = getLocalBounds().toFloat().removeFromTop(static_cast<float>(titleHeight));
        g.setFont(juce::FontOptions(interBoldFont).withHeight(titleFontSize));
        g.drawText(titleText, titleBounds.reduced(static_cast<float>(contentPaddingHorizontal), 0),
                   juce::Justification::centredLeft, false);
    }

    void resized() override {
        using namespace SettingsGroupConstants;
        
        auto contentArea = getLocalBounds();
        contentArea.removeFromTop(titleHeight + contentPaddingTop);
        contentArea = contentArea.reduced(contentPaddingHorizontal, 0);
        contentArea.removeFromBottom(contentPaddingBottom);
        
        // Use FlexBox for vertical layout of controls
        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::column;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        
        for (auto& item : controlItems) {
            fb.items.add(item);
        }
        
        fb.performLayout(contentArea.toFloat());
    }

    /**
     * Add a single control (e.g., checkbox) that spans the full width
     */
    void addControl(juce::Component& control, int height = 30, int spacing = 12) {
        addAndMakeVisible(control);
        controlItems.add(juce::FlexItem(control)
            .withHeight(static_cast<float>(height))
            .withMargin(juce::FlexItem::Margin(0, 0, static_cast<float>(spacing), 0)));
    }

    /**
     * Add a labeled control (e.g., slider with label)
     * Label will be on the left, control on the right
     */
    void addLabeledControl(juce::Component& label, juce::Component& control,
                          int height = 30, int labelWidth = 120, int spacing = 12) {
        // Create a container for the label + control row
        auto* container = new LabeledControlContainer(label, control, labelWidth);
        addAndMakeVisible(container);
        containers.add(container);

        controlItems.add(juce::FlexItem(*container)
            .withHeight(static_cast<float>(height))
            .withMargin(juce::FlexItem::Margin(0, 0, static_cast<float>(spacing), 0)));
    }

    /**
     * Add a custom component with full control over height and spacing
     */
    void addCustomControl(juce::Component& control, int height, int spacing = 12) {
        addAndMakeVisible(control);
        controlItems.add(juce::FlexItem(control)
            .withHeight(static_cast<float>(height))
            .withMargin(juce::FlexItem::Margin(0, 0, static_cast<float>(spacing), 0)));
    }

    /**
     * Calculate the minimum height needed for this group based on its contents
     */
    int getMinimumHeight() const {
        using namespace SettingsGroupConstants;
        
        float totalHeight = titleHeight + contentPaddingTop + contentPaddingBottom;
        
        for (const auto& item : controlItems) {
            totalHeight += item.height + item.margin.bottom;
        }
        
        return static_cast<int>(totalHeight);
    }

private:
    juce::String titleText;
    juce::Typeface::Ptr interBoldFont;
    juce::Array<juce::FlexItem> controlItems;
    juce::OwnedArray<juce::Component> containers;  // For label+control pairs
};

