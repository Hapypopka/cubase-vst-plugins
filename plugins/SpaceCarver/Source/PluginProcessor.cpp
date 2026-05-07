#include "PluginProcessor.h"
#include "PluginEditor.h"

SpaceCarverAudioProcessor::SpaceCarverAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMS", createLayout())
{
    smoothedMain.fill(-100.0f);
    smoothedSide.fill(-100.0f);
    smoothedReduction.fill(0.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpaceCarverAudioProcessor::createLayout()
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

void SpaceCarverAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    for (auto& state : channelState)
    {
        state.mask.prepare(fftSize * 2, sampleRate);
        state.ola.prepare(fftSize, hopSize);
        state.deltaOla.prepare(fftSize, hopSize);

        state.mainCircular.resize(fftSize, 0.0f);
        state.sideCircular.resize(fftSize, 0.0f);
        state.fftMainInput.resize(fftSize, 0.0f);
        state.fftSideInput.resize(fftSize, 0.0f);
        state.outputBlock.resize(fftSize, 0.0f);
        state.deltaBlock.resize(fftSize, 0.0f);

        state.circularWritePos = 0;
        state.hopCounter = 0;

        state.mask.reset();
        state.ola.reset();
        state.deltaOla.reset();
    }

    // Compute exact OLA gain compensation by measuring actual FFT round-trip + window gain
    {
        FFTProcessor testFFT(fftOrder);
        OverlapAdd testOla;
        testOla.prepare(fftSize, hopSize);

        std::vector<float> testInput(fftSize, 0.5f);
        std::vector<float> testOutput(fftSize, 0.0f);

        // Run enough hops to reach steady state
        for (int hop = 0; hop < 8; ++hop)
        {
            testFFT.forward(testInput.data());
            testFFT.inverse(testOutput.data());
            testOla.applyWindow(testOutput.data());
            testOla.addToOutput(testOutput.data());
            for (int s = 0; s < hopSize; ++s)
                testOla.getNextSample();
        }

        // Measure steady-state output level
        float sum = 0.0f;
        for (int hop = 0; hop < 2; ++hop)
        {
            testFFT.forward(testInput.data());
            testFFT.inverse(testOutput.data());
            testOla.applyWindow(testOutput.data());
            testOla.addToOutput(testOutput.data());
            for (int s = 0; s < hopSize; ++s)
                sum += testOla.getNextSample();
        }

        float avgOutput = sum / float(hopSize * 2);
        olaCompensation = (avgOutput > 1e-6f) ? (0.5f / avgOutput) : 1.0f;
    }

    smoothedMain.fill(-100.0f);
    smoothedSide.fill(-100.0f);
    smoothedReduction.fill(0.0f);
}

bool SpaceCarverAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void SpaceCarverAudioProcessor::copyCircularToLinear(const std::vector<float>& circular, int writePos, std::vector<float>& linear)
{
    const int bufSize = (int)circular.size();
    for (int i = 0; i < fftSize; ++i)
    {
        int idx = (writePos - fftSize + i + bufSize) % bufSize;
        linear[i] = circular[idx];
    }
}

void SpaceCarverAudioProcessor::updateSpectrumDisplay(const ChannelFFTState& state)
{
    const float* mainFFTData = state.mainFFT.getData();
    const float* sideFFTData = state.sideFFT.getData();
    const auto& reduction = state.mask.getGainReduction();

    const float smoothing = 0.8f;
    const float minDb = -100.0f;

    for (int bin = 0; bin < numDisplayBins && bin < fftSize; ++bin)
    {
        int i = bin * 2;

        // Normalize: 2/N for single-sided, plus ~18dB boost so music signal sits near 0dB
        // (individual FFT bins for broadband music are much lower than peak amplitude)
        const float normFactor = 16.0f / float(fftSize);

        float mainMag = std::sqrt(mainFFTData[i] * mainFFTData[i] + mainFFTData[i + 1] * mainFFTData[i + 1]) * normFactor;
        float mainDb = mainMag > 1e-10f ? 20.0f * std::log10(mainMag) : minDb;

        float sideMag = std::sqrt(sideFFTData[i] * sideFFTData[i] + sideFFTData[i + 1] * sideFFTData[i + 1]) * normFactor;
        float sideDb = sideMag > 1e-10f ? 20.0f * std::log10(sideMag) : minDb;

        float redDb = (bin < (int)reduction.size() / 2) ? reduction[i] * 48.0f : 0.0f;

        smoothedMain[bin] = smoothedMain[bin] * smoothing + mainDb * (1.0f - smoothing);
        smoothedSide[bin] = smoothedSide[bin] * smoothing + sideDb * (1.0f - smoothing);
        smoothedReduction[bin] = smoothedReduction[bin] * smoothing + redDb * (1.0f - smoothing);
    }

    const juce::SpinLock::ScopedLockType lock(spectrumLock);
    spectrumForDisplay.mainSpectrum = smoothedMain;
    spectrumForDisplay.sideSpectrum = smoothedSide;
    spectrumForDisplay.reductionDb = smoothedReduction;
}

void SpaceCarverAudioProcessor::processFFTBlock(ChannelFFTState& state, const SpectralMaskParams& params)
{
    float sideEnergy = 0.0f;
    for (int i = 0; i < fftSize; ++i)
        sideEnergy += state.fftSideInput[i] * state.fftSideInput[i];
    sideEnergy /= float(fftSize);

    if (sideEnergy < silenceThreshold)
    {
        state.mainFFT.forward(state.fftMainInput.data());
        state.sideFFT.forward(state.fftSideInput.data());

        // Update display even when silent
        updateSpectrumDisplay(state);

        state.mainFFT.inverse(state.outputBlock.data());
        state.ola.applyWindow(state.outputBlock.data());
        state.ola.addToOutput(state.outputBlock.data());

        std::fill(state.deltaBlock.begin(), state.deltaBlock.end(), 0.0f);
        state.deltaOla.applyWindow(state.deltaBlock.data());
        state.deltaOla.addToOutput(state.deltaBlock.data());
        return;
    }

    state.mainFFT.forward(state.fftMainInput.data());
    state.sideFFT.forward(state.fftSideInput.data());

    // Update display BEFORE mask processing so we show the original input spectrum
    updateSpectrumDisplay(state);

    state.mask.process(state.mainFFT.getData(), fftSize * 2, state.sideFFT.getData(), params);

    state.mainFFT.inverse(state.outputBlock.data());
    state.ola.applyWindow(state.outputBlock.data());
    state.ola.addToOutput(state.outputBlock.data());

    FFTProcessor tempFFT(fftOrder);
    tempFFT.forward(state.fftMainInput.data());
    tempFFT.inverse(state.deltaBlock.data());

    for (int i = 0; i < fftSize; ++i)
        state.deltaBlock[i] -= state.outputBlock[i];

    state.deltaOla.applyWindow(state.deltaBlock.data());
    state.deltaOla.addToOutput(state.deltaBlock.data());
}

void SpaceCarverAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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

    if (channelState[0].mainCircular.empty())
        return;

    auto sideBuffer = getBusBuffer(buffer, true, 1);
    const int sidechainChannels = sideBuffer.getNumChannels();
    if (sidechainChannels <= 0)
        return;

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

            state.mainCircular[state.circularWritePos] = mainData[i];
            state.sideCircular[state.circularWritePos] = sideData[i];
            state.circularWritePos = (state.circularWritePos + 1) % fftSize;

            state.hopCounter++;

            if (state.hopCounter >= hopSize)
            {
                state.hopCounter = 0;
                copyCircularToLinear(state.mainCircular, state.circularWritePos, state.fftMainInput);
                copyCircularToLinear(state.sideCircular, state.circularWritePos, state.fftSideInput);
                processFFTBlock(state, maskParams);
            }

            float wetSample = state.ola.getNextSample() * olaCompensation;
            float deltaSample = state.deltaOla.getNextSample() * olaCompensation;

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

juce::AudioProcessorEditor* SpaceCarverAudioProcessor::createEditor()
{
    return new SpaceCarverAudioProcessorEditor(*this);
}

void SpaceCarverAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState(); auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void SpaceCarverAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpaceCarverAudioProcessor();
}
