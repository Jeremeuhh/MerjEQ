/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ============================================================================== 
*/

#pragma once
#include <JuceHeader.h>
#include <array>

class MerjEQAudioProcessor;

class MerjEQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    MerjEQAudioProcessorEditor(MerjEQAudioProcessor&);
    ~MerjEQAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MerjEQAudioProcessor& processor;
    std::unique_ptr<juce::Image> backgroundImage;

    std::array<juce::Slider, 4> sliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> attachments;    juce::String gainPopupText;
    int gainPopupSlider = -1;
    std::unique_ptr<juce::Timer> gainPopupTimer;

    // Bouton de distorsion avec images custom
    std::unique_ptr<juce::DrawableButton> distButton;
    std::unique_ptr<juce::Drawable> distOnImage;
    std::unique_ptr<juce::Drawable> distOffImage;
    bool distState = false;

    std::unique_ptr<juce::TextButton> distTextButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MerjEQAudioProcessorEditor)
};
