#pragma once
#include <JuceHeader.h>
#include "FFTProcessor.h"
#include "SpectralMask.h"
#include "OverlapAdd.h"

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

    const juce::String getName() const override { return "Smart Unmasker"; }
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
    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int hopSize = fftSize / 4;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct ChannelFFTState
    {
        FFTProcessor mainFFT{fftOrder};
        FFTProcessor sideFFT{fftOrder};
        SpectralMask mask;
        OverlapAdd ola;
        std::vector<float> inputFifo;
        std::vector<float> sideFifo;
        std::vector<float> outputBlock;
        int fifoIndex = 0;
    };

    std::array<ChannelFFTState, 2> channelState;
    double currentSampleRate = 44100.0;

    void processFFTBlock(ChannelFFTState& state, float amount, float attackCoeff, float releaseCoeff);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartUnmaskerAudioProcessor)
};
