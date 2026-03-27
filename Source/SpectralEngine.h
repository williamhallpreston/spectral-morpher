#pragma once

#include <JuceHeader.h>
#include <vector>
#include <complex>
#include <array>

//==============================================================================
/**
 * SpectralEngine: Core FFT-based spectral processing unit.
 *
 * Algorithm overview:
 *  1. Overlap-add STFT (Short-Time Fourier Transform)
 *  2. Spectral manipulation in frequency domain
 *  3. Inverse FFT + overlap-add reconstruction
 *
 * Features:
 *  - Spectral Shift: pitch-independent frequency bin shifting
 *  - Spectral Freeze: locks the current spectral snapshot
 *  - Spectral Tilt: frequency-domain shelving EQ
 *  - Harmonic Enhancer: boosts even/odd harmonics selectively
 *  - Morph: crossfades between dry and processed spectra
 */
class SpectralEngine
{
public:
    static constexpr int FFT_ORDER  = 11;          // 2048-point FFT
    static constexpr int FFT_SIZE   = 1 << FFT_ORDER;
    static constexpr int HOP_SIZE   = FFT_SIZE / 4; // 75% overlap
    static constexpr int NUM_BINS   = FFT_SIZE / 2 + 1;

    SpectralEngine();
    ~SpectralEngine() = default;

    //==============================================================================
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // Process a single channel of audio; returns output sample
    void processBlock(const float* input, float* output, int numSamples, int channel);

    //==============================================================================
    // Parameters (set from audio thread — all atomic)
    void setMorphAmount(float v)    { morphAmount.store(v); }
    void setSpectralShift(float v)  { spectralShift.store(v); }
    void setFreeze(bool v)          { freeze.store(v); }
    void setSpectralTilt(float v)   { spectralTilt.store(v); }
    void setHarmonics(float v)      { harmonics.store(v); }
    void setMix(float v)            { dryWet.store(v); }
    void setOutputGain(float v)     { outputGain.store(v); }

    //==============================================================================
    // Spectrum data for UI (copies to caller's buffer)
    void getMagnitudes(std::vector<float>& dest) const;
    void getMorphedMagnitudes(std::vector<float>& dest) const;

private:
    //==============================================================================
    juce::dsp::FFT fft;

    // Per-channel state
    struct ChannelState
    {
        // Input/output circular buffers
        std::vector<float> inputBuffer;
        std::vector<float> outputBuffer;
        int inputWritePos  = 0;
        int outputReadPos  = 0;

        // FFT working buffers
        std::vector<float>               window;
        std::vector<std::complex<float>> spectrum;
        std::vector<std::complex<float>> frozenSpectrum;
        std::vector<float>               magnitudes;
        std::vector<float>               morphedMags;
        std::vector<float>               phases;
        std::vector<float>               lastPhases;
        std::vector<float>               synthPhases;

        int samplesUntilHop = 0;
        bool hasFrozen      = false;
    };

    static constexpr int MAX_CHANNELS = 2;
    std::array<ChannelState, MAX_CHANNELS> channels;

    // Shared magnitude snapshot for UI (channel 0)
    mutable juce::CriticalSection spectrumLock;
    std::vector<float> uiMagnitudes;
    std::vector<float> uiMorphedMags;

    double sampleRate = 44100.0;

    // Atomic parameters
    std::atomic<float> morphAmount  { 0.5f };
    std::atomic<float> spectralShift{ 0.0f };
    std::atomic<bool>  freeze       { false };
    std::atomic<float> spectralTilt { 0.0f };
    std::atomic<float> harmonics    { 0.0f };
    std::atomic<float> dryWet       { 1.0f };
    std::atomic<float> outputGain   { 1.0f };

    //==============================================================================
    void processFrame(ChannelState& ch, int channelIdx);
    void applyWindow(ChannelState& ch);
    void applySpectralShift(ChannelState& ch, float shiftAmount);
    void applySpectralTilt(ChannelState& ch, float tilt);
    void applyHarmonicEnhancer(ChannelState& ch, float amount);
    void applyMorph(ChannelState& ch, float amount);
    void updateUISpectrum(ChannelState& ch);

    // Hann window coefficients
    std::vector<float> hannWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralEngine)
};
