/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleStereoGainAdjustAudioProcessorEditor::SimpleStereoGainAdjustAudioProcessorEditor (SimpleStereoGainAdjustAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    gainLeftAttach = std::make_unique<Attach>(audioProcessor.getAPVTS(), "LEFTGAIN", gainLeftSlider);
    gainRightAttach = std::make_unique<Attach>(audioProcessor.getAPVTS(), "RIGHTGAIN", gainRightSlider);
    gainMainAttach = std::make_unique<Attach>(audioProcessor.getAPVTS(), "MAINGAIN", gainMainSlider);
    
    setupSlider(gainLeftSlider);
    setupSlider(gainRightSlider);
    setupSlider(gainMainSlider);

    startTimerHz(60);

    setSize (400, 300);
}

SimpleStereoGainAdjustAudioProcessorEditor::~SimpleStereoGainAdjustAudioProcessorEditor()
{
}

//==============================================================================
void SimpleStereoGainAdjustAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::blanchedalmond);

    auto& volumeLeft = audioProcessor.getLeftChannelVolume();
    auto& volumeRight = audioProcessor.getRightChannelVolume();

    g.fillRect(gainLeftSlider.getRight(), (int)(300.f * (1.0f - volumeLeft.load())), (int)(getWidth() * 0.15f), getHeight());
    g.fillRect(gainMainSlider.getRight(), (int)(300.f * (1.0f - volumeRight.load())), (int)(getWidth() * 0.15f), getHeight());

}

void SimpleStereoGainAdjustAudioProcessorEditor::resized()
{
    gainLeftSlider.setBoundsRelative(0.0f, 0.0f, 0.2f, 1.0f);
    gainRightSlider.setBoundsRelative(0.8f, 0.0f, 0.2f, 1.0f);
    gainMainSlider.setBoundsRelative(0.35f, 0.0f, 0.3f, 1.0f);
}

void SimpleStereoGainAdjustAudioProcessorEditor::setupSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearBarVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    slider.setColour(juce::Slider::ColourIds::thumbColourId, juce::Colours::darkslateblue);
    //slider.setColour(juce::Slider::ColourIds::backgroundColourId, juce::Colours::hotpink);
    slider.setColour(juce::Slider::ColourIds::trackColourId, juce::Colours::lime);
    addAndMakeVisible(slider);
}

void SimpleStereoGainAdjustAudioProcessorEditor::timerCallback()
{
    repaint();
}
