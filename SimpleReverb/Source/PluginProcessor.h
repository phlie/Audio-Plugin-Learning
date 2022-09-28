/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class SimpleReverbAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SimpleReverbAudioProcessor();
    ~SimpleReverbAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    // The default reverb supplied within the DSP framework
    juce::dsp::Reverb reverb;


    // A circular buffer meant to hold the previous data.
    juce::AudioBuffer<float> circleBuffer;

    // Sample Rate
    float setSampleRate{ 44800.0f };

    // How long the read heads delay is behind the write head in samples.
    float delayTimeSeconds{ 0.03f };

    // This the is the maximum setable delay.
    const float maxDelayTimeSeconds{ 0.1f };

    // How far behind the read head is in samples
    int readHeadDelaySamples{ 0 };

    // The current position of the write head.
    int writeHeadSamplePosition{ 0 };

    // The amount of the read signal that is applied to the current signal.
    float combFeedback{ 0.92f };

    // The ValueTreeState object
    juce::AudioProcessorValueTreeState apvts;

    // Creates a Parameter Layout for the Value Tree State
    juce::AudioProcessorValueTreeState::ParameterLayout createParams();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleReverbAudioProcessor)
};
