#include "ImageKnob.h"
#include "JuceHeader.h"
#include "BinaryData.h"

juce::ReferenceCountedObjectPtr<juce::Typeface> ImageKnobLookAndFeel::upheavttTypeface;

juce::Font ImageKnobLookAndFeel::getUpheavttFont(float size)
{
    if (upheavttTypeface == nullptr)
        upheavttTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::upheavtt_ttf, BinaryData::upheavtt_ttfSize);
    juce::Font font(*upheavttTypeface);
    font.setHeight(size);
    return font;
}

ImageKnobLookAndFeel::ImageKnobLookAndFeel(const juce::Image& knobImage)
    : knobImg(knobImage) {}

void ImageKnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional,
                                            float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, juce::Slider& slider)
{
    const float scale = 1.61f; // facteur de zoom (>1.0 = image plus grande)
    const int imgSize = juce::jmin(width, height);
    const float cx = x + width * 0.5f;
    const float cy = y + height * 0.5f;
    const float drawSize = imgSize * scale;
    const float imgX = cx - drawSize * 0.5f;
    const float imgY = cy - drawSize * 0.5f;
    constexpr float startAngle = juce::degreesToRadians(0.0f);
    constexpr float endAngle   = juce::degreesToRadians(270.0f);
    float angle = startAngle + sliderPosProportional * (endAngle - startAngle);

    // Dessin du knob image
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(angle, cx, cy));
    g.drawImage(knobImg, imgX, imgY, drawSize, drawSize, 0, 0, knobImg.getWidth(), knobImg.getHeight());
    g.restoreState();

    // Affichage de la valeur (si demandé par le contexte du slider)
    // Affiche la valeur centrée SOUS le centre du knob, pour tous les sliders SAUF Mid Q
    if (slider.getName() != "Mid Q" && (slider.isMouseOverOrDragging() || slider.isMouseButtonDown() || slider.hasKeyboardFocus(true))) {
        juce::String valueText;
        if (slider.getTextValueSuffix().isNotEmpty())
            valueText = juce::String(slider.getValue(), 1) + " " + slider.getTextValueSuffix();
        else
            valueText = juce::String(slider.getValue(), 1);
        g.setColour(juce::Colours::white);
        g.setFont(getUpheavttFont(14.0f));
        int textHeight = 18;
        int yOffset = height / 2 + 15; // 15px sous le centre du knob
        g.drawFittedText(valueText, x, y + yOffset, width, textHeight, juce::Justification::centred, 1);
    }
    // Affichage spécifique pour le knob Q si midQSliderPointer est défini
    else if (midQSliderPointer && &slider == midQSliderPointer && (slider.isMouseOverOrDragging() || slider.isMouseButtonDown() || slider.hasKeyboardFocus(true))) {
        juce::String valueText = juce::String(slider.getValue(), 2);
        g.setColour(juce::Colours::white);
        g.setFont(getUpheavttFont(14.0f));
        int textHeight = 18;
        int yOffset = height / 2 + 15 - 10; // Décalage vers le haut de 10px
        g.drawFittedText(valueText, x, y + yOffset, width, textHeight, juce::Justification::centred, 1);
    }
}
