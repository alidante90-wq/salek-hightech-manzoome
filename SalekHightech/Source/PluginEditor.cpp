#include "PluginEditor.h"

namespace
{
    void setupRotary (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    }
}

SalekHightechAudioProcessorEditor::SalekHightechAudioProcessorEditor (SalekHightechAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    setSize (640, 360);

    title.setJustificationType (juce::Justification::centred);
    title.setFont (juce::Font (20.0f, juce::Font::bold));
    addAndMakeVisible (title);

    auto& apvts = processor.apvts;

    auto bind = [this, &apvts] (juce::Slider& slider, juce::Label& label, const juce::String& paramID,
                                 std::unique_ptr<SliderAttachment>& att)
    {
        setupRotary (slider);
        addAndMakeVisible (slider);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
        att = std::make_unique<SliderAttachment> (apvts, paramID, slider);
    };

    bind (frameSlider, frameLabel, "osc1_frame", frameAtt);
    bind (warpAmountSlider, warpLabel, "osc1_warp_amount", warpAmountAtt);
    bind (unisonSlider, unisonLabel, "osc1_unison", unisonAtt);
    bind (detuneSlider, detuneLabel, "osc1_detune", detuneAtt);
    bind (attackSlider, attackLabel, "env_attack", attackAtt);
    bind (decaySlider, decayLabel, "env_decay", decayAtt);
    bind (sustainSlider, sustainLabel, "env_sustain", sustainAtt);
    bind (releaseSlider, releaseLabel, "env_release", releaseAtt);
    bind (gainSlider, gainLabel, "master_gain", gainAtt);

    warpModeBox.addItemList ({ "Off", "Fold Sym", "Fold Asym", "Phase Distort", "Bend", "Sync" }, 1);
    addAndMakeVisible (warpModeBox);
    addAndMakeVisible (modeLabel);
    modeLabel.setJustificationType (juce::Justification::centred);
    warpModeAtt = std::make_unique<ComboAttachment> (apvts, "osc1_warp_mode", warpModeBox);
}

void SalekHightechAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff14071f));
}

void SalekHightechAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    title.setBounds (area.removeFromTop (30));

    juce::Slider* sliders[] = { &frameSlider, &warpAmountSlider, &unisonSlider, &detuneSlider,
                                 &attackSlider, &decaySlider, &sustainSlider, &releaseSlider, &gainSlider };
    juce::Label* labels[] = { &frameLabel, &warpLabel, &unisonLabel, &detuneLabel,
                               &attackLabel, &decayLabel, &sustainLabel, &releaseLabel, &gainLabel };

    const int cols = 5;
    const int cellW = area.getWidth() / cols;
    const int cellH = 130;

    for (int i = 0; i < 9; ++i)
    {
        auto cell = juce::Rectangle<int> (area.getX() + (i % cols) * cellW,
                                           area.getY() + (i / cols) * cellH,
                                           cellW, cellH);
        labels[(size_t) i]->setBounds (cell.removeFromTop (18));
        sliders[(size_t) i]->setBounds (cell.reduced (4));
    }

    warpModeBox.setBounds (area.getX() + 4 * cellW, area.getY() + cellH + 18, cellW - 8, 24);
    modeLabel.setBounds (area.getX() + 4 * cellW, area.getY() + cellH, cellW - 8, 18);
}
