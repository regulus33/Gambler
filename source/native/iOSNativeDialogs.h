#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * Native iOS dialog helpers.
 * Presents UIAlertController from the JUCE component's UIViewController.
 */
struct iOSNativeDialogs {
    /**
     * Shows a native iOS text input alert.
     * @param parent       JUCE component to present from (required for AUv3)
     * @param title        Alert title
     * @param placeholder  Text field placeholder
     * @param defaultText  Pre-filled text
     * @param onSave       Called with the entered text when Save is tapped
     */
    static void showTextInputAlert(juce::Component* parent,
                                   const juce::String& title,
                                   const juce::String& placeholder,
                                   const juce::String& defaultText,
                                   std::function<void(juce::String)> onSave);
};
