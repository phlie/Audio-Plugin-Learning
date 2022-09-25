/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class SimpleStereoGainAdjustAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                                    public juce::Timer
{
public:
    SimpleStereoGainAdjustAudioProcessorEditor (SimpleStereoGainAdjustAudioProcessor&);
    ~SimpleStereoGainAdjustAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void setupSlider(juce::Slider& slider);

    // This comes from the Timer abstract class and has to be overriden with our own code for when the timer is called.
    void timerCallback() override;

private:
    using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;
    
    juce::Slider gainLeftSlider;
    juce::Slider gainRightSlider;
    juce::Slider gainMainSlider;
    
    std::unique_ptr<Attach> gainLeftAttach;
    std::unique_ptr<Attach> gainRightAttach;
    std::unique_ptr<Attach> gainMainAttach;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleStereoGainAdjustAudioProcessor& audioProcessor;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleStereoGainAdjustAudioProcessorEditor)
};
