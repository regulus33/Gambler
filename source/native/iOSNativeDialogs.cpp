#include "iOSNativeDialogs.h"

#if ! JUCE_IOS

// Stubs for non-iOS builds — native dialogs are only available on iOS.
void iOSNativeDialogs::showTextInputAlert(juce::Component* /*parent*/,
                                           const juce::String& /*title*/,
                                           const juce::String& /*placeholder*/,
                                           const juce::String& /*defaultText*/,
                                           std::function<void(juce::String)> /*onSave*/)
{
}

void iOSNativeDialogs::shareFile(juce::Component* /*parent*/,
                                  const juce::File& /*file*/,
                                  std::function<void()> /*onDone*/)
{
}

#endif
