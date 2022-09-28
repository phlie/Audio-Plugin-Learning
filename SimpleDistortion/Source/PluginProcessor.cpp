/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleDistortionAudioProcessor::SimpleDistortionAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "PARAMETERS", createParameters())
#endif
{
}

SimpleDistortionAudioProcessor::~SimpleDistortionAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleDistortionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleDistortionAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleDistortionAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleDistortionAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleDistortionAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleDistortionAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleDistortionAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleDistortionAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleDistortionAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleDistortionAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleDistortionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void SimpleDistortionAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleDistortionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleDistortionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    float threshold = apvts.getRawParameterValue("THRESHOLD")->load();
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        // Get a write pointer to the current channel
        auto* channelData = buffer.getWritePointer (channel);

        // Increment over all the samples in the buffer.
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // Call the Wave Folder algorithm on the sample and save it back.
            channelData[sample] = waveFolder(channelData[sample], threshold);
        }

            /*// Store the input temporarily
            float input = channelData[sample];

            // If the input is greater than the threshold, fold...
            if (input > threshold)
            {
                // This means first calculate the part above the threshold
                float aboveThreshold = input - threshold;

                // The make it that amount below the threshold
                channelData[sample] = threshold - aboveThreshold;
            }
            // If the signal is over in the negative direction
            else if (input < -threshold)
            {
                // Calculate the amount below the negative of the threshold, or input - (-threshold)
                float belowThreshold = input + threshold;

                // Then it is the same equation as before with threshold negative and belowThreshold while be positive.
                channelData[sample] = -threshold - belowThreshold; 
            }
            else
            {
                // No folding has occured.
                channelData[sample] = input;
            } */
    }
}

//==============================================================================
bool SimpleDistortionAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleDistortionAudioProcessor::createEditor()
{
    // Used instead to display the apvts by default
    return new juce::GenericAudioProcessorEditor(this);
    //return new SimpleDistortionAudioProcessorEditor (*this);
}

//==============================================================================
void SimpleDistortionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleDistortionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleDistortionAudioProcessor::createParameters()
{
    // Creates a variable to store the layout
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Adds an AudioParameterFloat to control the threshold value.
    layout.add(std::make_unique<juce::AudioParameterFloat>("THRESHOLD", "Threshold", 0.0f, 1.0f, 1.0f));

    return layout;
}

float SimpleDistortionAudioProcessor::waveFolder(float& signal, float& threshold)
{
    // Start assuming it is positive
    bool isPositive = true;

    // If the signal is negative, convert it to positive and erase the isPositive flag
    if (signal < 0.0f)
    { 
        isPositive = false;
        signal = -signal;
    }

    // t = 0.75 s = 1.0    t = 0.2  s = 0.9 i = 5 total Used = 4 * 0.2 = 0.8 total left = 0.1 

    // The loop starts at increment 0
    int i = 0;

    // Until it is on the signals last bouncy journey.
    while (i * threshold < signal)
    {
        // Increment the counter
        i++;
    }

    // It was the previous loop index that is the most that can be taken away.
    i--;

    // The output defaults to 0
    float output = 0.0f;

    // The total used is the total lengths of the trips between 0 and the threshold minus the last trip.
    auto totalUsed = (i * threshold);

    // Then the total left over is easily calculated by minusing the total length of the journey.
    auto totalLeft = signal - totalUsed;

    // If it is a even number, it is just the difference between 0 and the total left.
    if (i % 2 == 0)
    {
        output = totalLeft;
    }

    // If it has made an odd number of trips, it is the distance towards 0 from the threshold.
    else
    {
        output = threshold - totalLeft;
    }

    // Finally, if it is a positive return it, and if it is a negative number convert it back to negative.
    return isPositive ? output : -output;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleDistortionAudioProcessor();
}
