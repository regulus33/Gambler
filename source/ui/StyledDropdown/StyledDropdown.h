#pragma once

#include "StyledDropdownConstants.h"
#include "StyledDropdownMenuLookAndFeel.h"
#include "StyledDropdownMenuItem.h"
#include "StyledDropdownList.h"
#include "BinaryData.h"
#include "../ClickFlashMixin.h"
#include "../Icons.h"
#include "../../util/PlatformUtils.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include <functional>

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

    void setItems(const juce::StringArray& items) { items_ = items; }

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

    void setSelectedId(int id, juce::NotificationType notification = juce::dontSendNotification) {
        selectedIndex_ = id - 1;
        repaint();
        if (notification != juce::dontSendNotification)
            notifySelection(selectedIndex_);
    }

    int getSelectedId() const { return selectedIndex_ + 1; }

    void setShowDeleteButtons(bool show) { showDelete_ = show; }
    void setPlaceholderText(const juce::String& text) { placeholderText_ = text; }

    std::function<void(int)> onItemSelected;
    std::function<void(int)> onItemDeleteRequested;
    std::function<void()> onChange;

    void paint(juce::Graphics& g) override {
        using namespace StyledDropdownConstants;

        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(getLocalBounds().toFloat(), borderRadius, borderWidth);

        paintLabel(g);
        paintChevron(g);

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
    void paintLabel(juce::Graphics& g) {
        using namespace StyledDropdownConstants;
        juce::Font font(interFont_);
        font.setHeight(fontSize);
        g.setFont(font);

        juce::String displayText = (selectedIndex_ >= 0 && selectedIndex_ < items_.size())
            ? items_[selectedIndex_] : placeholderText_;
        g.setColour(selectedIndex_ >= 0 ? juce::Colours::white : juce::Colours::white.withAlpha(0.5f));

        auto textArea = getLocalBounds().reduced(textLeftPadding, 0);
        textArea.removeFromRight(iconSize + iconRightPadding);
        g.drawText(displayText, textArea, juce::Justification::centredLeft, true);
    }

    void paintChevron(juce::Graphics& g) {
        using namespace StyledDropdownConstants;
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
