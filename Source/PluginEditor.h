#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectralEngine.h"

//==============================================================================
/**  Custom look-and-feel for the SpectralMorpher UI  */
class SpectralLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SpectralLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool isMouseOver, bool isButtonDown) override;

    juce::Font getLabelFont(juce::Label&) override;

private:
    juce::Colour accentColour  { 0xFF00E5FF };  // Cyan
    juce::Colour glowColour    { 0xFF7B2FFF };  // Purple
    juce::Colour bgColour      { 0xFF0A0A14 };
    juce::Colour knobBg        { 0xFF1A1A2E };
};

//==============================================================================
/**  Real-time spectrum analyser component  */
class SpectrumAnalyser : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumAnalyser(SpectralMorpherAudioProcessor& p);
    ~SpectrumAnalyser() override;

    void paint(juce::Graphics&) override;
    void timerCallback() override;

private:
    SpectralMorpherAudioProcessor& processor;
    std::vector<float> inputMags;
    std::vector<float> morphedMags;

    juce::Colour inputColour  { 0x8000BFFF };   // semi-transparent blue
    juce::Colour morphColour  { 0xBF7B2FFF };   // semi-transparent purple

    static constexpr int NUM_DISPLAY_BINS = 256;
    std::vector<float> smoothInput;
    std::vector<float> smoothMorph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyser)
};

//==============================================================================
/**  A labelled knob with APVTS attachment  */
class LabelledKnob : public juce::Component
{
public:
    LabelledKnob(const juce::String& labelText,
                 const juce::String& paramID,
                 juce::AudioProcessorValueTreeState& apvts,
                 juce::LookAndFeel& laf);

    void resized() override;
    juce::Slider& getSlider() { return slider; }

private:
    juce::Slider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label  label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabelledKnob)
};

//==============================================================================
/**  Main editor window  */
class SpectralMorpherAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SpectralMorpherAudioProcessorEditor(SpectralMorpherAudioProcessor&);
    ~SpectralMorpherAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SpectralMorpherAudioProcessor& audioProcessor;
    SpectralLookAndFeel laf;

    // Spectrum visualiser
    SpectrumAnalyser spectrumAnalyser;

    // Knobs
    LabelledKnob morphKnob;
    LabelledKnob shiftKnob;
    LabelledKnob tiltKnob;
    LabelledKnob harmonicsKnob;
    LabelledKnob mixKnob;
    LabelledKnob gainKnob;

    // Freeze toggle
    juce::ToggleButton freezeButton { "FREEZE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;

    // Title
    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralMorpherAudioProcessorEditor)
};
