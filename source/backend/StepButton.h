#pragma once

/**
 * Represents a single step button in the sequence grid.
 * Each button can be in multiple states: selected and/or highlighted.
 */
class StepButton {
public:
    StepButton() = default;
    
    // State getters
    bool isSelected() const { return selected; }
    bool isHighlighted() const { return highlighted; }
    
    // State setters
    void setSelected(bool value) { selected = value; }
    void setHighlighted(bool value) { highlighted = value; }
    
    // Toggle methods
    void toggleSelected() { selected = !selected; }
    void toggleHighlighted() { highlighted = !highlighted; }
    
    // Reset to default state
    void reset() {
        selected = false;
        highlighted = false;
    }
    
private:
    bool selected = false;
    bool highlighted = false;
};

