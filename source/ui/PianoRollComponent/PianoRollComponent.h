#pragma once

#include "PianoRollConstants.h"
#include "KeyData.h"
#include "PianoKeyPaths.h"
#include "../../backend/Scale.h"
#include "../../backend/MidiLearnRegistry/MidiLearnRegistry.h"
#include "BinaryData.h"
#include "../MidiLearnOverlayPainter.h"
#include "../StateListenerComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include <map>

/**
 * PianoRollComponent - Piano keyboard for scale selection
 *
 * Displays 12 keys representing the chromatic scale using custom SVG paths.
 * Users toggle notes on/off to define the scale. Total size: 422 x 206.
 *
 * Observer Pattern: listens to all scale parameters (12 notes + range).
 */
class PianoRollComponent : public StateListenerComponent {
public:
    explicit PianoRollComponent(PluginProcessor& processor)
        : StateListenerComponent(processor) {
        interFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        createKeyData();
        initMidiControlIds();
        setupKeyPaths();

        listenToGlobalScaleNotes();
        listenToScaleRange();
        listenToInternalState();

        setSize (PianoRollConstants::componentWidth, PianoRollConstants::componentHeight);
        triggerAsyncUpdate();
    }

    ~PianoRollComponent() override {
        keys.clear (true);
    }

    void paint (juce::Graphics& g) override {
        g.fillAll (juce::Colour (0xFF2B2B2B));

        for (int i = 0; i < 12; i++)
            if (keys[i]->isBlack)
                drawKey (g, *keys[i], i);

        for (int i = 0; i < 12; i++)
            if (!keys[i]->isBlack)
                drawKey (g, *keys[i], i);

        paintMidiLearnOverlays(g);

        displayColoursInitialised_ = true;
    }

    void mouseDown (const juce::MouseEvent& event) override {
        lastDragKey_ = -1;
        int keyIndex = findKeyAtPosition(event.position);
        if (keyIndex < 0)
            return;

        if (handleMidiLearn(keyIndex))
            return;

        lastDragKey_ = keyIndex;
        activateKeyForTouch(keyIndex, event.source.getIndex());
    }

    void mouseUp (const juce::MouseEvent& event) override {
        if (!isScaleLatchEnabled()) {
            auto it = pressedKeys_.find(event.source.getIndex());
            if (it != pressedKeys_.end()) {
                int keyIndex = it->second;
                pressedKeys_.erase(it);
                if (trySetKey(keyIndex, false)) {
                    repaint();
                    if (onNoteChanged)
                        onNoteChanged();
                }
            }
        }
        lastDragKey_ = -1;
    }

    void mouseDrag (const juce::MouseEvent& event) override {
        int hitKey = findKeyAtPosition(event.position);
        if (hitKey < 0 || hitKey == lastDragKey_)
            return;
        lastDragKey_ = hitKey;

        if (isScaleLatchEnabled()) {
            if (tryToggleKey(hitKey)) {
                repaint();
                if (onNoteChanged)
                    onNoteChanged();
            }
        } else {
            releasePreviousTouchKey(event.source.getIndex());
            pressedKeys_[event.source.getIndex()] = hitKey;
            if (trySetKey(hitKey, true)) {
                repaint();
                if (onNoteChanged)
                    onNoteChanged();
            }
        }
    }

    void resized() override {}

    void parentHierarchyChanged() override {
        if (getTopLevelComponent() != nullptr && vBlankAttachment_ == nullptr) {
            vBlankAttachment_ = std::make_unique<juce::VBlankAttachment>(
                this, [this](double timestampSec) { onVBlank(timestampSec); });
        }
    }

    void setScale (const Scale& scale) {
        auto scaleNotes = scale.getScaleNotes();
        for (int i = 0; i < 12; i++)
            keys[i]->isToggled = (scaleNotes[i] != 0);
        repaint();
    }

    Scale getScale() const {
        std::array<int, 12> scaleNotes{};
        for (int i = 0; i < 12; i++)
            scaleNotes[i] = keys[i]->isToggled ? 1 : 0;
        Scale scale;
        scale.setScaleNotes (scaleNotes);
        return scale;
    }

    void setActiveNote (int noteIndex) {
        if (activeNoteIndex != noteIndex) {
            activeNoteIndex = noteIndex;
            for (int i = 0; i < 12; i++)
                activeCircleTargetAlpha_[i] = (i == activeNoteIndex) ? 1.0f : 0.0f;
            needsAnimationRepaint_ = true;
            repaint();
        }
    }

    int getActiveNote() const { return activeNoteIndex; }

    juce::Array<int> getSelectedNoteIndices() const {
        juce::Array<int> selected;
        for (int i = 0; i < 12; i++)
            if (keys[i]->isToggled)
                selected.add(i);
        return selected;
    }

    juce::Array<juce::String> getSelectedNoteNames() const {
        juce::Array<juce::String> selected;
        for (int i = 0; i < 12; i++)
            if (keys[i]->isToggled)
                selected.add(keys[i]->name);
        return selected;
    }

    juce::Array<juce::Colour> getSelectedNoteColours() const {
        juce::Array<juce::Colour> selected;
        for (int i = 0; i < 12; i++)
            if (keys[i]->isToggled)
                selected.add(keys[i]->color);
        return selected;
    }

    bool TEST_simulateKeyClick(int keyIndex) {
        if (keyIndex < 0 || keyIndex >= 12) return false;
        bool wasToggled = tryToggleKey(keyIndex);
        if (wasToggled) {
            repaint();
            if (onNoteChanged) onNoteChanged();
        }
        return wasToggled;
    }

    const KeyData* getKeyData(int keyIndex) const {
        if (keyIndex >= 0 && keyIndex < 12)
            return keys[keyIndex];
        return nullptr;
    }

    std::function<void()> onNoteChanged;

    void setRegistry(MidiLearnRegistry* registry) { registry_ = registry; }

protected:
    void onStateChanged() override {
        auto& state = getState();
        Scale scale = state.getGlobalScale();
        auto scaleNotes = scale.getScaleNotes();
        for (int i = 0; i < 12; i++) {
            keys[i]->isToggled = (scaleNotes[i] != 0);
            keys[i]->color = state.getNoteColour(i);
        }
        repaint();
    }

private:
    void createKeyData() {
        struct KeyInfo { const char* name; juce::Colour color; bool isBlack; };
        const KeyInfo keyInfos[12] = {
            { "C",  juce::Colour (0xFFFF6B6B), false },
            { "Db", juce::Colour (0xFFFF9F6B), true  },
            { "D",  juce::Colour (0xFFFFD66B), false },
            { "Eb", juce::Colour (0xFFFFE66B), true  },
            { "E",  juce::Colour (0xFFE6FF6B), false },
            { "F",  juce::Colour (0xFF6BFFB4), false },
            { "Gb", juce::Colour (0xFF6BFFD6), true  },
            { "G",  juce::Colour (0xFF6BD6FF), false },
            { "Ab", juce::Colour (0xFF6BB4FF), true  },
            { "A",  juce::Colour (0xFF9F6BFF), false },
            { "Bb", juce::Colour (0xFFD66BFF), true  },
            { "B",  juce::Colour (0xFFFF6BD6), false }
        };
        for (int i = 0; i < 12; i++)
            keys.add (new KeyData (keyInfos[i].name, keyInfos[i].color, keyInfos[i].isBlack));
    }

    void initMidiControlIds() {
        for (int i = 0; i < 12; i++) {
            keyMidiControlIds_[i] = static_cast<MidiControlId>(
                static_cast<int>(MidiControlId::SCALE_NOTE_C) + i);
        }
    }

    int findKeyAtPosition(juce::Point<float> position) const {
        for (int i = 0; i < 12; i++)
            if (!keys[i]->isBlack && keys[i]->path.contains (position))
                return i;
        for (int i = 0; i < 12; i++)
            if (keys[i]->isBlack && keys[i]->path.contains (position))
                return i;
        return -1;
    }

    bool handleMidiLearn(int keyIndex) {
        if (registry_ && registry_->isLearning() && keyMidiControlIds_[keyIndex] != MidiControlId::COUNT) {
            registry_->setLearningControl(keyMidiControlIds_[keyIndex]);
            repaint();
            return true;
        }
        return false;
    }

    void activateKeyForTouch(int keyIndex, int touchSourceIndex) {
        if (isScaleLatchEnabled()) {
            if (tryToggleKey(keyIndex)) {
                repaint();
                if (onNoteChanged) onNoteChanged();
            }
        } else {
            pressedKeys_[touchSourceIndex] = keyIndex;
            if (trySetKey(keyIndex, true)) {
                repaint();
                if (onNoteChanged) onNoteChanged();
            }
        }
    }

    void releasePreviousTouchKey(int touchSourceIndex) {
        auto it = pressedKeys_.find(touchSourceIndex);
        if (it == pressedKeys_.end())
            return;
        int prevKey = it->second;
        if (trySetKey(prevKey, false)) {
            repaint();
            if (onNoteChanged) onNoteChanged();
        }
    }

    void paintMidiLearnOverlays(juce::Graphics& g) {
        if (!(registry_ && registry_->isLearning()))
            return;

        int activeOverlayIndex = -1;
        for (int i = 0; i < 12; i++) {
            if (registry_->getLearningControl() == keyMidiControlIds_[i]) {
                activeOverlayIndex = i;
                continue;
            }
            paintMidiLearnOverlayForKey(g, keys[i]->path.getBounds(), i);
        }
        if (activeOverlayIndex >= 0)
            paintMidiLearnOverlayForKey(g, keys[activeOverlayIndex]->path.getBounds(), activeOverlayIndex);
    }

    void onVBlank(double timestampSec) {
        double delta = (lastVBlankTimestamp_ > 0.0)
            ? (timestampSec - lastVBlankTimestamp_)
            : (1.0 / 60.0);
        lastVBlankTimestamp_ = timestampSec;
        frameDeltaSec_ = static_cast<float>(juce::jlimit(0.001, 0.1, delta));

        if (animateActiveCircles())
            needsAnimationRepaint_ = true;

        if (needsAnimationRepaint_) {
            needsAnimationRepaint_ = false;
            repaint();
        }
    }

    bool animateActiveCircles() {
        bool animating = false;
        for (int i = 0; i < 12; i++) {
            float& alpha = activeCircleAlpha_[i];
            float target = activeCircleTargetAlpha_[i];
            if (std::abs(alpha - target) > 0.001f) {
                float duration = (target > alpha)
                    ? PianoRollConstants::activeCircleFadeInDuration
                    : PianoRollConstants::activeCircleFadeOutDuration;
                float step = frameDeltaSec_ / duration;
                alpha = (target > alpha)
                    ? juce::jmin(alpha + step, target)
                    : juce::jmax(alpha - step, target);
                animating = true;
            }
        }
        return animating;
    }

    int countToggledNotes() const {
        int count = 0;
        for (int i = 0; i < 12; i++)
            if (keys[i]->isToggled) count++;
        return count;
    }

    bool tryToggleKey(int keyIndex) {
        keys[keyIndex]->isToggled = !keys[keyIndex]->isToggled;
        return true;
    }

    bool trySetKey(int keyIndex, bool enabled) {
        if (keys[keyIndex]->isToggled != enabled) {
            keys[keyIndex]->isToggled = enabled;
            return true;
        }
        return false;
    }

    bool isScaleLatchEnabled() const {
        if (auto* param = getProcessor().getAPVTS().getParameter(Params::Keys::ID::SCALE_LATCH))
            return param->getValue() > 0.5f;
        return false;
    }

    void drawKey (juce::Graphics& g, const KeyData& key, int keyIndex) {
        using namespace PianoRollConstants;

        juce::Colour targetColor = key.isToggled
            ? key.color
            : (key.isBlack ? unselectedBlackKeyColor : unselectedWhiteKeyColor);

        if (!displayColoursInitialised_) {
            displayColours_[keyIndex] = targetColor;
        } else if (displayColours_[keyIndex] != targetColor) {
            float t = juce::jmin(1.0f, colourFadeSpeed * frameDeltaSec_);
            displayColours_[keyIndex] = displayColours_[keyIndex].interpolatedWith(targetColor, t);
            if (displayColours_[keyIndex] != targetColor)
                needsAnimationRepaint_ = true;
        }

        g.setColour (displayColours_[keyIndex]);
        g.fillPath (key.path);

        g.setColour (juce::Colours::black);
        g.strokePath (key.path, juce::PathStrokeType (keyBorderWidth));

        drawKeyLabel(g, key, keyIndex);
    }

    void drawKeyLabel(juce::Graphics& g, const KeyData& key, int keyIndex) {
        using namespace PianoRollConstants;

        auto pathBounds = key.path.getBounds();
        auto textArea = pathBounds.withTrimmedTop (pathBounds.getHeight() * 0.7f);

        float scaledFontSize = baseFontSize * derivedScale_ * noteTextScale;
        float scaledCircleRadius = baseActiveCircleRadius * derivedScale_ * noteTextScale;
        float circleAlpha = activeCircleAlpha_[keyIndex];

        if (circleAlpha > 0.0f) {
            auto circleCentre = textArea.getCentre();
            g.setColour (juce::Colours::black.withAlpha(circleAlpha));
            g.fillEllipse (circleCentre.x - scaledCircleRadius, circleCentre.y - scaledCircleRadius,
                          scaledCircleRadius * 2.0f, scaledCircleRadius * 2.0f);
        }

        auto textColour = juce::Colours::black.interpolatedWith(juce::Colours::white, circleAlpha);
        g.setColour (textColour);
        juce::Font font (interFont);
        font.setHeight (scaledFontSize);
        if (circleAlpha > 0.5f)
            font.setBold (true);
        g.setFont (font);

        g.drawText (key.name, textArea.toNearestInt(), juce::Justification::centred);
    }

    void setupKeyPaths() {
        using namespace PianoRollConstants;

        float xPos = 0.0f;
        const float cWidth = 57.1f - 0.5f;
        const float dWidth = 60.7f - 0.5f;
        const float eWidth = 57.1f - 0.5f;
        const float fWidth = 57.1f - 0.5f;
        const float gWidth = 60.7f - 0.5f;
        const float aWidth = 60.7f - 0.5f;

        keys[0]->path = PianoKeyPaths::createWhiteKeyRightNotch (xPos);
        xPos += cWidth;

        keys[1]->path = PianoKeyPaths::createBlackKey(
            xPos - (cWidth / 2.0f) - (blackKeyWidth / 2.0f) + blackKeyShift);

        keys[2]->path = PianoKeyPaths::createWhiteKeyBothNotches (xPos);
        xPos += dWidth;

        keys[3]->path = PianoKeyPaths::createBlackKey(
            xPos - (dWidth / 2.0f) - (blackKeyWidth / 2.0f) + blackKeyShift);

        keys[4]->path = PianoKeyPaths::createWhiteKeyLeftNotch (xPos);
        xPos += eWidth;

        keys[5]->path = PianoKeyPaths::createWhiteKeyRightNotch (xPos);
        xPos += fWidth;

        keys[6]->path = PianoKeyPaths::createBlackKey(
            xPos - (fWidth / 2.0f) - (blackKeyWidth / 2.0f) + blackKeyShift);

        keys[7]->path = PianoKeyPaths::createWhiteKeyBothNotches (xPos);
        xPos += gWidth;

        keys[8]->path = PianoKeyPaths::createBlackKey(
            xPos - (gWidth / 2.0f) - (blackKeyWidth / 2.0f) + blackKeyShift);

        keys[9]->path = PianoKeyPaths::createWhiteKeyBothNotches (xPos);
        xPos += aWidth;

        keys[10]->path = PianoKeyPaths::createBlackKey(
            xPos - (aWidth / 2.0f) - (blackKeyWidth / 2.0f) + blackKeyShift);

        keys[11]->path = PianoKeyPaths::createWhiteKeyLeftNotch (xPos);

        float naturalWidth = 0.0f;
        for (int i = 0; i < 12; i++)
            naturalWidth = juce::jmax(naturalWidth, keys[i]->path.getBounds().getRight());

        derivedScale_ = static_cast<float>(componentWidth) / naturalWidth;

        auto scaleTransform = juce::AffineTransform::scale(derivedScale_);
        for (int i = 0; i < 12; i++)
            keys[i]->path.applyTransform(scaleTransform);
    }

    void paintMidiLearnOverlayForKey(juce::Graphics& g, juce::Rectangle<float> keyBounds, int keyIndex) {
        MidiLearnOverlayPainter::paint(g, keyBounds, registry_, keyMidiControlIds_[keyIndex], 0.0f, 0.3f);
    }

    juce::OwnedArray<KeyData> keys;
    juce::Typeface::Ptr interFont;
    float derivedScale_ = 1.0f;
    int activeNoteIndex = -1;

    MidiLearnRegistry* registry_ = nullptr;
    std::array<MidiControlId, 12> keyMidiControlIds_{};

    std::map<int, int> pressedKeys_;
    int lastDragKey_ = -1;

    std::array<float, 12> activeCircleAlpha_ = {};
    std::array<float, 12> activeCircleTargetAlpha_ = {};

    std::array<juce::Colour, 12> displayColours_;
    bool displayColoursInitialised_ = false;
    bool needsAnimationRepaint_ = false;
    float frameDeltaSec_ = 1.0f / 60.0f;
    double lastVBlankTimestamp_ = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vBlankAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollComponent)
};
