#pragma once

#include "BinaryData.h"
#include "ClickFlashMixin.h"
#include "Icons.h"
#include "../util/PlatformUtils.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include <functional>

namespace StyledDropdownConstants {
    constexpr float fontSize = 14.0f;
    constexpr int iconSize = 8;
    constexpr int iconRightPadding = 6;
    constexpr int textLeftPadding = 8;
    constexpr int itemHeight = 24;
    constexpr int maxVisibleHeight = 300;
    constexpr int checkmarkSize = 10;
    constexpr int deleteIconSize = 10;
    constexpr int iconSpacing = 6;
    constexpr int iconsRightPadding = 6;
    constexpr float borderWidth = 2.0f;
    constexpr float borderRadius = 4.0f;
}

/**
 * StyledDropdownMenuLookAndFeel
 *
 * Draws the dark popup menu background used on desktop.
 */
class StyledDropdownMenuLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override {
        g.fillAll(juce::Colour(0xFF1E1E1E));
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawRect(0, 0, width, height, 1);
    }

    void getIdealPopupMenuItemSize(const juce::String& /*text*/, bool /*isSeparator*/,
                                    int /*standardMenuItemHeight*/, int& idealWidth, int& idealHeight) override {
        idealWidth = 200;
        idealHeight = StyledDropdownConstants::itemHeight;
    }
};

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
                static_cast<float>(area.getCentreY() - checkmarkSize / 2),
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
                static_cast<float>(area.getCentreY() - deleteIconSize / 2),
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

            // Hover highlight
            if (i == hoveredIndex_ && !isMouseOverDelete_ && !isDragging_) {
                g.setColour(juce::Colours::white.withAlpha(0.1f));
                g.fillRect(row);
            }

            auto area = row;
            area.removeFromLeft(textLeftPadding);
            int rightEdge = area.getRight() - iconsRightPadding;

            // Checkmark for selected item
            if (i == selectedIndex_) {
                auto checkBounds = juce::Rectangle<float>(
                    static_cast<float>(rightEdge - checkmarkSize),
                    static_cast<float>(area.getCentreY() - checkmarkSize / 2),
                    static_cast<float>(checkmarkSize),
                    static_cast<float>(checkmarkSize));
                Icons::strokeIcon(g, Icons::createCheckmarkIcon(), checkBounds,
                                  juce::Colours::white, 1.5f);
            }

            // Delete X icon
            if (showDelete_) {
                int deleteX = (i == selectedIndex_)
                    ? rightEdge - checkmarkSize - iconSpacing - deleteIconSize
                    : rightEdge - deleteIconSize;

                auto deleteBounds = juce::Rectangle<float>(
                    static_cast<float>(deleteX),
                    static_cast<float>(area.getCentreY() - deleteIconSize / 2),
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

            // Text
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

        // Scroll fade indicators
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

/**
 * StyledDropdown
 *
 * A unified dropdown component used throughout the app.
 * Renders a bordered button with Inter font text and animated chevron.
 * Opens a styled popup (PopupMenu on desktop, CallOutBox on iOS).
 *
 * Features:
 * - Checkmark on selected item
 * - Optional delete X buttons per item
 * - Gold click-flash animation
 * - Smooth chevron rotation animation
 * - Touch-drag scrolling on iOS
 *
 * Configuration:
 * - setShowDeleteButtons(true) to enable per-item delete
 * - setPlaceholderText() for empty-state text
 *
 * Callbacks:
 * - onItemSelected(int index)  — fired when an item is picked (0-based)
 * - onItemDeleteRequested(int) — fired when delete X is clicked
 * - onChange()                  — simple notification, fired alongside onItemSelected
 */
class StyledDropdown : public juce::Component {
public:
    StyledDropdown() {
        interFont_ = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
    }

    // -- Items ----------------------------------------------------------

    void setItems(const juce::StringArray& items) { items_ = items; }

    // -- Selection (0-based index) --------------------------------------

    void setSelectedIndex(int index, bool sendNotification = false) {
        if (index >= 0 && index < items_.size()) {
            selectedIndex_ = index;
            repaint();
            if (sendNotification) notifySelection(index);
        }
    }

    int getSelectedIndex() const { return selectedIndex_; }

    void setSelectedByName(const juce::String& name, bool sendNotification = false) {
        int index = items_.indexOf(name);
        if (index >= 0) setSelectedIndex(index, sendNotification);
    }

    void clearSelection() {
        selectedIndex_ = -1;
        repaint();
    }

    juce::String getSelectedText() const {
        if (selectedIndex_ >= 0 && selectedIndex_ < items_.size())
            return items_[selectedIndex_];
        return {};
    }

    // -- 1-based ID helpers (for legacy call sites) ---------------------

    void setSelectedId(int id, juce::NotificationType notification = juce::dontSendNotification) {
        selectedIndex_ = id - 1;
        repaint();
        if (notification != juce::dontSendNotification)
            notifySelection(selectedIndex_);
    }

    int getSelectedId() const { return selectedIndex_ + 1; }

    // -- Configuration --------------------------------------------------

    void setShowDeleteButtons(bool show) { showDelete_ = show; }
    void setPlaceholderText(const juce::String& text) { placeholderText_ = text; }

    // -- Callbacks ------------------------------------------------------

    std::function<void(int)> onItemSelected;
    std::function<void(int)> onItemDeleteRequested;
    std::function<void()> onChange;

    // -- Component overrides --------------------------------------------

    void paint(juce::Graphics& g) override {
        using namespace StyledDropdownConstants;

        // Border
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(getLocalBounds().toFloat(), borderRadius, borderWidth);

        // Text
        juce::Font font(interFont_);
        font.setHeight(fontSize);
        g.setFont(font);

        juce::String displayText = (selectedIndex_ >= 0 && selectedIndex_ < items_.size())
            ? items_[selectedIndex_] : placeholderText_;
        g.setColour(selectedIndex_ >= 0 ? juce::Colours::white : juce::Colours::white.withAlpha(0.5f));

        auto textArea = getLocalBounds().reduced(textLeftPadding, 0);
        textArea.removeFromRight(iconSize + iconRightPadding);
        g.drawText(displayText, textArea, juce::Justification::centredLeft, true);

        // Chevron with rotation
        {
            float cx = static_cast<float>(getWidth() - iconSize - iconRightPadding) + static_cast<float>(iconSize) * 0.5f;
            float cy = static_cast<float>(getHeight()) * 0.5f;
            auto iconBounds = juce::Rectangle<float>(
                cx - static_cast<float>(iconSize) * 0.5f,
                cy - static_cast<float>(iconSize) * 0.5f,
                static_cast<float>(iconSize),
                static_cast<float>(iconSize));

            auto chevronPath = Icons::createChevronDownIcon();

            if (std::abs(chevronAngle_) > 0.001f) {
                g.saveState();
                g.addTransform(juce::AffineTransform::rotation(chevronAngle_, cx, cy));
            }

            Icons::strokeIcon(g, chevronPath, iconBounds, juce::Colours::white, 1.5f);

            if (std::abs(chevronAngle_) > 0.001f)
                g.restoreState();
        }

        // Click flash overlay
        clickFlash_.paint(g, getLocalBounds());
    }

    void mouseDown(const juce::MouseEvent&) override {
        clickFlash_.trigger();
        setPopupOpen(true);
        showPopup();
    }

    void parentHierarchyChanged() override {
        if (getTopLevelComponent() != nullptr) {
            clickFlash_.attachTo(this);
            ensureVBlank();
        }
    }

    void mouseEnter(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

private:
    void notifySelection(int index) {
        if (onItemSelected) onItemSelected(index);
        if (onChange) onChange();
    }

    void showPopup() {
        if (PlatformUtils::isIOS())
            showPopupIOS();
        else
            showPopupDesktop();
    }

    void showPopupDesktop() {
        juce::PopupMenu menu;
        menu.setLookAndFeel(&menuLookAndFeel_);

        for (int i = 0; i < items_.size(); ++i) {
            bool isCurrent = (i == selectedIndex_);
            int idx = i;

            auto item = std::make_unique<StyledDropdownMenuItem>(
                items_[i], isCurrent, showDelete_,
                [this, idx]() {
                    setSelectedIndex(idx, false);
                    notifySelection(idx);
                },
                [this, idx]() {
                    if (onItemDeleteRequested)
                        onItemDeleteRequested(idx);
                });

            menu.addCustomItem(-1, std::move(item), nullptr, items_[i]);
        }

        if (items_.isEmpty())
            menu.addItem(1, "No Items", false);

        auto options = juce::PopupMenu::Options()
            .withTargetComponent(this)
            .withMinimumWidth(getWidth())
            .withMaximumNumColumns(1);

        menu.showMenuAsync(options, [this](int) { setPopupOpen(false); });
    }

    void showPopupIOS() {
        auto* list = new StyledDropdownList(items_, selectedIndex_, showDelete_, getWidth(),
            [this](int index) {
                setSelectedIndex(index, false);
                notifySelection(index);
            },
            [this](int index) {
                if (onItemDeleteRequested)
                    onItemDeleteRequested(index);
            },
            [this]() { setPopupOpen(false); });

        juce::CallOutBox::launchAsynchronously(
            std::unique_ptr<juce::Component>(list),
            localAreaToGlobal(getLocalBounds()),
            getTopLevelComponent());
    }

    void setPopupOpen(bool open) {
        chevronTargetAngle_ = open ? juce::MathConstants<float>::pi : 0.0f;
        needsChevronAnimation_ = true;
        ensureVBlank();
    }

    void ensureVBlank() {
        if (!vBlank_ && getTopLevelComponent() != nullptr) {
            vBlank_ = std::make_unique<juce::VBlankAttachment>(
                this, [this](double ts) { onVBlank(ts); });
        }
    }

    void onVBlank(double timestampSec) {
        if (!needsChevronAnimation_) return;

        double delta = (lastVBlankTimestamp_ > 0.0)
            ? (timestampSec - lastVBlankTimestamp_)
            : (1.0 / 60.0);
        lastVBlankTimestamp_ = timestampSec;
        float dt = static_cast<float>(juce::jlimit(0.001, 0.1, delta));

        float diff = chevronTargetAngle_ - chevronAngle_;
        if (std::abs(diff) > 0.01f) {
            chevronAngle_ += diff * 12.0f * dt;
            repaint();
        } else {
            chevronAngle_ = chevronTargetAngle_;
            needsChevronAnimation_ = false;
            repaint();
        }
    }

    juce::StringArray items_;
    int selectedIndex_ = -1;
    bool showDelete_ = false;
    juce::String placeholderText_ { "Select..." };
    juce::Typeface::Ptr interFont_;
    StyledDropdownMenuLookAndFeel menuLookAndFeel_;
    ClickFlash clickFlash_;
    float chevronAngle_ = 0.0f;
    float chevronTargetAngle_ = 0.0f;
    bool needsChevronAnimation_ = false;
    double lastVBlankTimestamp_ = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vBlank_;
};
