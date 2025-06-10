/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ============================================================================== 
*/

#pragma once

#include <JuceHeader.h>
#include <array>

class MerjEQAudioProcessor : public juce::AudioProcessor
{
public:
    MerjEQAudioProcessor();
    ~MerjEQAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(const int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()};

    // === Saturation ON/OFF ===
    bool saturationEnabled = false;

private:
    juce::dsp::IIR::Filter<float> lowShelfFilter[2];
    juce::dsp::IIR::Filter<float> midBandFilter[2];
    juce::dsp::IIR::Filter<float> highShelfFilter[2];
    double lastSampleRate = 44100.0;
    void updateFilters();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MerjEQAudioProcessor)
};
