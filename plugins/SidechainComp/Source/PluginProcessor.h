#pragma once
#include <JuceHeader.h>

class SidechainCompAudioProcessor : public juce::AudioProcessor
{
public:
    SidechainCompAudioProcessor();
    ~SidechainCompAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SidechainComp"; }
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

    float envelopeLevel = 0.0f;

    juce::SmoothedValue<float> smoothedThreshold;
    juce::SmoothedValue<float> smoothedRatio;
    juce::SmoothedValue<float> smoothedAttack;
    juce::SmoothedValue<float> smoothedRelease;
    juce::SmoothedValue<float> smoothedMix;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainCompAudioProcessor)
};
