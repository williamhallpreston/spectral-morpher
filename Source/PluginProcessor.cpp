#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
SpectralMorpherAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "morphAmount", "Morph Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v * 100.0f, 1) + "%"; }));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spectralShift", "Spectral Shift",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 2) + " st"; }));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "freeze", "Spectral Freeze", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "tilt", "Spectral Tilt",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) {
            if (v > 0.01f)  return "Bright +" + juce::String(v * 100.0f, 0) + "%";
            if (v < -0.01f) return "Warm "    + juce::String(-v * 100.0f, 0) + "%";
            return juce::String("Flat");
        }));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "harmonics", "Harmonic Enhancer",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v * 100.0f, 1) + "%"; }));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Dry/Wet Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v * 100.0f, 1) + "%"; }));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "outputGain", "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " dB"; }));

    return { params.begin(), params.end() };
}

//==============================================================================
SpectralMorpherAudioProcessor::SpectralMorpherAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SpectralMorpherState", createParameterLayout()),
      spectralEngine(std::make_unique<SpectralEngine>())
{
    // Cache parameter pointers
    morphAmountParam   = apvts.getRawParameterValue("morphAmount");
    spectralShiftParam = apvts.getRawParameterValue("spectralShift");
    freezeParam        = apvts.getRawParameterValue("freeze");
    tiltParam          = apvts.getRawParameterValue("tilt");
    harmonicsParam     = apvts.getRawParameterValue("harmonics");
    mixParam           = apvts.getRawParameterValue("mix");
    outputGainParam    = apvts.getRawParameterValue("outputGain");

    // Register as listener for all params
    for (const auto& id : { "morphAmount", "spectralShift", "freeze",
                             "tilt", "harmonics", "mix", "outputGain" })
        apvts.addParameterListener(id, this);
}

SpectralMorpherAudioProcessor::~SpectralMorpherAudioProcessor()
{
    for (const auto& id : { "morphAmount", "spectralShift", "freeze",
                             "tilt", "harmonics", "mix", "outputGain" })
        apvts.removeParameterListener(id, this);
}

//==============================================================================
void SpectralMorpherAudioProcessor::parameterChanged(const juce::String& paramID, float newValue)
{
    if (spectralEngine == nullptr) return;

    if      (paramID == "morphAmount")   spectralEngine->setMorphAmount(newValue);
    else if (paramID == "spectralShift") spectralEngine->setSpectralShift(newValue);
    else if (paramID == "freeze")        spectralEngine->setFreeze(newValue > 0.5f);
    else if (paramID == "tilt")          spectralEngine->setSpectralTilt(newValue);
    else if (paramID == "harmonics")     spectralEngine->setHarmonics(newValue);
    else if (paramID == "mix")           spectralEngine->setMix(newValue);
    else if (paramID == "outputGain")    spectralEngine->setOutputGain(juce::Decibels::decibelsToGain(newValue));
}

//==============================================================================
void SpectralMorpherAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    spectralEngine->prepare(sampleRate, samplesPerBlock);

    // Push current parameter values to engine
    spectralEngine->setMorphAmount   (*morphAmountParam);
    spectralEngine->setSpectralShift (*spectralShiftParam);
    spectralEngine->setFreeze        (*freezeParam > 0.5f);
    spectralEngine->setSpectralTilt  (*tiltParam);
    spectralEngine->setHarmonics     (*harmonicsParam);
    spectralEngine->setMix           (*mixParam);
    spectralEngine->setOutputGain    (juce::Decibels::decibelsToGain(outputGainParam->load()));
}

void SpectralMorpherAudioProcessor::releaseResources()
{
    spectralEngine->reset();
}

bool SpectralMorpherAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

//==============================================================================
void SpectralMorpherAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int ch = 0; ch < std::min(numChannels, 2); ++ch)
    {
        spectralEngine->processBlock(buffer.getReadPointer(ch),
                                     buffer.getWritePointer(ch),
                                     numSamples, ch);
    }
}

//==============================================================================
const std::vector<float>& SpectralMorpherAudioProcessor::getMagnitudeSpectrum() const
{
    // Delegate via a thread-safe copy; editor stores its own buffer
    static std::vector<float> buf;
    spectralEngine->getMagnitudes(buf);
    return buf;
}

const std::vector<float>& SpectralMorpherAudioProcessor::getMorphedSpectrum() const
{
    static std::vector<float> buf;
    spectralEngine->getMorphedMagnitudes(buf);
    return buf;
}

//==============================================================================
void SpectralMorpherAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SpectralMorpherAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
juce::AudioProcessorEditor* SpectralMorpherAudioProcessor::createEditor()
{
    return new SpectralMorpherAudioProcessorEditor(*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectralMorpherAudioProcessor();
}
