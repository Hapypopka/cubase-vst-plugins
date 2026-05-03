#include "PluginEditor.h"

SmartUnmaskerAudioProcessorEditor::SmartUnmaskerAudioProcessorEditor(SmartUnmaskerAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processorRef(p)
{
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text,
                              const juce::String& paramId)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };

    setupSlider(claritySlider, clarityLabel, "Clarity", "clarity");
    setupSlider(attackSlider, attackLabel, "Attack", "attack");
    setupSlider(releaseSlider, releaseLabel, "Release", "release");
    setupSlider(mixSlider, mixLabel, "Mix", "mix");

    clarityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "clarity", claritySlider);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "attack", attackSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "release", releaseSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, "mix", mixSlider);

    setSize(480, 280);
}

void SmartUnmaskerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("Smart Unmasker", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);

    g.setFont(12.0f);
    g.setColour(juce::Colour(0xffaaaaaa));
    g.drawFittedText("Sidechain: route vocal/instrument to sidechain input",
                     getLocalBounds().removeFromBottom(24), juce::Justification::centred, 1);
}

void SmartUnmaskerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(44);
    bounds.removeFromBottom(24);

    const int knobWidth = bounds.getWidth() / 4;

    auto setupArea = [&](juce::Slider& slider, juce::Label& label)
    {
        auto area = bounds.removeFromLeft(knobWidth);
        label.setBounds(area.removeFromBottom(22));
        slider.setBounds(area);
    };

    setupArea(claritySlider, clarityLabel);
    setupArea(attackSlider, attackLabel);
    setupArea(releaseSlider, releaseLabel);
    setupArea(mixSlider, mixLabel);
}
