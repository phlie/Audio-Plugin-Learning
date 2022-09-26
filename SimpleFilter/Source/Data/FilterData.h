/*
  ==============================================================================

    FilterData.h
    Created: 25 Sep 2022 4:34:28pm
    Author:  phlie

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class FilterData
{
public:
    void prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();
    void updateParameters(const int frequency, const float resonance);

private:
    juce::dsp::StateVariableFilter::Filter<float> filter;
    bool isPrepared{ false };
    double hostSampleRate{ 0.0f };
};