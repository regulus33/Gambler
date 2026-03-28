#pragma once

#include "BinaryData.h"
#include "ClickFlashMixin.h"
#include "Icons.h"
#include "../util/PlatformUtils.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include <functional>

namespace PresetDropdownConstants {
    constexpr float fontSize = 14.0f;
    constexpr int dropdownHeight = 24;
    constexpr int dropdownMinWidth = 120;
    constexpr int iconSize = 8;
    constexpr int iconRightPadding = 6;
    constexpr int textLeftPadding = 8;
    constexpr int popupItemHeight = 24;
    constexpr int popupMaxHeight = 300;
    constexpr int checkmarkSize = 10;
    constexpr int deleteIconSize = 10;
    constexpr int iconSpacing = 6;  // Space between delete X and checkmark
    constexpr int iconsRightPadding = 6;
    constexpr float borderWidth = 2.0f;
    constexpr float borderRadius = 4.0f;
    const juce::Colour borderColor = juce::Colours::white;
}

/**
 * PresetDropdownLookAndFeel
 * 
 * Custom look and feel for the preset dropdown with:
 * - Transparent background
 * - White Inter 12px font
 * - Checkmark icon on the right of selected item in popup
 */
class PresetDropdownLookAndFeel : public juce::LookAndFeel_V4 {
public:
    PresetDropdownLookAndFeel() {
        interFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                      int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                      juce::ComboBox& box) override
    {
        using namespace PresetDropdownConstants;
        
        // Transparent background - no fill
        
        // Draw chevron icon on the right
        auto iconBounds = juce::Rectangle<float>(
            static_cast<float>(width - iconSize - iconRightPadding),
            static_cast<float>((height - iconSize) / 2),
            static_cast<float>(iconSize),
            static_cast<float>(iconSize));
        
        auto chevronPath = Icons::createChevronDownIcon();
        Icons::strokeIcon(g, chevronPath, iconBounds, juce::Colours::white, 1.5f);
    }

    void drawComboBoxTextWhenNothingSelected(juce::Graphics& g, juce::ComboBox& box,
                                              juce::Label& label) override
    {
        using namespace PresetDropdownConstants;
        
        juce::Font font(interFont);
        font.setHeight(fontSize);
        g.setFont(font);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        
        auto textArea = label.getBounds().reduced(textLeftPadding, 0);
        g.drawText("Select Preset", textArea, juce::Justification::centredLeft, true);
    }

    juce::Font getComboBoxFont(juce::ComboBox& /*box*/) override
    {
        juce::Font font(interFont);
        font.setHeight(PresetDropdownConstants::fontSize);
        return font;
    }

    juce::Label* createComboBoxTextBox(juce::ComboBox& box) override
    {
        auto* label = new juce::Label(juce::String(), juce::String());
        label->setJustificationType(juce::Justification::centredLeft);
        label->setColour(juce::Label::textColourId, juce::Colours::white);
        return label;
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        using namespace PresetDropdownConstants;
        
        label.setBounds(textLeftPadding, 0,
                       box.getWidth() - iconSize - iconRightPadding - textLeftPadding,
                       box.getHeight());
        
        juce::Font font(interFont);
        font.setHeight(fontSize);
        label.setFont(font);
    }

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        // Dark background matching app theme
        g.fillAll(juce::Colour(0xFF1E1E1E));
        
        // Subtle border
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawRect(0, 0, width, height, 1);
    }

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override
    {
        using namespace PresetDropdownConstants;
        juce::ignoreUnused(isSeparator, hasSubMenu, shortcutKeyText, icon, textColour);

        if (isHighlighted && isActive) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRect(area);
        }

        // Draw text
        juce::Font font(interFont);
        font.setHeight(fontSize);
        g.setFont(font);
        g.setColour(isActive ? juce::Colours::white : juce::Colours::white.withAlpha(0.4f));

        auto textArea = area.reduced(textLeftPadding, 0);
        // Leave space for icons on the right (delete X and possibly checkmark)
        int iconsWidth = deleteIconSize + iconsRightPadding;
        if (isTicked) {
            iconsWidth += checkmarkSize + iconSpacing;
        }
        textArea.removeFromRight(iconsWidth);

        g.drawText(text, textArea, juce::Justification::centredLeft, true);

        // Calculate icon positions from right edge
        int rightEdge = area.getRight() - iconsRightPadding;

        if (isTicked) {
            // Delete X on far right (aligned with non-selected items)
            auto deleteBounds = juce::Rectangle<float>(
                static_cast<float>(rightEdge - deleteIconSize),
                static_cast<float>(area.getCentreY() - deleteIconSize / 2),
                static_cast<float>(deleteIconSize),
                static_cast<float>(deleteIconSize));

            auto deletePath = Icons::createCloseIcon();
            Icons::strokeIcon(g, deletePath, deleteBounds, juce::Colours::white.withAlpha(0.6f), 1.5f, true, Icons::closeIconScale);

            // Checkmark goes to the left of the delete X
            auto checkBounds = juce::Rectangle<float>(
                static_cast<float>(rightEdge - deleteIconSize - iconSpacing - checkmarkSize),
                static_cast<float>(area.getCentreY() - checkmarkSize / 2),
                static_cast<float>(checkmarkSize),
                static_cast<float>(checkmarkSize));

            auto checkPath = Icons::createCheckmarkIcon();
            Icons::strokeIcon(g, checkPath, checkBounds, juce::Colours::white, 1.5f);
        } else {
            // Delete X on far right
            auto deleteBounds = juce::Rectangle<float>(
                static_cast<float>(rightEdge - deleteIconSize),
                static_cast<float>(area.getCentreY() - deleteIconSize / 2),
                static_cast<float>(deleteIconSize),
                static_cast<float>(deleteIconSize));

            auto deletePath = Icons::createCloseIcon();
            Icons::strokeIcon(g, deletePath, deleteBounds, juce::Colours::white.withAlpha(0.6f), 1.5f, true, Icons::closeIconScale);
        }
    }

    void getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
                                    int standardMenuItemHeight, int& idealWidth, int& idealHeight) override
    {
        using namespace PresetDropdownConstants;
        juce::ignoreUnused(text, isSeparator, standardMenuItemHeight);

        juce::Font font(interFont);
        font.setHeight(fontSize);

        // Account for both delete X and checkmark (worst case)
        idealWidth = font.getStringWidth(text) + textLeftPadding * 2 +
                     deleteIconSize + iconSpacing + checkmarkSize + iconsRightPadding;
        idealHeight = popupItemHeight;
    }

private:
    juce::Typeface::Ptr interFont;
};

/**
 * PresetMenuItemComponent
 *
 * A custom component for preset menu items with clickable delete button.
 */
class PresetMenuItemComponent : public juce::PopupMenu::CustomComponent {
public:
    PresetMenuItemComponent(const juce::String& presetName, bool isSelected, bool isCurrent,
                           std::function<void()> onSelect, std::function<void()> onDelete)
        : name(presetName), selected(isSelected), current(isCurrent),
          selectCallback(std::move(onSelect)), deleteCallback(std::move(onDelete))
    {
        interFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
    }

    void paint(juce::Graphics& g) override {
        using namespace PresetDropdownConstants;

        auto area = getLocalBounds();

        // Highlight on hover (but not over delete button)
        if (isItemHighlighted() && !isMouseOverDelete) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRect(area);
        }

        // Draw text
        juce::Font font(interFont);
        font.setHeight(fontSize);
        g.setFont(font);
        g.setColour(juce::Colours::white);

        auto textArea = area.reduced(textLeftPadding, 0);
        int iconsWidth = deleteIconSize + iconsRightPadding;
        if (current) {
            iconsWidth += checkmarkSize + iconSpacing;
        }
        textArea.removeFromRight(iconsWidth);

        g.drawText(name, textArea, juce::Justification::centredLeft, true);

        // Calculate icon positions from right edge
        int rightEdge = area.getRight() - iconsRightPadding;

        if (current) {
            // Draw checkmark on far right
            auto checkBounds = juce::Rectangle<float>(
                static_cast<float>(rightEdge - checkmarkSize),
                static_cast<float>(area.getCentreY() - checkmarkSize / 2),
                static_cast<float>(checkmarkSize),
                static_cast<float>(checkmarkSize));

            auto checkPath = Icons::createCheckmarkIcon();
            Icons::strokeIcon(g, checkPath, checkBounds, juce::Colours::white, 1.5f);

            // Delete X goes to the left of checkmark
            deleteButtonBounds = juce::Rectangle<int>(
                rightEdge - checkmarkSize - iconSpacing - deleteIconSize - 2,
                area.getCentreY() - deleteIconSize / 2 - 2,
                deleteIconSize + 4,
                deleteIconSize + 4);

            auto deleteBounds = juce::Rectangle<float>(
                static_cast<float>(rightEdge - checkmarkSize - iconSpacing - deleteIconSize),
                static_cast<float>(area.getCentreY() - deleteIconSize / 2),
                static_cast<float>(deleteIconSize),
                static_cast<float>(deleteIconSize));

            auto deletePath = Icons::createCloseIcon();
            float alpha = isMouseOverDelete ? 1.0f : 0.5f;
            Icons::strokeIcon(g, deletePath, deleteBounds, juce::Colours::white.withAlpha(alpha), 1.5f, true, Icons::closeIconScale);
        } else {
            // Delete X goes where checkmark would be (far right)
            deleteButtonBounds = juce::Rectangle<int>(
                rightEdge - deleteIconSize - 2,
                area.getCentreY() - deleteIconSize / 2 - 2,
                deleteIconSize + 4,
                deleteIconSize + 4);

            auto deleteBounds = juce::Rectangle<float>(
                static_cast<float>(rightEdge - deleteIconSize),
                static_cast<float>(area.getCentreY() - deleteIconSize / 2),
                static_cast<float>(deleteIconSize),
                static_cast<float>(deleteIconSize));

            auto deletePath = Icons::createCloseIcon();
            float alpha = isMouseOverDelete ? 1.0f : 0.5f;
            Icons::strokeIcon(g, deletePath, deleteBounds, juce::Colours::white.withAlpha(alpha), 1.5f, true, Icons::closeIconScale);
        }
    }

    void mouseMove(const juce::MouseEvent& event) override {
        bool wasOverDelete = isMouseOverDelete;
        isMouseOverDelete = deleteButtonBounds.contains(event.getPosition());
        if (wasOverDelete != isMouseOverDelete) {
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent& /*event*/) override {
        if (isMouseOverDelete) {
            isMouseOverDelete = false;
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent& event) override {
        if (deleteButtonBounds.contains(event.getPosition())) {
            // Clicked on delete button - trigger the menu item to close the menu
            // then call the delete callback
            if (deleteCallback) {
                // Use a timer to call delete after menu closes
                juce::Timer::callAfterDelay(50, [callback = deleteCallback]() {
                    callback();
                });
            }
            triggerMenuItem();
        } else {
            // Clicked on the item itself - select it
            if (selectCallback) {
                selectCallback();
            }
            triggerMenuItem();
        }
    }

    void getIdealSize(int& idealWidth, int& idealHeight) override {
        using namespace PresetDropdownConstants;

        juce::Font font(interFont);
        font.setHeight(fontSize);

        idealWidth = font.getStringWidth(name) + textLeftPadding * 2 +
                     deleteIconSize + iconSpacing + checkmarkSize + iconsRightPadding + 20;
        idealHeight = popupItemHeight;
    }

private:
    juce::String name;
    bool selected;
    bool current;
    std::function<void()> selectCallback;
    std::function<void()> deleteCallback;
    juce::Typeface::Ptr interFont;
    juce::Rectangle<int> deleteButtonBounds;
    bool isMouseOverDelete = false;
};

/**
 * PresetListComponent
 *
 * A styled list of presets shown inside a CallOutBox.
 * Used instead of PopupMenu because PopupMenu is broken in iOS AUv3.
 * Matches the original PresetMenuItemComponent look: Inter font, stroke
 * checkmark icon on the selected preset, delete X on every row.
 */
class PresetListComponent : public juce::Component {
public:
    PresetListComponent(const juce::StringArray& names, int currentIndex,
                        std::function<void(int)> onSelect,
                        std::function<void(int)> onDelete,
                        std::function<void()> onDismiss = nullptr)
        : presetNames(names), selectedIndex(currentIndex),
          selectCallback(std::move(onSelect)), deleteCallback(std::move(onDelete)),
          dismissCallback(std::move(onDismiss))
    {
        using namespace PresetDropdownConstants;
        interFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        contentHeight_ = presetNames.size() * popupItemHeight;
        int visibleHeight = juce::jmin(contentHeight_, popupMaxHeight);
        setSize(200, juce::jmax(popupItemHeight, visibleHeight));
    }

    ~PresetListComponent() override {
        if (dismissCallback)
            juce::MessageManager::callAsync(dismissCallback);
    }

    void paint(juce::Graphics& g) override {
        using namespace PresetDropdownConstants;
        g.fillAll(juce::Colour(0xFF1E1E1E));

        juce::Font font(interFont);
        font.setHeight(fontSize);
        g.setFont(font);

        for (int i = 0; i < presetNames.size(); ++i) {
            int y = i * popupItemHeight + scrollOffset_;
            if (y + popupItemHeight < 0 || y > getHeight())
                continue; // skip offscreen rows

            auto row = juce::Rectangle<int>(0, y, getWidth(), popupItemHeight);

            // Hover highlight (skip if hovering over delete button or dragging)
            if (i == hoveredIndex && !isMouseOverDelete && !isDragging_) {
                g.setColour(juce::Colours::white.withAlpha(0.1f));
                g.fillRect(row);
            }

            // Layout: [padding | text ... | deleteX | gap | checkmark? | padding]
            auto area = row;
            area.removeFromLeft(textLeftPadding);
            int rightEdge = area.getRight() - iconsRightPadding;

            // Checkmark icon for the currently selected preset
            if (i == selectedIndex) {
                auto checkBounds = juce::Rectangle<float>(
                    static_cast<float>(rightEdge - checkmarkSize),
                    static_cast<float>(area.getCentreY() - checkmarkSize / 2),
                    static_cast<float>(checkmarkSize),
                    static_cast<float>(checkmarkSize));
                Icons::strokeIcon(g, Icons::createCheckmarkIcon(), checkBounds,
                                  juce::Colours::white, 1.5f);
            }

            // Delete X icon
            int deleteX = (i == selectedIndex)
                ? rightEdge - checkmarkSize - iconSpacing - deleteIconSize
                : rightEdge - deleteIconSize;

            auto deleteBounds = juce::Rectangle<float>(
                static_cast<float>(deleteX),
                static_cast<float>(area.getCentreY() - deleteIconSize / 2),
                static_cast<float>(deleteIconSize),
                static_cast<float>(deleteIconSize));

            bool hoveringThisDelete = (i == hoveredIndex && isMouseOverDelete && !isDragging_);
            float deleteAlpha = hoveringThisDelete ? 1.0f : 0.5f;
            Icons::strokeIcon(g, Icons::createCloseIcon(), deleteBounds,
                              juce::Colours::white.withAlpha(deleteAlpha), 1.5f,
                              true, Icons::closeIconScale);

            // Store delete hit area for this row (in scrolled coordinates)
            if (i < maxRows)
                deleteHitAreas[i] = juce::Rectangle<int>(
                    deleteX - 2, area.getCentreY() - deleteIconSize / 2 - 2,
                    deleteIconSize + 4, deleteIconSize + 4);

            // Preset name text
            int iconsWidth = deleteIconSize + iconsRightPadding +
                             (i == selectedIndex ? checkmarkSize + iconSpacing : 0);
            auto textArea = row.withTrimmedLeft(textLeftPadding).withTrimmedRight(iconsWidth + iconsRightPadding);
            g.setColour(juce::Colours::white);
            g.drawText(presetNames[i], textArea, juce::Justification::centredLeft, true);
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
        if (!isDragging_ && std::abs(delta) > dragThreshold)
            isDragging_ = true;

        if (isDragging_) {
            scrollOffset_ = dragStartScrollOffset_ + delta;
            clampScroll();
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        using namespace PresetDropdownConstants;
        if (isDragging_) {
            isDragging_ = false;
            return;
        }

        int index = (e.y - scrollOffset_) / popupItemHeight;
        if (index < 0 || index >= presetNames.size())
            return;

        if (index < maxRows && deleteHitAreas[index].contains(e.getPosition())) {
            auto cb = deleteCallback;
            if (auto* callout = findParentComponentOfClass<juce::CallOutBox>())
                callout->dismiss();
            if (cb)
                juce::Timer::callAfterDelay(50, [cb, index]() { cb(index); });
        } else {
            if (selectCallback)
                selectCallback(index);
            if (auto* callout = findParentComponentOfClass<juce::CallOutBox>())
                callout->dismiss();
        }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        using namespace PresetDropdownConstants;
        int newHover = (e.y - scrollOffset_) / popupItemHeight;
        bool overDelete = false;
        if (newHover >= 0 && newHover < presetNames.size() && newHover < maxRows)
            overDelete = deleteHitAreas[newHover].contains(e.getPosition());

        if (newHover != hoveredIndex || overDelete != isMouseOverDelete) {
            hoveredIndex = newHover;
            isMouseOverDelete = overDelete;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override {
        hoveredIndex = -1;
        isMouseOverDelete = false;
        repaint();
    }

private:
    void clampScroll() {
        int maxScroll = 0;
        int minScroll = juce::jmin(0, getHeight() - contentHeight_);
        scrollOffset_ = juce::jlimit(minScroll, maxScroll, scrollOffset_);
    }

    static constexpr int maxRows = 64;
    static constexpr int dragThreshold = 8;

    juce::StringArray presetNames;
    int selectedIndex = -1;
    int hoveredIndex = -1;
    bool isMouseOverDelete = false;
    std::function<void(int)> selectCallback;
    std::function<void(int)> deleteCallback;
    std::function<void()> dismissCallback;
    juce::Typeface::Ptr interFont;
    juce::Rectangle<int> deleteHitAreas[maxRows];

    int contentHeight_ = 0;
    int scrollOffset_ = 0;
    int dragStartY_ = 0;
    int dragStartScrollOffset_ = 0;
    bool isDragging_ = false;
};

/**
 * PresetDropdownComponent
 *
 * A custom dropdown for selecting presets with:
 * - Transparent background
 * - White Inter 12px font
 * - Chevron icon on the right
 * - Checkmark icon for selected item in popup
 * - Delete X icon for each item
 *
 * Auto-loads presets when selected from the dropdown.
 */
class PresetDropdownComponent : public juce::Component {
public:
    PresetDropdownComponent() {
        interFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
    }

    void paint(juce::Graphics& g) override {
        using namespace PresetDropdownConstants;

        // Draw border around the dropdown with rounded corners
        g.setColour(borderColor);
        g.drawRoundedRectangle(getLocalBounds().toFloat(), borderRadius, borderWidth);

        // Draw current selection text
        juce::Font font(interFont);
        font.setHeight(fontSize);
        g.setFont(font);

        juce::String displayText = currentPresetName.isEmpty() ? "Select Preset" : currentPresetName;
        g.setColour(currentPresetName.isEmpty() ? juce::Colours::white.withAlpha(0.5f) : juce::Colours::white);

        auto textArea = getLocalBounds().reduced(textLeftPadding, 0);
        textArea.removeFromRight(iconSize + iconRightPadding);
        g.drawText(displayText, textArea, juce::Justification::centredLeft, true);

        // Draw chevron icon on the right (rotates when popup is open)
        {
            float cx = static_cast<float>(getWidth() - iconSize - iconRightPadding) + static_cast<float>(iconSize) * 0.5f;
            float cy = static_cast<float>(getHeight()) * 0.5f;
            auto iconBounds = juce::Rectangle<float>(
                cx - static_cast<float>(iconSize) * 0.5f,
                cy - static_cast<float>(iconSize) * 0.5f,
                static_cast<float>(iconSize),
                static_cast<float>(iconSize));

            auto chevronPath = Icons::createChevronDownIcon();

            // Apply rotation around center
            if (std::abs(chevronAngle_) > 0.001f) {
                g.saveState();
                g.addTransform(juce::AffineTransform::rotation(chevronAngle_, cx, cy));
            }

            Icons::strokeIcon(g, chevronPath, iconBounds, juce::Colours::white, 1.5f);

            if (std::abs(chevronAngle_) > 0.001f)
                g.restoreState();
        }

        // Draw click flash overlay
        clickFlash_.paint(g, getLocalBounds());
    }

    void mouseDown(const juce::MouseEvent& /*event*/) override {
        clickFlash_.trigger();
        setPopupOpen(true);
        showPopupMenu();
    }

    void parentHierarchyChanged() override {
        if (getTopLevelComponent() != nullptr) {
            clickFlash_.attachTo(this);
            ensureVBlank();
        }
    }

    void mouseEnter(const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    /**
     * Update the list of presets in the dropdown
     */
    void setPresets(const juce::StringArray& names) {
        presetNames = names;
    }

    /**
     * Select a preset by index (0-based)
     */
    void selectPresetByIndex(int index, bool sendNotification = false) {
        if (index >= 0 && index < presetNames.size()) {
            currentPresetName = presetNames[index];
            selectedIndex = index;
            repaint();

            if (sendNotification && onPresetSelected) {
                onPresetSelected(index);
            }
        }
    }

    /**
     * Select a preset by name
     */
    void selectPresetByName(const juce::String& name, bool sendNotification = false) {
        int index = presetNames.indexOf(name);
        if (index >= 0) {
            selectPresetByIndex(index, sendNotification);
        }
    }

    /**
     * Clear the selection
     */
    void clearSelection() {
        currentPresetName = "";
        selectedIndex = -1;
        repaint();
    }

    /**
     * Callback when a preset is selected
     * Parameter: 0-based index of the selected preset
     */
    std::function<void(int)> onPresetSelected;

    /**
     * Callback when a preset delete is requested
     * Parameter: 0-based index of the preset to delete
     */
    std::function<void(int)> onPresetDeleteRequested;

    /**
     * Set the parent component for popup menus (needed for iOS AUv3)
     */
    void setPopupParent(juce::Component* parent) {
        popupParent = parent;
    }

private:
    void showPopupMenu() {
        if (PlatformUtils::isIOS()) {
            showPopupMenuIOS();
        } else {
            showPopupMenuDesktop();
        }
    }

    void showPopupMenuDesktop() {
        using namespace PresetDropdownConstants;

        juce::PopupMenu menu;
        menu.setLookAndFeel(&menuLookAndFeel);

        for (int i = 0; i < presetNames.size(); ++i) {
            bool isCurrent = (i == selectedIndex);
            int capturedIndex = i;

            auto item = std::make_unique<PresetMenuItemComponent>(
                presetNames[i],
                false,
                isCurrent,
                [this, capturedIndex]() {
                    selectPresetByIndex(capturedIndex, false);
                    if (onPresetSelected)
                        onPresetSelected(capturedIndex);
                },
                [this, capturedIndex]() {
                    if (onPresetDeleteRequested)
                        onPresetDeleteRequested(capturedIndex);
                }
            );

            menu.addCustomItem(-1, std::move(item), nullptr, presetNames[i]);
        }

        if (presetNames.isEmpty())
            menu.addItem(1, "No Presets", false);

        auto options = juce::PopupMenu::Options()
            .withTargetComponent(this)
            .withMinimumWidth(getWidth())
            .withMaximumNumColumns(1);

        menu.showMenuAsync(options, [this](int) { setPopupOpen(false); });
    }

    void showPopupMenuIOS() {
        auto* listBox = new PresetListComponent(presetNames, selectedIndex,
            [this](int index) {
                selectPresetByIndex(index, false);
                if (onPresetSelected)
                    onPresetSelected(index);
            },
            [this](int index) {
                if (onPresetDeleteRequested)
                    onPresetDeleteRequested(index);
            },
            [this]() { setPopupOpen(false); });

        juce::CallOutBox::launchAsynchronously(
            std::unique_ptr<juce::Component>(listBox),
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

        // Animate chevron rotation with framerate-independent easing
        float diff = chevronTargetAngle_ - chevronAngle_;
        if (std::abs(diff) > 0.01f) {
            chevronAngle_ += diff * 12.0f * dt; // ~0.2 per frame at 60fps
            repaint();
        } else {
            chevronAngle_ = chevronTargetAngle_;
            needsChevronAnimation_ = false;
            repaint();
        }
    }

    juce::StringArray presetNames;
    juce::String currentPresetName;
    int selectedIndex = -1;
    juce::Typeface::Ptr interFont;
    juce::Component* popupParent = nullptr;
    PresetDropdownLookAndFeel menuLookAndFeel;
    ClickFlash clickFlash_;
    float chevronAngle_ = 0.0f;
    float chevronTargetAngle_ = 0.0f;
    bool needsChevronAnimation_ = false;
    double lastVBlankTimestamp_ = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vBlank_;
};

