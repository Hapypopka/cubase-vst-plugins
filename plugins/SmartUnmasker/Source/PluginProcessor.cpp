#include "PluginProcessor.h"
#include "PluginEditor.h"

SmartUnmaskerAudioProcessor::SmartUnmaskerAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SmartUnmaskerAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("clarity", 1),
        "Clarity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("attack", 1),
        "Attack",
        juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f, 0.5f),
        10.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("release", 1),
        "Release",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.5f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mix", 1),
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        1.0f));

    return { params.begin(), params.end() };
}

void SmartUnmaskerAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    for (auto& state : channelState)
    {
        state.mask.prepare(fftSize * 2);
        state.ola.prepare(fftSize, hopSize);
        state.inputFifo.resize(fftSize, 0.0f);
        state.sideFifo.resize(fftSize, 0.0f);
        state.outputBlock.resize(fftSize, 0.0f);
        state.fifoIndex = 0;
        state.mask.reset();
        state.ola.reset();
    }
}

bool SmartUnmaskerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainInput = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    if (mainOutput != mainInput)
        return false;

    if (mainOutput != juce::AudioChannelSet::stereo() && mainOutput != juce::AudioChannelSet::mono())
        return false;

    // Sidechain must be stereo, mono, or disabled
    const auto& sidechain = layouts.getChannelSet(true, 1);
    if (!sidechain.isDisabled()
        && sidechain != juce::AudioChannelSet::stereo()
        && sidechain != juce::AudioChannelSet::mono())
        return false;

    return true;
}

void SmartUnmaskerAudioProcessor::processFFTBlock(ChannelFFTState& state, float amount, float attackCoeff, float releaseCoeff)
{
    state.mainFFT.forward(state.inputFifo.data());
    state.sideFFT.forward(state.sideFifo.data());

    state.mask.process(state.mainFFT.getData(), state.sideFFT.getData(), fftSize * 2, amount, attackCoeff, releaseCoeff);

    state.mainFFT.inverse(state.outputBlock.data());
    state.ola.applyWindow(state.outputBlock.data());
    state.ola.addToOutput(state.outputBlock.data());
}

void SmartUnmaskerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numOutputChannels = juce::jmin(buffer.getNumChannels(), 2);

    auto* sideInputBus = getBus(true, 1);
    const bool hasSidechain = sideInputBus != nullptr && sideInputBus->isEnabled()
                              && getBusBuffer(buffer, true, 1).getNumChannels() > 0;

    // If no sidechain connected or not yet prepared, pass audio through unchanged
    if (!hasSidechain || channelState[0].inputFifo.empty())
        return;

    const float clarity = parameters.getRawParameterValue("clarity")->load();
    const float attackMs = parameters.getRawParameterValue("attack")->load();
    const float releaseMs = parameters.getRawParameterValue("release")->load();
    const float mix = parameters.getRawParameterValue("mix")->load();

    const float attackCoeff = 1.0f - std::exp(-1.0f / (float(currentSampleRate) * attackMs * 0.001f));
    const float releaseCoeff = 1.0f - std::exp(-1.0f / (float(currentSampleRate) * releaseMs * 0.001f));

    auto sideBuffer = getBusBuffer(buffer, true, 1);
    const int sidechainChannels = sideBuffer.getNumChannels();

    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        auto& state = channelState[ch];
        auto* mainData = buffer.getWritePointer(ch);
        const int sideCh = juce::jmin(ch, sidechainChannels - 1);
        const float* sideData = sideBuffer.getReadPointer(sideCh);

        for (int i = 0; i < numSamples; ++i)
        {
            const float drySample = mainData[i];

            state.inputFifo[state.fifoIndex] = mainData[i];
            state.sideFifo[state.fifoIndex] = sideData[i];
            state.fifoIndex++;

            if (state.fifoIndex >= fftSize)
            {
                state.fifoIndex = 0;
                processFFTBlock(state, clarity * 3.0f, attackCoeff, releaseCoeff);
            }

            float wetSample = state.ola.getNextSample();
            mainData[i] = drySample + mix * (wetSample - drySample);
        }
    }
}

juce::AudioProcessorEditor* SmartUnmaskerAudioProcessor::createEditor()
{
    return new SmartUnmaskerAudioProcessorEditor(*this);
}

void SmartUnmaskerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState(); auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void SmartUnmaskerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SmartUnmaskerAudioProcessor();
}
