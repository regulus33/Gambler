#pragma once

#include "BinaryData.h"
#include "Icons.h"
#include "../util/PlatformUtils.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * CallOutDropdownList
 *
 * A selectable list shown inside a CallOutBox with touch-drag scrolling.
 * Replaces ComboBox for iOS AUv3 compatibility.
 *
 * Touch behavior:
 * - Short tap (< 8px movement): selects the item
 * - Drag: scrolls the list with momentum-free 1:1 tracking
 */
class CallOutDropdownList : public juce::Component {
public:
    CallOutDropdownList(const juce::StringArray& items, int currentIndex,
                        std::function<void(int)> onSelect)
        : items_(items), selectedIndex_(currentIndex),
          selectCallback_(std::move(onSelect))
    {
        contentHeight_ = items_.size() * itemHeight;
        int visibleHeight = juce::jmin(contentHeight_, maxVisibleHeight);
        setSize(180, visibleHeight);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xFF2A2A2A));

        for (int i = 0; i < items_.size(); ++i) {
            int y = i * itemHeight + scrollOffset_;
            if (y + itemHeight < 0 || y > getHeight())
                continue; // skip offscreen rows

            auto row = juce::Rectangle<int>(0, y, getWidth(), itemHeight);

            if (i == hoveredIndex_ && !isDragging_) {
                g.setColour(juce::Colours::white.withAlpha(0.1f));
                g.fillRect(row);
            }

            // Checkmark for selected
            if (i == selectedIndex_) {
                auto checkBounds = juce::Rectangle<float>(
                    4.0f,
                    static_cast<float>(row.getCentreY() - 5),
                    10.0f, 10.0f);
                Icons::strokeIcon(g, Icons::createCheckmarkIcon(), checkBounds,
                                  juce::Colours::white, 1.5f);
            }

            g.setColour(juce::Colours::white);
            g.drawText(items_[i], row.withTrimmedLeft(22).withTrimmedRight(6),
                       juce::Justification::centredLeft, true);
        }

        // Scroll indicators when content is clipped
        if (scrollOffset_ < 0) {
            // Fade at top
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xFF2A2A2A), 0.0f, 0.0f,
                juce::Colours::transparentBlack, 0.0f, 12.0f, false));
            g.fillRect(0, 0, getWidth(), 12);
        }
        if (scrollOffset_ + contentHeight_ > getHeight()) {
            // Fade at bottom
            float bottomY = static_cast<float>(getHeight());
            g.setGradientFill(juce::ColourGradient(
                juce::Colours::transparentBlack, 0.0f, bottomY - 12.0f,
                juce::Colour(0xFF2A2A2A), 0.0f, bottomY, false));
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
        if (!isDragging_ && std::abs(delta) > dragThreshold)
            isDragging_ = true;

        if (isDragging_) {
            scrollOffset_ = dragStartScrollOffset_ + delta;
            clampScroll();
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (!isDragging_) {
            // Tap — select the item under the finger
            int index = (e.y - scrollOffset_) / itemHeight;
            if (index >= 0 && index < items_.size()) {
                if (selectCallback_)
                    selectCallback_(index);
                if (auto* callout = findParentComponentOfClass<juce::CallOutBox>())
                    callout->dismiss();
            }
        }
        isDragging_ = false;
    }

    void mouseMove(const juce::MouseEvent& e) override {
        int newHover = (e.y - scrollOffset_) / itemHeight;
        if (newHover != hoveredIndex_) {
            hoveredIndex_ = newHover;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override {
        hoveredIndex_ = -1;
        repaint();
    }

private:
    void clampScroll() {
        int maxScroll = 0; // top of list at top of view
        int minScroll = juce::jmin(0, getHeight() - contentHeight_); // bottom of list at bottom of view
        scrollOffset_ = juce::jlimit(minScroll, maxScroll, scrollOffset_);
    }

    static constexpr int itemHeight = 28;
    static constexpr int maxVisibleHeight = 350;
    static constexpr int dragThreshold = 8;

    juce::StringArray items_;
    int selectedIndex_ = -1;
    int hoveredIndex_ = -1;
    std::function<void(int)> selectCallback_;

    int contentHeight_ = 0;
    int scrollOffset_ = 0;
    int dragStartY_ = 0;
    int dragStartScrollOffset_ = 0;
    bool isDragging_ = false;
};

/**
 * CallOutDropdown
 *
 * Drop-in ComboBox replacement.
 * - Desktop: delegates to a native juce::ComboBox (PopupMenu works fine)
 * - iOS: renders a custom button, opens a CallOutDropdownList in a CallOutBox
 */
class CallOutDropdown : public juce::Component {
public:
    CallOutDropdown() {
        if (!PlatformUtils::isIOS()) {
            comboBox_ = std::make_unique<juce::ComboBox>();
            comboBox_->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF2A2A2A));
            comboBox_->setColour(juce::ComboBox::textColourId, juce::Colours::white);
            comboBox_->setColour(juce::ComboBox::outlineColourId, juce::Colours::grey);
            comboBox_->setColour(juce::ComboBox::arrowColourId, juce::Colours::white);
            comboBox_->onChange = [this] {
                selectedIndex_ = comboBox_->getSelectedId() - 1;
                if (onChange) onChange();
            };
            addAndMakeVisible(*comboBox_);
        }
    }

    void resized() override {
        if (comboBox_)
            comboBox_->setBounds(getLocalBounds());
    }

    void setItems(const juce::StringArray& items) {
        items_ = items;
        if (comboBox_) {
            comboBox_->clear(juce::dontSendNotification);
            for (int i = 0; i < items.size(); ++i)
                comboBox_->addItem(items[i], i + 1);
        }
    }

    void setSelectedIndex(int index, bool notify = false) {
        if (index >= 0 && index < items_.size()) {
            selectedIndex_ = index;
            if (comboBox_)
                comboBox_->setSelectedId(index + 1, notify ? juce::sendNotification : juce::dontSendNotification);
            else
                repaint();
            if (notify && onChange)
                onChange();
        }
    }

    int getSelectedIndex() const { return selectedIndex_; }

    int getSelectedId() const { return selectedIndex_ + 1; }

    void setSelectedId(int id, juce::NotificationType notification = juce::sendNotification) {
        selectedIndex_ = id - 1;
        if (comboBox_)
            comboBox_->setSelectedId(id, notification);
        else
            repaint();
        if (notification != juce::dontSendNotification && onChange)
            onChange();
    }

    juce::String getSelectedText() const {
        if (selectedIndex_ >= 0 && selectedIndex_ < items_.size())
            return items_[selectedIndex_];
        return {};
    }

    // iOS-only rendering
    void paint(juce::Graphics& g) override {
        if (comboBox_) return; // desktop uses ComboBox

        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colours::grey);
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

        g.setColour(juce::Colours::white);
        g.drawText(getSelectedText(), getLocalBounds().reduced(6, 0).withTrimmedRight(16),
                   juce::Justification::centredLeft, true);

        auto iconBounds = juce::Rectangle<float>(
            static_cast<float>(getWidth() - 14),
            static_cast<float>(getHeight() / 2 - 3), 8.0f, 6.0f);
        Icons::strokeIcon(g, Icons::createChevronDownIcon(), iconBounds, juce::Colours::white, 1.5f);
    }

    // iOS-only click handler
    void mouseDown(const juce::MouseEvent&) override {
        if (comboBox_) return; // desktop uses ComboBox

        auto* list = new CallOutDropdownList(items_, selectedIndex_,
            [this](int index) { setSelectedIndex(index, true); });

        juce::CallOutBox::launchAsynchronously(
            std::unique_ptr<juce::Component>(list),
            localAreaToGlobal(getLocalBounds()),
            getTopLevelComponent());
    }

    std::function<void()> onChange;

private:
    juce::StringArray items_;
    int selectedIndex_ = -1;
    std::unique_ptr<juce::ComboBox> comboBox_; // desktop only
};
