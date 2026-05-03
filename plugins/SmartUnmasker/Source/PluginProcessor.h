#pragma once
#include <JuceHeader.h>
#include "DSP/FFTProcessor.h"
#include "DSP/SpectralMask.h"
#include "DSP/OverlapAdd.h"

class SmartUnmaskerAudioProcessor : public juce::AudioProcessor
{
public:
    SmartUnmaskerAudioProcessor();
    ~SmartUnmaskerAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SmartUnmasker"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void processChannel(int channel, const float* mainInput, const float* sideInput,
                        float* output, int numSamples);

    static constexpr int maxChannels = 2;
    static constexpr int fftSize = FFTProcessor::fftSize;
    static constexpr int numBins = fftSize / 2 + 1;
    static constexpr int hopSize = fftSize / 4;

    FFTProcessor fftProcessor;
    SpectralMask spectralMask[maxChannels];
    OverlapAdd overlapAddMain[maxChannels];
    OverlapAdd overlapAddSide[maxChannels];

    // FFT work buffers
    std::array<float, fftSize * 2> mainFFTData;
    std::array<float, fftSize * 2> sideFFTData;
    std::array<float, fftSize> mainFrame;
    std::array<float, fftSize> sideFrame;
    std::array<float, numBins> mainMag;
    std::array<float, numBins> sideMag;
    std::array<float, numBins> gainReduction;
    std::array<float, fftSize * 2> processedFFT;

    int samplesPushed[maxChannels] = {};

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartUnmaskerAudioProcessor)
};
