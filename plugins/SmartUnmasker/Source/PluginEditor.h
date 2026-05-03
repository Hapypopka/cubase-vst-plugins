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

    // Processing knobs
    juce::Slider claritySlider, focusSlider, attackSlider, releaseSlider, protectBodySlider, transientsSlider;
    juce::Label clarityLabel, focusLabel, attackLabel, releaseLabel, protectBodyLabel, transientsLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clarityAtt, focusAtt, attackAtt, releaseAtt, protectBodyAtt, transientsAtt;

    // Mode buttons
    juce::TextButton vocalCleanBtn{"Vocal Clean"}, mixGlueBtn{"Mix Glue"}, punchBtn{"Punch"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> modeAtt;

    // Bottom section
    juce::Slider mixSlider, outputSlider;
    juce::Label mixLabel, outputLabel;
    juce::ToggleButton deltaBtn{"Delta"};

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAtt, outputAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> deltaAtt;

    // Mode handling
    void updateModeButtons(int mode);
    void setMode(int mode);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartUnmaskerAudioProcessorEditor)
};
