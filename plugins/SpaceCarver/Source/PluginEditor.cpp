#include "PluginEditor.h"

static const juce::Colour bgColour(0xff1a1a2e);
static const juce::Colour panelColour(0xff252540);
static const juce::Colour accentColour(0xff4a6cf7);
static const juce::Colour textColour(0xffdddddd);
static const juce::Colour dimTextColour(0xff888899);
static const juce::Colour activeButtonColour(0xff4a6cf7);
static const juce::Colour inactiveButtonColour(0xff333350);

SpaceCarverAudioProcessorEditor::SpaceCarverAudioProcessorEditor(SpaceCarverAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processorRef(p), analyzer(p)
{
    addAndMakeVisible(analyzer);

    auto setupKnob = [this](juce::Slider& slider, juce::Label& label, const juce::String& text,
                             const juce::String& paramId, auto& attachment)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 18);
        slider.setColour(juce::Slider::rotarySliderFillColourId, accentColour);
        slider.setColour(juce::Slider::textBoxTextColourId, textColour);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, dimTextColour);
        label.setFont(juce::Font(12.0f));
        addAndMakeVisible(label);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.parameters, paramId, slider);
    };

    setupKnob(claritySlider, clarityLabel, "CLARITY", "clarity", clarityAtt);
    setupKnob(focusSlider, focusLabel, "FOCUS", "focus", focusAtt);
    setupKnob(attackSlider, attackLabel, "ATTACK", "attack", attackAtt);
    setupKnob(releaseSlider, releaseLabel, "RELEASE", "release", releaseAtt);
    setupKnob(protectBodySlider, protectBodyLabel, "PROTECT BODY", "protectbody", protectBodyAtt);
    setupKnob(transientsSlider, transientsLabel, "TRANSIENTS", "transients", transientsAtt);

    auto setupModeBtn = [this](juce::TextButton& btn, int modeIdx)
    {
        btn.setClickingTogglesState(false);
        btn.setColour(juce::TextButton::buttonColourId, inactiveButtonColour);
        btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        btn.setColour(juce::TextButton::textColourOffId, dimTextColour);
        btn.onClick = [this, modeIdx] { setMode(modeIdx); };
        addAndMakeVisible(btn);
    };

    setupModeBtn(vocalCleanBtn, 0);
    setupModeBtn(mixGlueBtn, 1);
    setupModeBtn(punchBtn, 2);

    int currentMode = (int)processorRef.parameters.getRawParameterValue("mode")->load();
    updateModeButtons(currentMode);

    setupKnob(mixSlider, mixLabel, "MIX", "mix", mixAtt);
    setupKnob(outputSlider, outputLabel, "OUTPUT", "output", outputAtt);

    deltaBtn.setColour(juce::ToggleButton::textColourId, textColour);
    deltaBtn.setColour(juce::ToggleButton::tickColourId, accentColour);
    addAndMakeVisible(deltaBtn);
    deltaAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.parameters, "delta", deltaBtn);

    setSize(750, 620);
}

void SpaceCarverAudioProcessorEditor::setMode(int mode)
{
    processorRef.parameters.getParameter("mode")->setValueNotifyingHost(
        processorRef.parameters.getParameter("mode")->convertTo0to1(float(mode)));
    updateModeButtons(mode);
}

void SpaceCarverAudioProcessorEditor::updateModeButtons(int mode)
{
    vocalCleanBtn.setColour(juce::TextButton::buttonColourId, mode == 0 ? activeButtonColour : inactiveButtonColour);
    mixGlueBtn.setColour(juce::TextButton::buttonColourId, mode == 1 ? activeButtonColour : inactiveButtonColour);
    punchBtn.setColour(juce::TextButton::buttonColourId, mode == 2 ? activeButtonColour : inactiveButtonColour);
    repaint();
}

void SpaceCarverAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bgColour);

    // Title bar
    auto titleArea = getLocalBounds().removeFromTop(40);
    g.setColour(juce::Colour(0xff12122a));
    g.fillRect(titleArea);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawFittedText("SPACECARVER", titleArea.reduced(16, 0), juce::Justification::centredLeft, 1);
    g.setFont(juce::Font(11.0f));
    g.setColour(dimTextColour);
    g.drawFittedText("Intelligent Spectral Unmasking", titleArea.reduced(16, 0), juce::Justification::centredRight, 1);

    // Processing panel
    auto bounds = getLocalBounds();
    bounds.removeFromTop(40);
    bounds.removeFromTop(240); // analyzer space
    auto processingArea = bounds.removeFromTop(200).reduced(10, 6);
    g.setColour(panelColour);
    g.fillRoundedRectangle(processingArea.toFloat(), 6.0f);

    g.setColour(accentColour);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("PROCESSING", processingArea.getX() + 12, processingArea.getY() + 6, 120, 16, juce::Justification::centredLeft);

    // Mode panel
    auto modePanelArea = processingArea.removeFromRight(130);
    g.setColour(panelColour.brighter(0.1f));
    g.fillRoundedRectangle(modePanelArea.toFloat(), 6.0f);
    g.setColour(accentColour);
    g.drawText("MODE", modePanelArea.getX() + 8, modePanelArea.getY() + 6, 60, 16, juce::Justification::centredLeft);

    // Bottom panel
    auto bottomArea = bounds.reduced(10, 4);
    g.setColour(panelColour);
    g.fillRoundedRectangle(bottomArea.toFloat(), 6.0f);

    g.setColour(dimTextColour);
    g.setFont(juce::Font(11.0f));
    g.drawFittedText("Route sidechain input in DAW", bottomArea.removeFromRight(200).reduced(8),
                     juce::Justification::centredRight, 1);
}

void SpaceCarverAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(40); // title

    // Spectrum analyzer
    auto analyzerArea = bounds.removeFromTop(240).reduced(10, 6);
    analyzer.setBounds(analyzerArea);

    // Processing section
    auto processingArea = bounds.removeFromTop(200).reduced(10, 6);
    auto modeArea = processingArea.removeFromRight(130).reduced(8, 28);

    auto knobArea = processingArea.reduced(8, 28);
    const int knobWidth = knobArea.getWidth() / 6;

    auto placeKnob = [&](juce::Slider& slider, juce::Label& label)
    {
        auto area = knobArea.removeFromLeft(knobWidth);
        label.setBounds(area.removeFromBottom(20));
        slider.setBounds(area.reduced(2, 4));
    };

    placeKnob(claritySlider, clarityLabel);
    placeKnob(focusSlider, focusLabel);
    placeKnob(attackSlider, attackLabel);
    placeKnob(releaseSlider, releaseLabel);
    placeKnob(protectBodySlider, protectBodyLabel);
    placeKnob(transientsSlider, transientsLabel);

    // Mode buttons
    const int btnHeight = 36;
    const int btnGap = 6;
    vocalCleanBtn.setBounds(modeArea.removeFromTop(btnHeight));
    modeArea.removeFromTop(btnGap);
    mixGlueBtn.setBounds(modeArea.removeFromTop(btnHeight));
    modeArea.removeFromTop(btnGap);
    punchBtn.setBounds(modeArea.removeFromTop(btnHeight));

    // Bottom section
    auto bottomArea = bounds.reduced(10, 4).reduced(8, 8);

    auto mixArea = bottomArea.removeFromLeft(90);
    mixLabel.setBounds(mixArea.removeFromBottom(20));
    mixSlider.setBounds(mixArea.reduced(4));

    auto outputArea = bottomArea.removeFromLeft(90);
    outputLabel.setBounds(outputArea.removeFromBottom(20));
    outputSlider.setBounds(outputArea.reduced(4));

    auto deltaArea = bottomArea.removeFromLeft(120);
    deltaBtn.setBounds(deltaArea.withSizeKeepingCentre(100, 30));
}
