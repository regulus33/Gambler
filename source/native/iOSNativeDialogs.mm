#include "iOSNativeDialogs.h"

#if JUCE_IOS

#import <UIKit/UIKit.h>

void iOSNativeDialogs::showTextInputAlert(juce::Component* parent,
                                           const juce::String& title,
                                           const juce::String& placeholder,
                                           const juce::String& defaultText,
                                           std::function<void(juce::String)> onSave)
{
    if (parent == nullptr)
        return;

    // Get the UIViewController from the JUCE component's peer
    UIView* view = (UIView*) parent->getWindowHandle();
    if (view == nil)
        return;

    UIViewController* vc = nil;
    UIResponder* responder = view;
    while (responder != nil) {
        if ([responder isKindOfClass:[UIViewController class]]) {
            vc = (UIViewController*) responder;
            break;
        }
        responder = [responder nextResponder];
    }

    if (vc == nil)
        return;

    NSString* nsTitle = title.toRawUTF8() ? [NSString stringWithUTF8String:title.toRawUTF8()] : @"";
    NSString* nsPlaceholder = placeholder.toRawUTF8() ? [NSString stringWithUTF8String:placeholder.toRawUTF8()] : @"";
    NSString* nsDefault = defaultText.toRawUTF8() ? [NSString stringWithUTF8String:defaultText.toRawUTF8()] : @"";

    UIAlertController* alert = [UIAlertController alertControllerWithTitle:nsTitle
                                                                  message:nil
                                                           preferredStyle:UIAlertControllerStyleAlert];

    [alert addTextFieldWithConfigurationHandler:^(UITextField* textField) {
        textField.placeholder = nsPlaceholder;
        textField.text = nsDefault;
        textField.clearButtonMode = UITextFieldViewModeWhileEditing;
        // Select all text so user can easily replace
        dispatch_async(dispatch_get_main_queue(), ^{
            [textField selectAll:nil];
        });
    }];

    auto savedCallback = std::make_shared<std::function<void(juce::String)>>(std::move(onSave));

    [alert addAction:[UIAlertAction actionWithTitle:@"Cancel"
                                              style:UIAlertActionStyleCancel
                                            handler:nil]];

    [alert addAction:[UIAlertAction actionWithTitle:@"Save"
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction* action) {
        NSString* text = alert.textFields.firstObject.text;
        juce::String result = text ? juce::String::fromUTF8([text UTF8String]) : juce::String();
        juce::MessageManager::callAsync([savedCallback, result]() {
            if (*savedCallback)
                (*savedCallback)(result);
        });
    }]];

    [vc presentViewController:alert animated:YES completion:nil];
}

#endif
