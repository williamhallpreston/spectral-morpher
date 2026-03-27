#include "PluginEditor.h"

//==============================================================================
// SpectralLookAndFeel
//==============================================================================
SpectralLookAndFeel::SpectralLookAndFeel()
{
    setColour(juce::Slider::thumbColourId,            accentColour);
    setColour(juce::Slider::rotarySliderFillColourId, accentColour);
    setColour(juce::Slider::rotarySliderOutlineColourId, knobBg);
    setColour(juce::Slider::textBoxTextColourId,      accentColour.withAlpha(0.85f));
    setColour(juce::Slider::textBoxOutlineColourId,   juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, knobBg);
    setColour(juce::Label::textColourId,              juce::Colour(0xFFAABBCC));
    setColour(juce::ToggleButton::textColourId,       accentColour);
    setColour(juce::ToggleButton::tickColourId,       accentColour);
    setColour(juce::ToggleButton::tickDisabledColourId, accentColour.withAlpha(0.4f));
}

void SpectralLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                            int x, int y, int w, int h,
                                            float sliderPos,
                                            float startAngle, float endAngle,
                                            juce::Slider& /*slider*/)
{
    const float radius   = juce::jmin(w, h) * 0.5f - 4.0f;
    const float centreX  = x + w * 0.5f;
    const float centreY  = y + h * 0.5f;
    const float angle    = startAngle + sliderPos * (endAngle - startAngle);

    // Background circle
    g.setColour(knobBg);
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Outer ring track
    juce::Path track;
    track.addCentredArc(centreX, centreY, radius - 1.5f, radius - 1.5f,
                        0.0f, startAngle, endAngle, true);
    g.setColour(juce::Colour(0xFF2A2A3E));
    g.strokePath(track, juce::PathStrokeType(3.0f));

    // Filled arc (value indicator)
    juce::Path arc;
    arc.addCentredArc(centreX, centreY, radius - 1.5f, radius - 1.5f,
                      0.0f, startAngle, angle, true);
    g.setColour(accentColour);
    g.strokePath(arc, juce::PathStrokeType(3.0f,
                 juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Glow effect on arc
    g.setColour(accentColour.withAlpha(0.25f));
    g.strokePath(arc, juce::PathStrokeType(7.0f,
                 juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer line
    juce::Path pointer;
    const float pointerLen = radius * 0.55f;
    pointer.addRectangle(-1.5f, -radius, 3.0f, pointerLen);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(accentColour);
    g.fillPath(pointer);

    // Centre dot
    g.setColour(glowColour);
    g.fillEllipse(centreX - 4.0f, centreY - 4.0f, 8.0f, 8.0f);
    g.setColour(accentColour.withAlpha(0.6f));
    g.fillEllipse(centreX - 2.5f, centreY - 2.5f, 5.0f, 5.0f);
}

void SpectralLookAndFeel::drawToggleButton(juce::Graphics& g,
                                            juce::ToggleButton& button,
                                            bool isMouseOver, bool /*isButtonDown*/)
{
    const auto bounds  = button.getLocalBounds().toFloat();
    const bool toggled = button.getToggleState();

    // Pill background
    const float corner = bounds.getHeight() * 0.4f;
    g.setColour(toggled ? accentColour.withAlpha(0.25f)
                        : juce::Colour(0xFF1A1A2E));
    g.fillRoundedRectangle(bounds.reduced(2.0f), corner);

    // Border
    g.setColour(toggled ? accentColour : accentColour.withAlpha(0.35f));
    g.drawRoundedRectangle(bounds.reduced(2.0f), corner,
                           toggled ? 2.0f : 1.0f);

    // Text
    g.setColour(toggled ? accentColour : accentColour.withAlpha(0.55f));
    g.setFont(juce::FontOptions("Arial", 11.0f, juce::Font::bold));
    g.drawFittedText(button.getButtonText(), bounds.toNearestInt(),
                     juce::Justification::centred, 1);

    // Glow when active
    if (toggled)
    {
        g.setColour(accentColour.withAlpha(0.15f));
        g.fillRoundedRectangle(bounds, corner + 2.0f);
    }
    if (isMouseOver)
        g.fillAll(juce::Colours::white.withAlpha(0.03f));
}

juce::Font SpectralLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::Font(juce::FontOptions("Arial", 10.0f, juce::Font::plain));
}

//==============================================================================
// SpectrumAnalyser
//==============================================================================
SpectrumAnalyser::SpectrumAnalyser(SpectralMorpherAudioProcessor& p)
    : processor(p)
{
    smoothInput .assign(NUM_DISPLAY_BINS, 0.0f);
    smoothMorph .assign(NUM_DISPLAY_BINS, 0.0f);
    startTimerHz(30);
}

SpectrumAnalyser::~SpectrumAnalyser() { stopTimer(); }

void SpectrumAnalyser::timerCallback()
{
    processor.getMagnitudeSpectrum();  // warm cache
    processor.getMorphedSpectrum();
    repaint();
}

void SpectrumAnalyser::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float W = bounds.getWidth();
    const float H = bounds.getHeight();

    // Background
    g.setColour(juce::Colour(0xFF080812));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Grid lines
    g.setColour(juce::Colour(0xFF1E1E30));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine((int)(H * i / 4.0f), 0.0f, W);
    for (int db : { -12, -24, -36, -48 })
    {
        const float yPos = juce::jmap((float)db, -60.0f, 0.0f, H, 0.0f);
        g.setFont(8.0f);
        g.setColour(juce::Colour(0xFF2A3050));
        g.drawText(juce::String(db) + "dB", 4, (int)yPos - 8, 30, 12,
                   juce::Justification::left);
    }

    // Fetch spectrum data
    std::vector<float> rawInput, rawMorph;
    processor.getMagnitudeSpectrum(); // updates internal cache
    rawInput = processor.getMagnitudeSpectrum();
    rawMorph = processor.getMorphedSpectrum();

    if (rawInput.empty() || rawMorph.empty()) return;

    const int totalBins = (int)rawInput.size();

    // Downsample to display bins (log scale)
    auto getLogBin = [&](int displayBin, const std::vector<float>& src) -> float
    {
        const float minLog = std::log2(1.0f);
        const float maxLog = std::log2((float)totalBins);
        const float logPos = minLog + (float)displayBin / NUM_DISPLAY_BINS * (maxLog - minLog);
        const int   binIdx = juce::jlimit(0, totalBins - 1, (int)std::pow(2.0f, logPos));
        return src[binIdx];
    };

    auto drawSpectrum = [&](const std::vector<float>& smooth, juce::Colour colour)
    {
        juce::Path path;
        bool started = false;
        for (int i = 0; i < NUM_DISPLAY_BINS; ++i)
        {
            const float mag  = smooth[i];
            const float dB   = juce::jlimit(-60.0f, 0.0f,
                                juce::Decibels::gainToDecibels(mag + 1e-9f));
            const float yPos = juce::jmap(dB, -60.0f, 0.0f, H, 0.0f);
            const float xPos = W * (float)i / NUM_DISPLAY_BINS;

            if (!started) { path.startNewSubPath(xPos, H); started = true; }
            path.lineTo(xPos, yPos);
        }
        path.lineTo(W, H);
        path.closeSubPath();

        // Filled gradient
        juce::ColourGradient grad(colour.withAlpha(0.35f), 0, 0,
                                  colour.withAlpha(0.0f),  0, H, false);
        g.setGradientFill(grad);
        g.fillPath(path);

        // Stroke
        g.setColour(colour.withAlpha(0.85f));
        juce::Path stroke;
        for (int i = 0; i < NUM_DISPLAY_BINS; ++i)
        {
            const float mag  = smooth[i];
            const float dB   = juce::jlimit(-60.0f, 0.0f,
                                juce::Decibels::gainToDecibels(mag + 1e-9f));
            const float yPos = juce::jmap(dB, -60.0f, 0.0f, H, 0.0f);
            const float xPos = W * (float)i / NUM_DISPLAY_BINS;
            if (i == 0) stroke.startNewSubPath(xPos, yPos);
            else        stroke.lineTo(xPos, yPos);
        }
        g.strokePath(stroke, juce::PathStrokeType(1.5f));
    };

    // Smooth for display
    for (int i = 0; i < NUM_DISPLAY_BINS; ++i)
    {
        const float alpha = 0.25f;
        smoothInput[i] += alpha * (getLogBin(i, rawInput)  - smoothInput[i]);
        smoothMorph[i] += alpha * (getLogBin(i, rawMorph)  - smoothMorph[i]);
    }

    drawSpectrum(smoothInput, inputColour);
    drawSpectrum(smoothMorph, morphColour);

    // Legend
    g.setFont(9.0f);
    g.setColour(inputColour.withAlpha(0.9f));
    g.drawText("INPUT",  8,  8, 40, 12, juce::Justification::left);
    g.setColour(morphColour.withAlpha(0.9f));
    g.drawText("MORPH", 54, 8, 45, 12, juce::Justification::left);

    // Border
    g.setColour(juce::Colour(0xFF00E5FF).withAlpha(0.2f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
}

//==============================================================================
// LabelledKnob
//==============================================================================
LabelledKnob::LabelledKnob(const juce::String& labelText,
                            const juce::String& paramID,
                            juce::AudioProcessorValueTreeState& apvts,
                            juce::LookAndFeel& laf)
{
    slider.setLookAndFeel(&laf);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions("Arial", 10.5f, juce::Font::plain)));
    label.setColour(juce::Label::textColourId, juce::Colour(0xFF7899BB));
    addAndMakeVisible(label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, paramID, slider);
}

void LabelledKnob::resized()
{
    auto area = getLocalBounds();
    label .setBounds(area.removeFromBottom(18));
    slider.setBounds(area);
}

//==============================================================================
// SpectralMorpherAudioProcessorEditor
//==============================================================================
SpectralMorpherAudioProcessorEditor::SpectralMorpherAudioProcessorEditor(
    SpectralMorpherAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      spectrumAnalyser(p),
      morphKnob    ("MORPH",     "morphAmount",   p.getAPVTS(), laf),
      shiftKnob    ("SHIFT",     "spectralShift", p.getAPVTS(), laf),
      tiltKnob     ("TILT",      "tilt",          p.getAPVTS(), laf),
      harmonicsKnob("HARMONICS", "harmonics",     p.getAPVTS(), laf),
      mixKnob      ("MIX",       "mix",           p.getAPVTS(), laf),
      gainKnob     ("GAIN",      "outputGain",    p.getAPVTS(), laf)
{
    setLookAndFeel(&laf);
    setSize(640, 460);
    setResizable(true, true);
    setResizeLimits(480, 360, 1280, 720);

    // Title
    titleLabel.setText("SPECTRAL MORPHER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", 18.0f, juce::Font::bold)));
    titleLabel.setJustificationType(juce::Justification::left);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF00E5FF));
    addAndMakeVisible(titleLabel);

    // Freeze button
    freezeButton.setLookAndFeel(&laf);
    addAndMakeVisible(freezeButton);
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.getAPVTS(), "freeze", freezeButton);

    addAndMakeVisible(spectrumAnalyser);
    addAndMakeVisible(morphKnob);
    addAndMakeVisible(shiftKnob);
    addAndMakeVisible(tiltKnob);
    addAndMakeVisible(harmonicsKnob);
    addAndMakeVisible(mixKnob);
    addAndMakeVisible(gainKnob);
}

SpectralMorpherAudioProcessorEditor::~SpectralMorpherAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void SpectralMorpherAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Dark gradient background
    juce::ColourGradient bg(juce::Colour(0xFF0A0A18), 0, 0,
                            juce::Colour(0xFF0D0820), 0, (float)getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    // Subtle top accent bar
    g.setColour(juce::Colour(0xFF00E5FF).withAlpha(0.15f));
    g.fillRect(0, 0, getWidth(), 3);

    // Separator line above knobs
    const int knobTop = getHeight() - 160;
    g.setColour(juce::Colour(0xFF00E5FF).withAlpha(0.12f));
    g.drawHorizontalLine(knobTop - 10, 16.0f, (float)getWidth() - 16.0f);

    // Version watermark
    g.setFont(9.0f);
    g.setColour(juce::Colour(0xFF334466));
    g.drawText("v1.0 | SPECTRAL MORPHER", getWidth() - 160, getHeight() - 18, 155, 14,
               juce::Justification::right);
}

void SpectralMorpherAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);

    // Header
    auto headerArea = area.removeFromTop(28);
    titleLabel   .setBounds(headerArea.removeFromLeft(250));
    freezeButton .setBounds(headerArea.removeFromRight(90).reduced(0, 2));

    area.removeFromTop(8);

    // Spectrum analyser: takes most of vertical space
    const int knobHeight = 148;
    spectrumAnalyser.setBounds(area.removeFromTop(area.getHeight() - knobHeight - 8));

    area.removeFromTop(12);

    // Knob row at the bottom
    const int numKnobs = 6;
    const int knobW    = area.getWidth() / numKnobs;
    for (auto* k : { &morphKnob, &shiftKnob, &tiltKnob, &harmonicsKnob, &mixKnob, &gainKnob })
        k->setBounds(area.removeFromLeft(knobW).reduced(4, 0));
}
