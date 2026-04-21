#pragma once

#include "StyledDropdownConstants.h"
#include "BinaryData.h"
#include "../Icons.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * StyledDropdownList
 *
 * A scrollable list shown inside a CallOutBox on iOS.
 * Supports touch-drag scrolling, checkmark on selected item,
 * optional delete X buttons, and scroll-fade indicators.
 */
class StyledDropdownList : public juce::Component {
public:
    StyledDropdownList(const juce::StringArray& items, int selectedIndex,
                       bool showDelete, int minWidth,
                       std::function<void(int)> onSelect,
                       std::function<void(int)> onDelete,
                       std::function<void()> onDismiss)
        : items_(items), selectedIndex_(selectedIndex), showDelete_(showDelete),
          selectCallback_(std::move(onSelect)), deleteCallback_(std::move(onDelete)),
          dismissCallback_(std::move(onDismiss))
    {
        using namespace StyledDropdownConstants;
        interFont_ = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        contentHeight_ = items_.size() * itemHeight;
        int visibleHeight = juce::jmin(contentHeight_, maxVisibleHeight);
        setSize(juce::jmax(minWidth, 200), juce::jmax(itemHeight, visibleHeight));
    }

    ~StyledDropdownList() override {
        if (dismissCallback_)
            juce::MessageManager::callAsync(dismissCallback_);
    }

    void paint(juce::Graphics& g) override {
        using namespace StyledDropdownConstants;
        g.fillAll(juce::Colour(0xFF1E1E1E));

        juce::Font font(interFont_);
        font.setHeight(fontSize);
        g.setFont(font);

        for (int i = 0; i < items_.size(); ++i) {
            int y = i * itemHeight + scrollOffset_;
            if (y + itemHeight < 0 || y > getHeight())
                continue;

            auto row = juce::Rectangle<int>(0, y, getWidth(), itemHeight);
            paintRow(g, i, row);
        }

        paintScrollFades(g);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        dragStartY_ = e.y;
        dragStartScrollOffset_ = scrollOffset_;
        isDragging_ = false;
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        int delta = e.y - dragStartY_;
        if (!isDragging_ && std::abs(delta) > dragThreshold_)
            isDragging_ = true;

        if (isDragging_) {
            scrollOffset_ = dragStartScrollOffset_ + delta;
            clampScroll();
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        using namespace StyledDropdownConstants;
        if (isDragging_) { isDragging_ = false; return; }

        int index = (e.y - scrollOffset_) / itemHeight;
        if (index < 0 || index >= items_.size()) return;

        if (showDelete_ && index < maxRows_ && deleteHitAreas_[index].contains(e.getPosition())) {
            auto cb = deleteCallback_;
            if (auto* callout = findParentComponentOfClass<juce::CallOutBox>())
                callout->dismiss();
            if (cb)
                juce::Timer::callAfterDelay(50, [cb, index]() { cb(index); });
        } else {
            if (selectCallback_) selectCallback_(index);
            if (auto* callout = findParentComponentOfClass<juce::CallOutBox>())
                callout->dismiss();
        }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        using namespace StyledDropdownConstants;
        int newHover = (e.y - scrollOffset_) / itemHeight;
        bool overDelete = false;
        if (showDelete_ && newHover >= 0 && newHover < items_.size() && newHover < maxRows_)
            overDelete = deleteHitAreas_[newHover].contains(e.getPosition());

        if (newHover != hoveredIndex_ || overDelete != isMouseOverDelete_) {
            hoveredIndex_ = newHover;
            isMouseOverDelete_ = overDelete;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override {
        hoveredIndex_ = -1;
        isMouseOverDelete_ = false;
        repaint();
    }

private:
    void paintRow(juce::Graphics& g, int i, juce::Rectangle<int> row) {
        using namespace StyledDropdownConstants;

        if (i == hoveredIndex_ && !isMouseOverDelete_ && !isDragging_) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRect(row);
        }

        auto area = row;
        area.removeFromLeft(textLeftPadding);
        int rightEdge = area.getRight() - iconsRightPadding;

        if (i == selectedIndex_)
            paintCheckmark(g, area, rightEdge);

        if (showDelete_)
            paintDeleteIcon(g, area, rightEdge, i);

        paintRowText(g, row, i);
    }

    void paintCheckmark(juce::Graphics& g, juce::Rectangle<int> area, int rightEdge) {
        using namespace StyledDropdownConstants;
        auto checkBounds = juce::Rectangle<float>(
            static_cast<float>(rightEdge - checkmarkSize),
            static_cast<float>(area.getCentreY()) - checkmarkSize / 2.0f,
            static_cast<float>(checkmarkSize),
            static_cast<float>(checkmarkSize));
        Icons::strokeIcon(g, Icons::createCheckmarkIcon(), checkBounds,
                          juce::Colours::white, 1.5f);
    }

    void paintDeleteIcon(juce::Graphics& g, juce::Rectangle<int> area, int rightEdge, int i) {
        using namespace StyledDropdownConstants;
        int deleteX = (i == selectedIndex_)
            ? rightEdge - checkmarkSize - iconSpacing - deleteIconSize
            : rightEdge - deleteIconSize;

        auto deleteBounds = juce::Rectangle<float>(
            static_cast<float>(deleteX),
            static_cast<float>(area.getCentreY()) - deleteIconSize / 2.0f,
            static_cast<float>(deleteIconSize),
            static_cast<float>(deleteIconSize));

        bool hoveringDelete = (i == hoveredIndex_ && isMouseOverDelete_ && !isDragging_);
        float deleteAlpha = hoveringDelete ? 1.0f : 0.5f;
        Icons::strokeIcon(g, Icons::createCloseIcon(), deleteBounds,
                          juce::Colours::white.withAlpha(deleteAlpha), 1.5f,
                          true, Icons::closeIconScale);

        if (i < maxRows_)
            deleteHitAreas_[i] = juce::Rectangle<int>(
                deleteX - 2, area.getCentreY() - deleteIconSize / 2 - 2,
                deleteIconSize + 4, deleteIconSize + 4);
    }

    void paintRowText(juce::Graphics& g, juce::Rectangle<int> row, int i) {
        using namespace StyledDropdownConstants;
        int iconsWidth = iconsRightPadding;
        if (showDelete_) {
            iconsWidth += deleteIconSize + iconsRightPadding;
            if (i == selectedIndex_) iconsWidth += checkmarkSize + iconSpacing;
        } else if (i == selectedIndex_) {
            iconsWidth += checkmarkSize + iconsRightPadding;
        }
        auto textArea = row.withTrimmedLeft(textLeftPadding).withTrimmedRight(iconsWidth);
        g.setColour(juce::Colours::white);
        g.drawText(items_[i], textArea, juce::Justification::centredLeft, true);
    }

    void paintScrollFades(juce::Graphics& g) {
        if (scrollOffset_ < 0) {
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xFF1E1E1E), 0.0f, 0.0f,
                juce::Colours::transparentBlack, 0.0f, 12.0f, false));
            g.fillRect(0, 0, getWidth(), 12);
        }
        if (scrollOffset_ + contentHeight_ > getHeight()) {
            float bottomY = static_cast<float>(getHeight());
            g.setGradientFill(juce::ColourGradient(
                juce::Colours::transparentBlack, 0.0f, bottomY - 12.0f,
                juce::Colour(0xFF1E1E1E), 0.0f, bottomY, false));
            g.fillRect(0, getHeight() - 12, getWidth(), 12);
        }
    }

    void clampScroll() {
        int maxScroll = 0;
        int minScroll = juce::jmin(0, getHeight() - contentHeight_);
        scrollOffset_ = juce::jlimit(minScroll, maxScroll, scrollOffset_);
    }

    static constexpr int maxRows_ = 64;
    static constexpr int dragThreshold_ = 8;

    juce::StringArray items_;
    int selectedIndex_ = -1;
    bool showDelete_ = false;
    int hoveredIndex_ = -1;
    bool isMouseOverDelete_ = false;
    std::function<void(int)> selectCallback_;
    std::function<void(int)> deleteCallback_;
    std::function<void()> dismissCallback_;
    juce::Typeface::Ptr interFont_;
    juce::Rectangle<int> deleteHitAreas_[maxRows_];

    int contentHeight_ = 0;
    int scrollOffset_ = 0;
    int dragStartY_ = 0;
    int dragStartScrollOffset_ = 0;
    bool isDragging_ = false;
};
