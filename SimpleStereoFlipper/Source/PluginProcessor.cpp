/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleStereoFlipperAudioProcessor::SimpleStereoFlipperAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Adds a single parameter that allows the musician to adjust the Flip Period on a knob.
    addParameter(flipPeriod = new juce::AudioParameterFloat("FLIP PERIOD", "Flip Period", 0.01f, 2.5f, 0.25f));
}

SimpleStereoFlipperAudioProcessor::~SimpleStereoFlipperAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleStereoFlipperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleStereoFlipperAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleStereoFlipperAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleStereoFlipperAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleStereoFlipperAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleStereoFlipperAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleStereoFlipperAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleStereoFlipperAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleStereoFlipperAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleStereoFlipperAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleStereoFlipperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void SimpleStereoFlipperAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleStereoFlipperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SimpleStereoFlipperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    // If it is not on a stereo in and out channel, throw an error and exit.
    if (getNumInputChannels() < 2 || getNumOutputChannels() < 2) { jassertfalse; };

    // Get a Write Pointer for both the Left and Right Channels.
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    // Remember that how long a second is is determined by the Sample Rate when measuring using samples.
    const double sampleRate = getSampleRate();

    // Get the Flip Period knobs value and update the lengthUntilFlip which is the amount of seconds before a flip occurs.
    lengthUntilFlip = flipPeriod->get();

    // Loop through all the samples in the buffer...
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // When it is within the first period, flip the channels.
        if (samplesForThisFlip < lengthUntilFlip * sampleRate)
        {
            // Temporary holds the right channels data
            float tempHolderRight = channelDataR[sample];
            
            // The left channel's data is coppied over to the right's, erasing what was there previously.
            channelDataR[sample] = channelDataL[sample];
            
            // Now add back in the temp Right Channel Sample to the Left Channel, completing the channel flip. 
            channelDataL[sample] = tempHolderRight;

            // Increment the sample count for this flip
            samplesForThisFlip++;
        }
        // When it is within the second period, keep the Left as the Left and the Right as the Right
        else if (samplesForThisFlip < lengthUntilFlip * 2.0f * sampleRate)
        {
            // To illustrate what the algorithm does, I decided to show Left stays Left and Right stays right
            channelDataL[sample] = channelDataL[sample];
            channelDataR[sample] = channelDataR[sample];

            // Still, an increment to the total samples made this cycle is necessary.
            samplesForThisFlip++;
        }
        // Finally if over a double period length in samples, set samplesForThisFlip back to 0
        else
        {
            samplesForThisFlip = 0.0f;
        }
    }
}

//==============================================================================
bool SimpleStereoFlipperAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleStereoFlipperAudioProcessor::createEditor()
{
    // Uses the default JUCE interface which is better than the default DAW editor for testing.
    return new juce::GenericAudioProcessorEditor(this);

    // Creates an Editor object with a reference to the AudioProcessor that it stores internally for intra communication.
    //return new SimpleStereoFlipperAudioProcessorEditor (*this);
}

//==============================================================================
void SimpleStereoFlipperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleStereoFlipperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleStereoFlipperAudioProcessor();
}
