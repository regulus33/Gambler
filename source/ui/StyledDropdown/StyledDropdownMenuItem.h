#pragma once

#include "StyledDropdownConstants.h"
#include "BinaryData.h"
#include "../Icons.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * StyledDropdownMenuItem
 *
 * A custom PopupMenu item used on desktop. Draws a single row with
 * Inter font, right-aligned checkmark for selected item, and optional delete X.
 */
class StyledDropdownMenuItem : public juce::PopupMenu::CustomComponent {
public:
    StyledDropdownMenuItem(const juce::String& text, bool isSelected, bool showDelete,
                           std::function<void()> onSelect, std::function<void()> onDelete)
        : text_(text), selected_(isSelected), showDelete_(showDelete),
          selectCallback_(std::move(onSelect)), deleteCallback_(std::move(onDelete))
    {
        interFont_ = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
    }

    void paint(juce::Graphics& g) override {
        using namespace StyledDropdownConstants;
        auto area = getLocalBounds();

        if (isItemHighlighted() && !isMouseOverDelete_) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRect(area);
        }

        juce::Font font(interFont_);
        font.setHeight(fontSize);
        g.setFont(font);
        g.setColour(juce::Colours::white);

        int rightEdge = area.getRight() - iconsRightPadding;

        // Checkmark for selected item (always far right)
        if (selected_) {
            auto checkBounds = juce::Rectangle<float>(
                static_cast<float>(rightEdge - checkmarkSize),
                static_cast<float>(area.getCentreY()) - checkmarkSize / 2.0f,
                static_cast<float>(checkmarkSize),
                static_cast<float>(checkmarkSize));
            Icons::strokeIcon(g, Icons::createCheckmarkIcon(), checkBounds,
                              juce::Colours::white, 1.5f);
        }

        // Delete X icon
        if (showDelete_) {
            int deleteX = selected_
                ? rightEdge - checkmarkSize - iconSpacing - deleteIconSize
                : rightEdge - deleteIconSize;

            deleteButtonBounds_ = juce::Rectangle<int>(
                deleteX - 2, area.getCentreY() - deleteIconSize / 2 - 2,
                deleteIconSize + 4, deleteIconSize + 4);

            auto deleteBounds = juce::Rectangle<float>(
                static_cast<float>(deleteX),
                static_cast<float>(area.getCentreY()) - deleteIconSize / 2.0f,
                static_cast<float>(deleteIconSize),
                static_cast<float>(deleteIconSize));

            float alpha = isMouseOverDelete_ ? 1.0f : 0.5f;
            Icons::strokeIcon(g, Icons::createCloseIcon(), deleteBounds,
                              juce::Colours::white.withAlpha(alpha), 1.5f,
                              true, Icons::closeIconScale);
        }

        // Text
        auto textArea = area.reduced(textLeftPadding, 0);
        int iconsWidth = iconsRightPadding;
        if (showDelete_) {
            iconsWidth += deleteIconSize + iconsRightPadding;
            if (selected_) iconsWidth += checkmarkSize + iconSpacing;
        } else if (selected_) {
            iconsWidth += checkmarkSize + iconsRightPadding;
        }
        textArea.removeFromRight(iconsWidth);
        g.drawText(text_, textArea, juce::Justification::centredLeft, true);
    }

    void mouseMove(const juce::MouseEvent& e) override {
        bool was = isMouseOverDelete_;
        isMouseOverDelete_ = showDelete_ && deleteButtonBounds_.contains(e.getPosition());
        if (was != isMouseOverDelete_) repaint();
    }

    void mouseExit(const juce::MouseEvent&) override {
        if (isMouseOverDelete_) { isMouseOverDelete_ = false; repaint(); }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (showDelete_ && deleteButtonBounds_.contains(e.getPosition())) {
            if (deleteCallback_)
                juce::Timer::callAfterDelay(50, [cb = deleteCallback_]() { cb(); });
            triggerMenuItem();
        } else {
            if (selectCallback_) selectCallback_();
            triggerMenuItem();
        }
    }

    void getIdealSize(int& w, int& h) override {
        using namespace StyledDropdownConstants;
        juce::Font font(interFont_);
        font.setHeight(fontSize);
        w = font.getStringWidth(text_) + textLeftPadding * 2 +
            deleteIconSize + iconSpacing + checkmarkSize + iconsRightPadding + 20;
        h = itemHeight;
    }

private:
    juce::String text_;
    bool selected_;
    bool showDelete_;
    std::function<void()> selectCallback_;
    std::function<void()> deleteCallback_;
    juce::Typeface::Ptr interFont_;
    juce::Rectangle<int> deleteButtonBounds_;
    bool isMouseOverDelete_ = false;
};
