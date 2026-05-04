#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectrumAnalyzer.h"

class SpaceCarverAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SpaceCarverAudioProcessorEditor(SpaceCarverAudioProcessor&);
    ~SpaceCarverAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SpaceCarverAudioProcessor& processorRef;

    SpectrumAnalyzer analyzer;

    juce::Slider claritySlider, focusSlider, attackSlider, releaseSlider, protectBodySlider, transientsSlider;
    juce::Label clarityLabel, focusLabel, attackLabel, releaseLabel, protectBodyLabel, transientsLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clarityAtt, focusAtt, attackAtt, releaseAtt, protectBodyAtt, transientsAtt;

    juce::TextButton vocalCleanBtn{"Vocal Clean"}, mixGlueBtn{"Mix Glue"}, punchBtn{"Punch"};

    juce::Slider mixSlider, outputSlider;
    juce::Label mixLabel, outputLabel;
    juce::ToggleButton deltaBtn{"Delta"};

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAtt, outputAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> deltaAtt;

    void updateModeButtons(int mode);
    void setMode(int mode);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpaceCarverAudioProcessorEditor)
};
