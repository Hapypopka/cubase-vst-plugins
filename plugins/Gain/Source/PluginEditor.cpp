#include "PluginEditor.h"

GainAudioProcessorEditor::GainAudioProcessorEditor(GainAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processorRef(p)
{
    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 22);
    addAndMakeVisible(gainSlider);

    gainLabel.setText("Gain (dB)", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gainLabel);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "gain", gainSlider);

    setSize(280, 240);
}

void GainAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222227));
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawFittedText("Gain", getLocalBounds().removeFromTop(36), juce::Justification::centred, 1);
}

void GainAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(36);
    gainLabel.setBounds(bounds.removeFromBottom(24));
    gainSlider.setBounds(bounds);
}
