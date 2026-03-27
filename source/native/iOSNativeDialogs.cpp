#include "iOSNativeDialogs.h"

#if ! JUCE_IOS

// Stub for non-iOS builds — native dialogs are only available on iOS.
void iOSNativeDialogs::showTextInputAlert(juce::Component* /*parent*/,
                                           const juce::String& /*title*/,
                                           const juce::String& /*placeholder*/,
                                           const juce::String& /*defaultText*/,
                                           std::function<void(juce::String)> /*onSave*/)
{
}

#endif
