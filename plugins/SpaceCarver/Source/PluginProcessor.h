#pragma once
#include <JuceHeader.h>
#include "FFTProcessor.h"
#include "SpectralMask.h"
#include "OverlapAdd.h"

class SpaceCarverAudioProcessor : public juce::AudioProcessor
{
public:
    SpaceCarverAudioProcessor();
    ~SpaceCarverAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SpaceCarver"; }
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

    enum Mode { VocalClean = 0, MixGlue = 1, Punch = 2 };

private:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int hopSize = fftSize / 4;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct ChannelFFTState
    {
        FFTProcessor mainFFT{fftOrder};
        FFTProcessor sideFFT{fftOrder};
        SpectralMask mask;
        OverlapAdd ola;
        OverlapAdd deltaOla;
        std::vector<float> inputFifo;
        std::vector<float> sideFifo;
        std::vector<float> outputBlock;
        std::vector<float> deltaBlock;
        int fifoIndex = 0;
    };

    std::array<ChannelFFTState, 2> channelState;
    double currentSampleRate = 44100.0;

    void processFFTBlock(ChannelFFTState& state, const SpectralMaskParams& params);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpaceCarverAudioProcessor)
};
