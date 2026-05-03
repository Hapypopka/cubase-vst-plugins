#include "PluginProcessor.h"
#include "PluginEditor.h"

BitCrusherAudioProcessor::BitCrusherAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout BitCrusherAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("crush", 1),
        "Crush",
        juce::NormalisableRange<float>(1.0f, 16.0f, 1.0f),
        16.0f,
        juce::AudioParameterFloatAttributes().withLabel("bits")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mix", 1),
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    return { params.begin(), params.end() };
}

void BitCrusherAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    smoothedCrush.reset(sampleRate, 0.02);
    smoothedCrush.setCurrentAndTargetValue(parameters.getRawParameterValue("crush")->load());
    smoothedMix.reset(sampleRate, 0.02);
    smoothedMix.setCurrentAndTargetValue(parameters.getRawParameterValue("mix")->load() * 0.01f);
}

bool BitCrusherAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput  = layouts.getMainInputChannelSet();
    return mainOutput == mainInput
        && (mainOutput == juce::AudioChannelSet::mono()
            || mainOutput == juce::AudioChannelSet::stereo());
}

void BitCrusherAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float crushBits = parameters.getRawParameterValue("crush")->load();
    smoothedCrush.setTargetValue(crushBits);
    smoothedMix.setTargetValue(parameters.getRawParameterValue("mix")->load() * 0.01f);

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float bits = smoothedCrush.getNextValue();
        const float mix  = smoothedMix.getNextValue();
        const float levels = std::pow(2.0f, bits) - 1.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float& s = buffer.getWritePointer(ch)[sample];
            const float crushed = std::round(s * levels) / levels;
            s = s + mix * (crushed - s);
        }
    }
}

juce::AudioProcessorEditor* BitCrusherAudioProcessor::createEditor()
{
    return new BitCrusherAudioProcessorEditor(*this);
}

void BitCrusherAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState(); auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void BitCrusherAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BitCrusherAudioProcessor();
}
