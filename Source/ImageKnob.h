#pragma once
#include <JuceHeader.h>

// LookAndFeel personnalisé pour knob image rotatif, sans aucun dessin JUCE par défaut
class ImageKnobLookAndFeel : public juce::LookAndFeel_V4 {
public:
    ImageKnobLookAndFeel(const juce::Image& knobImage);
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height, float sliderPosProportional,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    static juce::ReferenceCountedObjectPtr<juce::Typeface> upheavttTypeface;
    static juce::Font getUpheavttFont(float size);
    void setMidQSliderPointer(const juce::Slider* ptr) { midQSliderPointer = ptr; }

private:
    juce::Image knobImg;
    const juce::Slider* midQSliderPointer = nullptr;
};
