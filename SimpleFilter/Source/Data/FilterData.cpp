/*
  ==============================================================================

    FilterData.cpp
    Created: 25 Sep 2022 4:34:28pm
    Author:  phlie

  ==============================================================================
*/

#include "FilterData.h"

void FilterData::prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels)
{
    filter.reset();

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    hostSampleRate = sampleRate;
    spec.numChannels = numChannels;
    filter.prepare(spec);

    isPrepared = true;
}

void FilterData::process(juce::AudioBuffer<float>& buffer)
{
    jassert(isPrepared);
    juce::dsp::AudioBlock<float> block{ buffer };
    filter.process(juce::dsp::ProcessContextReplacing<float>{ block });
}

void FilterData::reset()
{
    filter.reset();
}

void FilterData::updateParameters(const int frequency, const float resonance)
{
    filter.parameters->type = juce::dsp::StateVariableFilter::StateVariableFilterType::lowPass;
    filter.parameters->setCutOffFrequency(hostSampleRate, frequency, resonance);
}
