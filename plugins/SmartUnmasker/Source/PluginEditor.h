#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SmartUnmaskerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SmartUnmaskerAudioProcessorEditor(SmartUnmaskerAudioProcessor&);
    ~SmartUnmaskerAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SmartUnmaskerAudioProcessor& processorRef;

    juce::Slider claritySlider, attackSlider, releaseSlider, mixSlider;
    juce::Label clarityLabel, attackLabel, releaseLabel, mixLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clarityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartUnmaskerAudioProcessorEditor)
};
