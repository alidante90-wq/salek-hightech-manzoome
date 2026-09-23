#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "WavetableOscillator.h"

namespace salek
{

/** Accepts any MIDI note/channel - single sound, poly voices handle the rest. */
class SalekSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override        { return true; }
    bool appliesToChannel (int) override     { return true; }
};

/** Shared, atomically-readable parameter block every voice reads from each block. */
struct VoiceParams
{
    std::atomic<float>* framePosition = nullptr;   // 0-1 wavetable frame morph
    std::atomic<float>* warpAmount    = nullptr;    // 0-1
    std::atomic<int>*   warpModeIndex = nullptr;    // matches WarpMode enum order
    std::atomic<float>* unisonVoices  = nullptr;    // 1-16 (stored as float param)
    std::atomic<float>* unisonDetune  = nullptr;    // cents
    std::atomic<float>* attack        = nullptr;
    std::atomic<float>* decay         = nullptr;
    std::atomic<float>* sustain       = nullptr;
    std::atomic<float>* release       = nullptr;
    std::atomic<float>* masterGain    = nullptr;
};

class SalekVoice : public juce::SynthesiserVoice
{
public:
    explicit SalekVoice (const Wavetable& table, const VoiceParams& p)
        : wavetable (table), params (p)
    {
    }

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SalekSound*> (s) != nullptr;
    }

    void setCurrentPlaybackSampleRate (double newRate) override
    {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
        osc.prepare (newRate);
        osc.setWavetable (&wavetable);
        adsr.setSampleRate (newRate);
    }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        currentVelocity = velocity;
        const float hz = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        osc.setFrequency (hz);
        osc.reset();

        juce::ADSR::Parameters ap;
        ap.attack  = params.attack  ? params.attack->load()  : 0.01f;
        ap.decay   = params.decay   ? params.decay->load()   : 0.15f;
        ap.sustain = params.sustain ? params.sustain->load() : 0.8f;
        ap.release = params.release ? params.release->load() : 0.25f;
        adsr.setParameters (ap);
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            clearCurrentNote();
            adsr.reset();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (! adsr.isActive() && ! adsr.getParameters().attack) {} // no-op guard

        osc.setFramePosition (params.framePosition ? params.framePosition->load() : 0.0f);
        osc.setWarpAmount    (params.warpAmount    ? params.warpAmount->load()    : 0.0f);
        osc.setWarpMode (static_cast<WarpMode> (params.warpModeIndex ? params.warpModeIndex->load() : 0));
        osc.setUnisonVoices ((int) (params.unisonVoices ? params.unisonVoices->load() : 1.0f));
        osc.setUnisonDetuneCents (params.unisonDetune ? params.unisonDetune->load() : 8.0f);

        const float gain = (params.masterGain ? params.masterGain->load() : 0.8f) * currentVelocity;

        for (int i = 0; i < numSamples; ++i)
        {
            if (! adsr.isActive())
            {
                clearCurrentNote();
                break;
            }

            const float env = adsr.getNextSample();
            const float sample = osc.renderSample() * env * gain;

            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
                outputBuffer.addSample (ch, startSample + i, sample);
        }
    }

private:
    const Wavetable& wavetable;
    const VoiceParams& params;
    WavetableOscillator osc;
    juce::ADSR adsr;
    float currentVelocity = 1.0f;
};

} // namespace salek
