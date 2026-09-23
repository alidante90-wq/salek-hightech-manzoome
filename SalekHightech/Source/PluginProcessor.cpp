#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace salek;

SalekHightechAudioProcessor::SalekHightechAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    voiceParams.framePosition = apvts.getRawParameterValue ("osc1_frame");
    voiceParams.warpAmount    = apvts.getRawParameterValue ("osc1_warp_amount");
    voiceParams.unisonVoices  = apvts.getRawParameterValue ("osc1_unison");
    voiceParams.unisonDetune  = apvts.getRawParameterValue ("osc1_detune");
    voiceParams.attack        = apvts.getRawParameterValue ("env_attack");
    voiceParams.decay         = apvts.getRawParameterValue ("env_decay");
    voiceParams.sustain       = apvts.getRawParameterValue ("env_sustain");
    voiceParams.release       = apvts.getRawParameterValue ("env_release");
    voiceParams.masterGain    = apvts.getRawParameterValue ("master_gain");
    // warp mode is a choice param stored as float index; SynthVoice casts it,
    // so bridge it through an int-valued atomic pulled fresh each block.
    static std::atomic<int> warpModeCache { 0 };
    voiceParams.warpModeIndex = &warpModeCache;

    synth.addSound (new SalekSound());
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new SalekVoice (wavetable, voiceParams));
}

juce::AudioProcessorValueTreeState::ParameterLayout SalekHightechAudioProcessor::createParameterLayout()
{
    using Param = juce::AudioParameterFloat;
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<Param> ("osc1_frame", "OSC1 Frame", Range (0.0f, 1.0f), 0.0f));
    params.push_back (std::make_unique<Param> ("osc1_warp_amount", "OSC1 Warp Amount", Range (0.0f, 1.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("osc1_warp_mode", "OSC1 Warp Mode",
        juce::StringArray { "Off", "Fold Sym", "Fold Asym", "Phase Distort", "Bend", "Sync" }, 0));
    params.push_back (std::make_unique<Param> ("osc1_unison", "OSC1 Unison Voices", Range (1.0f, 8.0f, 1.0f), 1.0f));
    params.push_back (std::make_unique<Param> ("osc1_detune", "OSC1 Unison Detune", Range (0.0f, 50.0f), 8.0f));

    params.push_back (std::make_unique<Param> ("env_attack",  "Amp Attack",  Range (0.001f, 5.0f, 0.0f, 0.3f), 0.01f));
    params.push_back (std::make_unique<Param> ("env_decay",   "Amp Decay",   Range (0.001f, 5.0f, 0.0f, 0.3f), 0.15f));
    params.push_back (std::make_unique<Param> ("env_sustain", "Amp Sustain", Range (0.0f, 1.0f), 0.8f));
    params.push_back (std::make_unique<Param> ("env_release", "Amp Release", Range (0.001f, 5.0f, 0.0f, 0.3f), 0.25f));

    params.push_back (std::make_unique<Param> ("master_gain", "Master Gain", Range (0.0f, 1.5f), 0.8f));

    return { params.begin(), params.end() };
}

void SalekHightechAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    wavetable = Wavetable::makeDefaultAnalogStack (sampleRate);
    synth.setCurrentPlaybackSampleRate (sampleRate);
}

void SalekHightechAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Sync the warp-mode choice param into the atomic int the voices read.
    if (auto* p = apvts.getParameter ("osc1_warp_mode"))
        voiceParams.warpModeIndex->store ((int) p->convertFrom0to1 (p->getValue()));

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* SalekHightechAudioProcessor::createEditor()
{
    return new SalekHightechAudioProcessorEditor (*this);
}

void SalekHightechAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void SalekHightechAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

bool SalekHightechAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SalekHightechAudioProcessor();
}
