/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleReverbAudioProcessor::SimpleReverbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
         // Create an instance of the AudioProcessorValueTreeState
                       ), apvts(*this, nullptr, "PARAMETERS", createParams())
#endif
{
}

SimpleReverbAudioProcessor::~SimpleReverbAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleReverbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleReverbAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleReverbAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleReverbAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleReverbAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleReverbAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleReverbAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleReverbAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleReverbAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleReverbAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleReverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Create a ProcessSpec struct to hold the data needed for the reverbs Prepare function
    juce::dsp::ProcessSpec spec;
    
    // Define the 3 variables that a ProcessSpec can hold.
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getNumInputChannels();

    // Prepare the reverb for play.
    reverb.prepare(spec);



    setSampleRate = sampleRate;

    // Sets how many samples behind the write head the read head is. Start at its max value
    readHeadDelaySamples = sampleRate * maxDelayTimeSeconds;

    // Sets the size of the circular buffer. As 4 times the maxDelay length.
    circleBuffer.setSize(getNumInputChannels(), maxDelayTimeSeconds * sampleRate * 2, false, false, true);

    // Clear the contents of the circleBuffer for now
    circleBuffer.clear();

}

void SimpleReverbAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SimpleReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    // Create a place to hold the reverb parameters
    juce::dsp::Reverb::Parameters reverbParams;

    // Get the values of all the Reverb Params.
    reverbParams.damping = apvts.getRawParameterValue("DAMPING")->load();
    reverbParams.dryLevel = apvts.getRawParameterValue("DRY")->load();
    reverbParams.freezeMode = apvts.getRawParameterValue("FREEZE")->load();
    reverbParams.roomSize = apvts.getRawParameterValue("SIZE")->load();
    reverbParams.wetLevel = apvts.getRawParameterValue("WET")->load();
    reverbParams.width = apvts.getRawParameterValue("WIDTH")->load();
    
    // Set the reverb params.
    reverb.setParameters(reverbParams);

    float mix = apvts.getRawParameterValue("MIX")->load();
    float delayLine = apvts.getRawParameterValue("DELAYLINE")->load();
    float feedback = apvts.getRawParameterValue("FEEDBACK")->load();

    //readHeadDelaySamples = (int)((float)setSampleRate * delayLine);
    readHeadDelaySamples = 1000;

    for (int channel = 0; channel < getNumInputChannels(); ++channel)
    {
        // The two buffers for the respective data.
        auto incomingData = buffer.getWritePointer(channel);
        auto circleRead = circleBuffer.getReadPointer(channel);
        auto circleWrite = circleBuffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // The clean signal coming through.
            float cleanSignal = incomingData[sample];

            // Get the current readPosition which is the current position of the circle buffer write head - the delay in samples
            int readPos = writeHeadSamplePosition - readHeadDelaySamples;

            // If the read position is in the negatives, its actual position is simply the length of the buffer minus its magnitute
            if (readPos < 0)
                readPos = circleBuffer.getNumSamples() + readPos;

            // The total signal is just the feedback + the current incoming audio. With the feedback being channgable.
            // Divide by the total max volume of the feedback data and incoming data
            float signal = (feedback * circleRead[readPos]);// + incomingData[sample]) / (1.0f + feedback);

            // The value to output is just the signal with an option to change the wet/dry
            incomingData[sample] = signal; //* mix + cleanSignal * (1.0f - mix);

            // The value at the Circle buffer read head position is the same.
            circleWrite[writeHeadSamplePosition] = incomingData[sample];

            // Increment the write head position.
            writeHeadSamplePosition++;

            // If the Write Head Position is the same size as the circleBuffer after incrementing, return it back to 0.
            if (writeHeadSamplePosition >= circleBuffer.getNumSamples())
                writeHeadSamplePosition = 0;
        }
    }
    // Creats an instance of an AudioBlock of type float out of the buffer object
    juce::dsp::AudioBlock<float> block{ buffer };
    
    // Sets up the context to use which is of type float and relys on block.
    auto contextToUse = juce::dsp::ProcessContextReplacing<float>(block);

    // Get the reverb to process the data in this block
    reverb.process(contextToUse);
}

//==============================================================================
bool SimpleReverbAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleReverbAudioProcessor::createEditor()
{
    // Temporary front end that just works.
    return new juce::GenericAudioProcessorEditor(this);
    //return new SimpleReverbAudioProcessorEditor (*this);
}

//==============================================================================
void SimpleReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    // Gets the state of the ValueTree
    auto state = apvts.copyState();
    
    // Creates a unique pointer of type XmlElement called xml and initialized with the apvts state
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // Converts Xml to a binary blob.
    copyXmlToBinary(*xml, destData);
}

void SimpleReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    // A unique ptr that gets the Xml Data from the Binary
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    // As long as there is information in the xmlState
    if (xmlState.get() != nullptr)
        
        // If the state has the tag name associated with apvts
        if (xmlState->hasTagName(apvts.state.getType()))
            
            // Changes the state of the Value Tree State to what ever was saved.
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleReverbAudioProcessor::createParams()
{
    // Create an instance of the ParameterLayout
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Add all the required knobs that are capable of being changed on the reverb.
    layout.add(std::make_unique<juce::AudioParameterFloat>("DRY", "Dry", 0.0f, 1.0f, 0.75f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("WET", "Wet", 0.0f, 1.0f, 0.25f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("SIZE", "Size", 0.0, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("DAMPING", "Damping", 0.0, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("WIDTH", "Width", 0.0, 1.0f, 0.75f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("FREEZE", "Freeze", 0.0, 1.0f, 0.0f));

    // Parameters for the Comb Reverb
    layout.add(std::make_unique<juce::AudioParameterFloat>("MIX", "Mix", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("DELAYLINE", "Delay Line", 0.00f, maxDelayTimeSeconds, 0.05f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("FEEDBACK", "Feedback", 0.0f, 1.0f, 0.99f));

    // Return the parameter layout.
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleReverbAudioProcessor();
}
