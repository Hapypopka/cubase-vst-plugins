#include "PluginProcessor.h"
#include "PluginEditor.h"

SmartUnmaskerAudioProcessor::SmartUnmaskerAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SmartUnmaskerAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("clarity", 1), "Clarity",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("focus", 1), "Focus",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("attack", 1), "Attack",
        juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f, 0.5f), 10.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("release", 1), "Release",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.5f), 120.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("protectbody", 1), "Protect Body",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 40.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("transients", 1), "Transients",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("mode", 1), "Mode",
        juce::StringArray{"Vocal Clean", "Mix Glue", "Punch"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mix", 1), "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("output", 1), "Output",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("delta", 1), "Delta", false));

    return { params.begin(), params.end() };
}

void SmartUnmaskerAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    for (auto& state : channelState)
    {
        state.mask.prepare(fftSize * 2, sampleRate);
        state.ola.prepare(fftSize, hopSize);
        state.deltaOla.prepare(fftSize, hopSize);
        state.inputFifo.resize(fftSize, 0.0f);
        state.sideFifo.resize(fftSize, 0.0f);
        state.outputBlock.resize(fftSize, 0.0f);
        state.deltaBlock.resize(fftSize, 0.0f);
        state.fifoIndex = 0;
        state.mask.reset();
        state.ola.reset();
        state.deltaOla.reset();
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

    const auto& sidechain = layouts.getChannelSet(true, 1);
    if (!sidechain.isDisabled()
        && sidechain != juce::AudioChannelSet::stereo()
        && sidechain != juce::AudioChannelSet::mono())
        return false;

    return true;
}

void SmartUnmaskerAudioProcessor::processFFTBlock(ChannelFFTState& state, const SpectralMaskParams& params)
{
    state.mainFFT.forward(state.inputFifo.data());
    state.sideFFT.forward(state.sideFifo.data());

    // Save dry FFT for delta calculation
    std::vector<float> dryFFT(fftSize * 2);
    std::memcpy(dryFFT.data(), state.mainFFT.getData(), sizeof(float) * fftSize * 2);

    state.mask.process(state.mainFFT.getData(), fftSize * 2, state.sideFFT.getData(), params);

    // Wet output
    state.mainFFT.inverse(state.outputBlock.data());
    state.ola.applyWindow(state.outputBlock.data());
    state.ola.addToOutput(state.outputBlock.data());

    // Delta = dry - wet (what was removed)
    float* wetFFT = state.mainFFT.getData();
    // We need to re-do inverse on dryFFT - wetFFT, but mainFFT was already modified
    // So compute delta in time domain after inverse
    // Actually recompute: delta = inputFifo (windowed) - outputBlock
    // Simpler: delta FFT = dryFFT - processedFFT
    // But mainFFT was already inversed. Let's use dryFFT directly.
    // Re-forward the processed to get delta? No, let's just compute delta in time domain.

    // Compute delta block: original windowed input minus processed output
    // The outputBlock already has the processed (post-inverse, pre-window) signal
    // We need original windowed signal for comparison
    FFTProcessor tempFFT(fftOrder);
    tempFFT.forward(state.inputFifo.data());
    tempFFT.inverse(state.deltaBlock.data());

    for (int i = 0; i < fftSize; ++i)
        state.deltaBlock[i] -= state.outputBlock[i];

    state.deltaOla.applyWindow(state.deltaBlock.data());
    state.deltaOla.addToOutput(state.deltaBlock.data());
}

void SmartUnmaskerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    const int numOutputChannels = juce::jmin(buffer.getNumChannels(), 2);

    auto* sideInputBus = getBus(true, 1);
    if (sideInputBus == nullptr || !sideInputBus->isEnabled())
        return;

    const int mainBusCh = getMainBusNumInputChannels();
    const int expectedTotalCh = mainBusCh + sideInputBus->getNumberOfChannels();
    if (buffer.getNumChannels() < expectedTotalCh)
        return;

    if (channelState[0].inputFifo.empty())
        return;

    auto sideBuffer = getBusBuffer(buffer, true, 1);
    const int sidechainChannels = sideBuffer.getNumChannels();
    if (sidechainChannels <= 0)
        return;

    // Read parameters
    const float clarity = parameters.getRawParameterValue("clarity")->load() / 100.0f;
    const float focus = parameters.getRawParameterValue("focus")->load() / 100.0f;
    const float attackMs = parameters.getRawParameterValue("attack")->load();
    const float releaseMs = parameters.getRawParameterValue("release")->load();
    const float protectBody = parameters.getRawParameterValue("protectbody")->load() / 100.0f;
    const float transients = parameters.getRawParameterValue("transients")->load() / 100.0f;
    const int mode = (int)parameters.getRawParameterValue("mode")->load();
    const float mix = parameters.getRawParameterValue("mix")->load() / 100.0f;
    const float outputDb = parameters.getRawParameterValue("output")->load();
    const bool delta = parameters.getRawParameterValue("delta")->load() > 0.5f;

    const float outputGain = juce::Decibels::decibelsToGain(outputDb);

    // Mode modifiers
    float modeAmountMult = 1.0f;
    float modeFocusBias = 0.0f;
    float modeBodyBias = 0.0f;
    float modeTransBias = 0.0f;

    switch (mode)
    {
        case VocalClean:
            modeAmountMult = 3.0f;
            modeFocusBias = 0.1f;
            modeBodyBias = 0.2f;
            break;
        case MixGlue:
            modeAmountMult = 2.0f;
            modeFocusBias = -0.2f;
            modeBodyBias = 0.0f;
            modeTransBias = -0.1f;
            break;
        case Punch:
            modeAmountMult = 3.5f;
            modeFocusBias = 0.2f;
            modeBodyBias = 0.1f;
            modeTransBias = 0.3f;
            break;
    }

    SpectralMaskParams maskParams;
    maskParams.amount = clarity * modeAmountMult;
    maskParams.focus = juce::jlimit(0.0f, 1.0f, focus + modeFocusBias);
    maskParams.protectBody = juce::jlimit(0.0f, 1.0f, protectBody + modeBodyBias);
    maskParams.transients = juce::jlimit(0.0f, 1.0f, transients + modeTransBias);
    maskParams.attackCoeff = 1.0f - std::exp(-1.0f / (float(currentSampleRate) * attackMs * 0.001f));
    maskParams.releaseCoeff = 1.0f - std::exp(-1.0f / (float(currentSampleRate) * releaseMs * 0.001f));

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
                processFFTBlock(state, maskParams);
            }

            float wetSample = state.ola.getNextSample();
            float deltaSample = state.deltaOla.getNextSample();

            if (delta)
            {
                mainData[i] = deltaSample * outputGain;
            }
            else
            {
                mainData[i] = (drySample + mix * (wetSample - drySample)) * outputGain;
            }
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
