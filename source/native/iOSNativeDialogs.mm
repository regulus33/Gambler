#include "iOSNativeDialogs.h"

#if JUCE_IOS

#import <UIKit/UIKit.h>

namespace {
    UIViewController* findViewController(juce::Component* component) {
        if (component == nullptr)
            return nil;

        UIView* view = (UIView*) component->getWindowHandle();
        if (view == nil)
            return nil;

        UIResponder* responder = view;
        while (responder != nil) {
            if ([responder isKindOfClass:[UIViewController class]])
                return (UIViewController*) responder;
            responder = [responder nextResponder];
        }
        return nil;
    }
}

void iOSNativeDialogs::showTextInputAlert(juce::Component* parent,
                                           const juce::String& title,
                                           const juce::String& placeholder,
                                           const juce::String& defaultText,
                                           std::function<void(juce::String)> onSave)
{
    UIViewController* vc = findViewController(parent);
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

void iOSNativeDialogs::shareFile(juce::Component* parent,
                                  const juce::File& file,
                                  std::function<void()> onDone)
{
    UIViewController* vc = findViewController(parent);
    if (vc == nil)
        return;

    NSURL* fileURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:file.getFullPathName().toRawUTF8()]];
    NSArray* items = @[fileURL];

    UIActivityViewController* avc = [[UIActivityViewController alloc] initWithActivityItems:items
                                                                      applicationActivities:nil];

    // Present as form sheet (no popover arrow)
    avc.modalPresentationStyle = UIModalPresentationFormSheet;

    auto doneCallback = std::make_shared<std::function<void()>>(std::move(onDone));
    avc.completionWithItemsHandler = ^(UIActivityType, BOOL, NSArray*, NSError*) {
        juce::MessageManager::callAsync([doneCallback]() {
            if (*doneCallback)
                (*doneCallback)();
        });
    };

    [vc presentViewController:avc animated:YES completion:nil];
}

#endif
