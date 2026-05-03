#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class BitCrusherAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit BitCrusherAudioProcessorEditor(BitCrusherAudioProcessor&);
    ~BitCrusherAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    BitCrusherAudioProcessor& processorRef;

    juce::Slider crushSlider;
    juce::Label  crushLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crushAttachment;

    juce::Slider mixSlider;
    juce::Label  mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BitCrusherAudioProcessorEditor)
};
