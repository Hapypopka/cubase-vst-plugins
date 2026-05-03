#include "PluginEditor.h"

BitCrusherAudioProcessorEditor::BitCrusherAudioProcessorEditor(BitCrusherAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processorRef(p)
{
    crushSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    crushSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 22);
    addAndMakeVisible(crushSlider);

    crushLabel.setText("Crush (bits)", juce::dontSendNotification);
    crushLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(crushLabel);

    crushAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "crush", crushSlider);

    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 22);
    addAndMakeVisible(mixSlider);

    mixLabel.setText("Mix (%)", juce::dontSendNotification);
    mixLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mixLabel);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "mix", mixSlider);

    setSize(420, 240);
}

void BitCrusherAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222227));
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawFittedText("BitCrusher", getLocalBounds().removeFromTop(36), juce::Justification::centred, 1);
}

void BitCrusherAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(36);

    auto left  = bounds.removeFromLeft(bounds.getWidth() / 2);
    auto right = bounds;

    crushLabel.setBounds(left.removeFromBottom(24));
    crushSlider.setBounds(left);

    mixLabel.setBounds(right.removeFromBottom(24));
    mixSlider.setBounds(right);
}
