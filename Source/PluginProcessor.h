#pragma once

#include <JuceHeader.h>
#include "SpectralEngine.h"

//==============================================================================
/**
 * SpectralMorpher VST3/AU Plugin
 *
 * A unique spectral morphing effect that:
 *  - Performs real-time FFT analysis on the input signal
 *  - Applies spectral shifting, freezing, and cross-synthesis
 *  - Provides a "Morph" parameter blending dry/processed spectral content
 *  - Features a spectral tilt EQ and harmonic enhancer
 */
class SpectralMorpherAudioProcessor : public juce::AudioProcessor,
                                      public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    SpectralMorpherAudioProcessor();
    ~SpectralMorpherAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    void parameterChanged(const juce::String& paramID, float newValue) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Expose spectral data for visualizer
    const std::vector<float>& getMagnitudeSpectrum() const;
    const std::vector<float>& getMorphedSpectrum() const;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::unique_ptr<SpectralEngine> spectralEngine;

    // Parameter pointers (cached for performance)
    std::atomic<float>* morphAmountParam   = nullptr;
    std::atomic<float>* spectralShiftParam = nullptr;
    std::atomic<float>* freezeParam        = nullptr;
    std::atomic<float>* tiltParam          = nullptr;
    std::atomic<float>* harmonicsParam     = nullptr;
    std::atomic<float>* mixParam           = nullptr;
    std::atomic<float>* outputGainParam    = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralMorpherAudioProcessor)
};
