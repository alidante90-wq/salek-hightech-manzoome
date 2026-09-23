#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
    PLACEHOLDER EDITOR.
    This exists so the plugin is playable and every parameter is real and
    automatable right now. The full SALEK HIGHTECH neon/cyberpunk UI (mod
    matrix, wavetable display, macros, FX rack, etc.) replaces this in a
    later step - nothing here is meant to be the final look.
*/
class SalekHightechAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SalekHightechAudioProcessorEditor (SalekHightechAudioProcessor&);
    ~SalekHightechAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SalekHightechAudioProcessor& processor;

    juce::Label title { "title", "SALEK HIGHTECH  (engine online - UI pending)" };

    juce::Slider frameSlider, warpAmountSlider, unisonSlider, detuneSlider,
                 attackSlider, decaySlider, sustainSlider, releaseSlider, gainSlider;
    juce::ComboBox warpModeBox;

    juce::Label frameLabel   { {}, "Frame" };
    juce::Label warpLabel    { {}, "Warp Amt" };
    juce::Label modeLabel    { {}, "Warp Mode" };
    juce::Label unisonLabel  { {}, "Unison" };
    juce::Label detuneLabel  { {}, "Detune" };
    juce::Label attackLabel  { {}, "Attack" };
    juce::Label decayLabel   { {}, "Decay" };
    juce::Label sustainLabel { {}, "Sustain" };
    juce::Label releaseLabel { {}, "Release" };
    juce::Label gainLabel    { {}, "Gain" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> frameAtt, warpAmountAtt, unisonAtt, detuneAtt,
                                       attackAtt, decayAtt, sustainAtt, releaseAtt, gainAtt;
    std::unique_ptr<ComboAttachment> warpModeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SalekHightechAudioProcessorEditor)
};
