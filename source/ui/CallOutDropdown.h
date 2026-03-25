#pragma once

#include "BinaryData.h"
#include "Icons.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * CallOutDropdownList
 *
 * A simple selectable list shown inside a CallOutBox.
 * Replaces ComboBox for iOS AUv3 compatibility.
 */
class CallOutDropdownList : public juce::Component {
public:
    CallOutDropdownList(const juce::StringArray& items, int currentIndex,
                        std::function<void(int)> onSelect)
        : items_(items), selectedIndex_(currentIndex),
          selectCallback_(std::move(onSelect))
    {
        int h = juce::jmax(itemHeight, items_.size() * itemHeight);
        setSize(180, juce::jmin(h, 350));
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xFF2A2A2A));

        for (int i = 0; i < items_.size(); ++i) {
            auto row = juce::Rectangle<int>(0, i * itemHeight, getWidth(), itemHeight);

            if (i == hoveredIndex_) {
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
    }

    void mouseMove(const juce::MouseEvent& e) override {
        int newHover = e.y / itemHeight;
        if (newHover != hoveredIndex_) {
            hoveredIndex_ = newHover;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override {
        hoveredIndex_ = -1;
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override {
        int index = e.y / itemHeight;
        if (index >= 0 && index < items_.size()) {
            if (selectCallback_)
                selectCallback_(index);
            if (auto* callout = findParentComponentOfClass<juce::CallOutBox>())
                callout->dismiss();
        }
    }

private:
    static constexpr int itemHeight = 28;
    juce::StringArray items_;
    int selectedIndex_ = -1;
    int hoveredIndex_ = -1;
    std::function<void(int)> selectCallback_;
};

/**
 * CallOutDropdown
 *
 * A ComboBox replacement that uses CallOutBox for iOS AUv3 compatibility.
 * Renders as a styled button with text and a chevron, opens a CallOutDropdownList
 * in a CallOutBox on click.
 */
class CallOutDropdown : public juce::Component {
public:
    CallOutDropdown() = default;

    void setItems(const juce::StringArray& items) {
        items_ = items;
    }

    void setSelectedIndex(int index, bool notify = false) {
        if (index >= 0 && index < items_.size()) {
            selectedIndex_ = index;
            repaint();
            if (notify && onChange)
                onChange();
        }
    }

    int getSelectedIndex() const { return selectedIndex_; }

    /** Returns 1-based ID for ComboBox compatibility */
    int getSelectedId() const { return selectedIndex_ + 1; }

    /** Sets selection using 1-based ID for ComboBox compatibility */
    void setSelectedId(int id, juce::NotificationType notification = juce::sendNotification) {
        selectedIndex_ = id - 1;
        repaint();
        if (notification != juce::dontSendNotification && onChange)
            onChange();
    }

    juce::String getSelectedText() const {
        if (selectedIndex_ >= 0 && selectedIndex_ < items_.size())
            return items_[selectedIndex_];
        return {};
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Border
        g.setColour(juce::Colours::grey);
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

        // Text
        g.setColour(juce::Colours::white);
        auto textArea = getLocalBounds().reduced(6, 0).withTrimmedRight(16);
        g.drawText(getSelectedText(), textArea, juce::Justification::centredLeft, true);

        // Chevron
        auto iconBounds = juce::Rectangle<float>(
            static_cast<float>(getWidth() - 14),
            static_cast<float>(getHeight() / 2 - 3),
            8.0f, 6.0f);
        auto chevron = Icons::createChevronDownIcon();
        Icons::strokeIcon(g, chevron, iconBounds, juce::Colours::white, 1.5f);
    }

    void mouseDown(const juce::MouseEvent&) override {
        auto* list = new CallOutDropdownList(items_, selectedIndex_,
            [this](int index) {
                setSelectedIndex(index, true);
            });

        auto screenBounds = localAreaToGlobal(getLocalBounds());

        juce::CallOutBox::launchAsynchronously(
            std::unique_ptr<juce::Component>(list),
            screenBounds,
            getTopLevelComponent());
    }

    std::function<void()> onChange;

private:
    juce::StringArray items_;
    int selectedIndex_ = -1;
};
