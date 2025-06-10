/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout MerjEQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LowGain", "Low Gain", juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MidGain", "Mid Gain", juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("HighGain", "High Gain", juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MidQ", "Mid Q", juce::NormalisableRange<float>(0.1f, 5.0f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("saturationEnabled", "Saturation Enabled", false));
    return { params.begin(), params.end() };
}

// === Saturation parameters ===
bool enableSaturation = true; // Activation facile de la saturation
float saturationInputGain = 1.2f; // Gain d'entrée pour la saturation (ajustable)

// Fonction de saturation harmonique douce (type lampe, harmoniques paires)
inline float applySaturation(float x)
{
    // Gain d'entrée
    x *= saturationInputGain;
    // Saturation douce type lampe (tanh + 10% d'asin pour enrichir les paires)
    float saturated = std::tanh(x) + 0.1f * std::asin(std::clamp(x, -1.0f, 1.0f));
    // Normalisation douce pour éviter le clipping
    return juce::jlimit(-1.0f, 1.0f, saturated * 0.9f);
}

// Fonction de saturation douce (tanh, +6 dB drive)
inline float softSaturation(float x)
{
    return std::tanh(x * 2.0f);
}

MerjEQAudioProcessor::MerjEQAudioProcessor()
    : apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Force gain params to 0.0 dB at construction (centered)
    if (auto* low = apvts.getParameter("LowGain")) low->setValueNotifyingHost(0.5f);
    if (auto* mid = apvts.getParameter("MidGain")) mid->setValueNotifyingHost(0.5f);
    if (auto* high = apvts.getParameter("HighGain")) high->setValueNotifyingHost(0.5f);
}

MerjEQAudioProcessor::~MerjEQAudioProcessor() = default;

void MerjEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    lastSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 };
    for (int ch = 0; ch < 2; ++ch) {
        lowShelfFilter[ch].reset();
        midBandFilter[ch].reset();
        highShelfFilter[ch].reset();
        lowShelfFilter[ch].prepare(spec);
        midBandFilter[ch].prepare(spec);
        highShelfFilter[ch].prepare(spec);
    }
    updateFilters();
}

void MerjEQAudioProcessor::updateFilters()
{
    static float prevLowGain = 0.0f, prevMidGain = 0.0f, prevHighGain = 0.0f;
    static float prevMidQ = 1.0f;

    float LowGain = apvts.getRawParameterValue("LowGain")->load();
    float MidGain = apvts.getRawParameterValue("MidGain")->load();
    float HighGain = apvts.getRawParameterValue("HighGain")->load();
    float MidQ = apvts.getRawParameterValue("MidQ")->load();

    bool lowChanged = (LowGain != prevLowGain);
    bool midChanged = (MidGain != prevMidGain) || (MidQ != prevMidQ);
    bool highChanged = (HighGain != prevHighGain);

    if (lowChanged) {
        auto lowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(lastSampleRate, 200.0f, 0.707f, juce::Decibels::decibelsToGain(LowGain));
        for (int ch = 0; ch < 2; ++ch)
            *lowShelfFilter[ch].coefficients = *lowCoeffs;
        prevLowGain = LowGain;
    }
    if (midChanged) {
        auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(lastSampleRate, 4000.0f, MidQ, juce::Decibels::decibelsToGain(MidGain));
        for (int ch = 0; ch < 2; ++ch)
            *midBandFilter[ch].coefficients = *midCoeffs;
        prevMidGain = MidGain;
        prevMidQ = MidQ;
    }
    if (highChanged) {
        auto highCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(lastSampleRate, 12000.0f, 0.707f, juce::Decibels::decibelsToGain(HighGain));
        for (int ch = 0; ch < 2; ++ch)
            *highShelfFilter[ch].coefficients = *highCoeffs;
        prevHighGain = HighGain;
    }
}

void MerjEQAudioProcessor::releaseResources() {}

void MerjEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto channelBlock = block.getSingleChannelBlock(ch);
        juce::dsp::ProcessContextReplacing<float> context(channelBlock);
        lowShelfFilter[ch].process(context);
        midBandFilter[ch].process(context);
        highShelfFilter[ch].process(context);
    }

    // === Saturation douce sur la sortie si activée ===
    if (apvts.getRawParameterValue("saturationEnabled")->load() > 0.5f) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                data[i] = softSaturation(data[i]);
            }
        }
    }
}

juce::AudioProcessorEditor* MerjEQAudioProcessor::createEditor() { return new MerjEQAudioProcessorEditor(*this); }
bool MerjEQAudioProcessor::hasEditor() const { return true; }

const juce::String MerjEQAudioProcessor::getName() const { return JucePlugin_Name; }
bool MerjEQAudioProcessor::acceptsMidi() const { return false; }
bool MerjEQAudioProcessor::producesMidi() const { return false; }
bool MerjEQAudioProcessor::isMidiEffect() const { return false; }
double MerjEQAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int MerjEQAudioProcessor::getNumPrograms() { return 1; }
int MerjEQAudioProcessor::getCurrentProgram() { return 0; }
void MerjEQAudioProcessor::setCurrentProgram(int) {}
const juce::String MerjEQAudioProcessor::getProgramName(int) { return {}; }
void MerjEQAudioProcessor::changeProgramName(const int, const juce::String&) {}

void MerjEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
void MerjEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
    // Always force gain params to 0.0 dB if not present in state
    if (!apvts.state.hasProperty("LowGain"))
        apvts.getParameter("LowGain")->setValueNotifyingHost(0.5f);
    if (!apvts.state.hasProperty("MidGain"))
        apvts.getParameter("MidGain")->setValueNotifyingHost(0.5f);
    if (!apvts.state.hasProperty("HighGain"))
        apvts.getParameter("HighGain")->setValueNotifyingHost(0.5f);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MerjEQAudioProcessor();
}
