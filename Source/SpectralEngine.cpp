#include "SpectralEngine.h"
#include <cmath>
#include <algorithm>
#include <numeric>

//==============================================================================
SpectralEngine::SpectralEngine()
    : fft(FFT_ORDER)
{
    // Build Hann window
    hannWindow.resize(FFT_SIZE);
    for (int i = 0; i < FFT_SIZE; ++i)
        hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (FFT_SIZE - 1)));

    // Init channel states
    for (auto& ch : channels)
    {
        ch.inputBuffer    .assign(FFT_SIZE * 2, 0.0f);
        ch.outputBuffer   .assign(FFT_SIZE * 2, 0.0f);
        ch.window         .assign(FFT_SIZE,     0.0f);
        ch.spectrum       .assign(FFT_SIZE,     {});
        ch.frozenSpectrum .assign(NUM_BINS,     {});
        ch.magnitudes     .assign(NUM_BINS,     0.0f);
        ch.morphedMags    .assign(NUM_BINS,     0.0f);
        ch.phases         .assign(NUM_BINS,     0.0f);
        ch.lastPhases     .assign(NUM_BINS,     0.0f);
        ch.synthPhases    .assign(NUM_BINS,     0.0f);
        ch.samplesUntilHop = HOP_SIZE;
    }

    uiMagnitudes  .assign(NUM_BINS, 0.0f);
    uiMorphedMags .assign(NUM_BINS, 0.0f);
}

//==============================================================================
void SpectralEngine::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    reset();
}

void SpectralEngine::reset()
{
    for (auto& ch : channels)
    {
        std::fill(ch.inputBuffer.begin(),  ch.inputBuffer.end(),  0.0f);
        std::fill(ch.outputBuffer.begin(), ch.outputBuffer.end(), 0.0f);
        std::fill(ch.lastPhases.begin(),   ch.lastPhases.end(),   0.0f);
        std::fill(ch.synthPhases.begin(),  ch.synthPhases.end(),  0.0f);
        ch.inputWritePos  = 0;
        ch.outputReadPos  = 0;
        ch.samplesUntilHop = HOP_SIZE;
        ch.hasFrozen       = false;
    }
}

//==============================================================================
void SpectralEngine::processBlock(const float* input, float* output, int numSamples, int channelIdx)
{
    if (channelIdx >= MAX_CHANNELS) return;
    auto& ch = channels[channelIdx];

    const float gain   = outputGain.load();
    const float wetMix = dryWet.load();
    const float dryMix = 1.0f - wetMix;

    for (int i = 0; i < numSamples; ++i)
    {
        const float in = input[i];

        // Write into circular input buffer
        ch.inputBuffer[ch.inputWritePos] = in;
        ch.inputWritePos = (ch.inputWritePos + 1) % (int)ch.inputBuffer.size();

        // Every HOP_SIZE samples, run spectral frame
        --ch.samplesUntilHop;
        if (ch.samplesUntilHop <= 0)
        {
            ch.samplesUntilHop = HOP_SIZE;
            processFrame(ch, channelIdx);
        }

        // Read from output overlap-add buffer
        float wet = ch.outputBuffer[ch.outputReadPos];
        ch.outputBuffer[ch.outputReadPos] = 0.0f;
        ch.outputReadPos = (ch.outputReadPos + 1) % (int)ch.outputBuffer.size();

        output[i] = (dryMix * in + wetMix * wet) * gain;
    }
}

//==============================================================================
void SpectralEngine::processFrame(ChannelState& ch, int channelIdx)
{
    // --- 1. Copy windowed input into FFT buffer ---
    const int bufSize = (int)ch.inputBuffer.size();
    int readPos = (ch.inputWritePos - FFT_SIZE + bufSize) % bufSize;

    for (int i = 0; i < FFT_SIZE; ++i)
    {
        ch.window[i] = ch.inputBuffer[readPos] * hannWindow[i];
        readPos = (readPos + 1) % bufSize;
    }

    // --- 2. Forward FFT ---
    // JUCE FFT expects interleaved complex data (size = FFT_SIZE * 2)
    std::vector<float> fftBuffer(FFT_SIZE * 2, 0.0f);
    for (int i = 0; i < FFT_SIZE; ++i)
        fftBuffer[i * 2] = ch.window[i];

    fft.performFrequencyOnlyForwardTransform(fftBuffer.data()); // magnitude only for visualizer
    // Full complex for processing:
    std::vector<float> complexBuf(FFT_SIZE * 2, 0.0f);
    for (int i = 0; i < FFT_SIZE; ++i)
        complexBuf[i * 2] = ch.window[i];
    fft.performRealOnlyForwardTransform(complexBuf.data());

    // Unpack to complex array
    for (int k = 0; k < NUM_BINS; ++k)
        ch.spectrum[k] = { complexBuf[k * 2], complexBuf[k * 2 + 1] };

    // Extract magnitude and phase
    for (int k = 0; k < NUM_BINS; ++k)
    {
        ch.magnitudes[k] = std::abs(ch.spectrum[k]);
        ch.phases[k]     = std::arg(ch.spectrum[k]);
    }

    // --- 3. Spectral Freeze ---
    if (freeze.load())
    {
        if (!ch.hasFrozen)
        {
            ch.frozenSpectrum = ch.spectrum;
            ch.hasFrozen = true;
        }
        // Replace spectrum with frozen snapshot
        ch.spectrum = ch.frozenSpectrum;
        for (int k = 0; k < NUM_BINS; ++k)
            ch.magnitudes[k] = std::abs(ch.spectrum[k]);
    }
    else
    {
        ch.hasFrozen = false;
    }

    // --- 4. Spectral Shift ---
    const float shift = spectralShift.load(); // in semitones, -24..+24
    if (std::abs(shift) > 0.01f)
        applySpectralShift(ch, shift);

    // --- 5. Spectral Tilt ---
    const float tilt = spectralTilt.load();
    if (std::abs(tilt) > 0.01f)
        applySpectralTilt(ch, tilt);

    // --- 6. Harmonic Enhancer ---
    const float harms = harmonics.load();
    if (harms > 0.001f)
        applyHarmonicEnhancer(ch, harms);

    // --- 7. Morph (cross-fade magnitudes with original) ---
    const float morph = morphAmount.load();
    applyMorph(ch, morph);

    // --- 8. Update UI spectrum (channel 0 only) ---
    if (channelIdx == 0)
        updateUISpectrum(ch);

    // --- 9. Inverse FFT + overlap-add ---
    // Reconstruct complex buffer from magnitude + unwrapped phase
    std::vector<float> ifftBuf(FFT_SIZE * 2, 0.0f);
    for (int k = 0; k < NUM_BINS; ++k)
    {
        // Phase vocoder: accumulate synthesis phases
        const float expectedPhase   = ch.lastPhases[k] + 2.0f * juce::MathConstants<float>::pi
                                      * (float)k * HOP_SIZE / FFT_SIZE;
        const float phaseDiff       = ch.phases[k] - expectedPhase;
        const float wrappedDiff     = phaseDiff - juce::MathConstants<float>::twoPi
                                      * std::round(phaseDiff / juce::MathConstants<float>::twoPi);
        ch.synthPhases[k]          += 2.0f * juce::MathConstants<float>::pi
                                      * (float)k / FFT_SIZE + wrappedDiff / HOP_SIZE;
        ch.lastPhases[k]            = ch.phases[k];

        const float mag = ch.morphedMags[k];
        ifftBuf[k * 2]     = mag * std::cos(ch.synthPhases[k]);
        ifftBuf[k * 2 + 1] = mag * std::sin(ch.synthPhases[k]);
    }

    // Mirror for real IFFT
    for (int k = 1; k < NUM_BINS - 1; ++k)
    {
        ifftBuf[(FFT_SIZE - k) * 2]     =  ifftBuf[k * 2];
        ifftBuf[(FFT_SIZE - k) * 2 + 1] = -ifftBuf[k * 2 + 1];
    }

    fft.performRealOnlyInverseTransform(ifftBuf.data());

    // Overlap-add with Hann window (normalised by overlap factor)
    const float normFactor = 0.5f / (FFT_SIZE / HOP_SIZE) * 2.0f;
    const int outBufSize = (int)ch.outputBuffer.size();

    for (int i = 0; i < FFT_SIZE; ++i)
    {
        int writePos = (ch.outputReadPos + i) % outBufSize;
        ch.outputBuffer[writePos] += ifftBuf[i * 2] * hannWindow[i] * normFactor;
    }
}

//==============================================================================
void SpectralEngine::applySpectralShift(ChannelState& ch, float semitones)
{
    const float ratio    = std::pow(2.0f, semitones / 12.0f);

    std::vector<std::complex<float>> shifted(NUM_BINS, {0.0f, 0.0f});

    for (int k = 0; k < NUM_BINS; ++k)
    {
        int newK = (int)(k * ratio);
        if (newK >= 0 && newK < NUM_BINS)
            shifted[newK] += ch.spectrum[k];
    }

    ch.spectrum = shifted;
    for (int k = 0; k < NUM_BINS; ++k)
        ch.magnitudes[k] = std::abs(ch.spectrum[k]);
}

void SpectralEngine::applySpectralTilt(ChannelState& ch, float tilt)
{
    // tilt: -1..+1, positive = high boost, negative = low boost
    for (int k = 0; k < NUM_BINS; ++k)
    {
        const float freq = (float)k / NUM_BINS; // normalised 0..1
        const float gain = std::pow(10.0f, tilt * (freq - 0.5f) * 12.0f / 20.0f); // ±6dB/octave
        ch.spectrum[k]   *= gain;
        ch.magnitudes[k]  = std::abs(ch.spectrum[k]);
    }
}

void SpectralEngine::applyHarmonicEnhancer(ChannelState& ch, float amount)
{
    // Soft even-harmonic saturation: fold spectrum onto double frequencies
    const float blend = amount * 0.3f;
    for (int k = 1; k < NUM_BINS / 2; ++k)
    {
        const int harmK = k * 2;
        if (harmK < NUM_BINS)
        {
            const float mag   = std::abs(ch.spectrum[k]);
            const float phase = std::arg(ch.spectrum[k]) * 2.0f;
            ch.spectrum[harmK] += std::complex<float>(mag * blend * std::cos(phase),
                                                      mag * blend * std::sin(phase));
        }
    }
    for (int k = 0; k < NUM_BINS; ++k)
        ch.magnitudes[k] = std::abs(ch.spectrum[k]);
}

void SpectralEngine::applyMorph(ChannelState& ch, float amount)
{
    // Morph: amount=0 → dry magnitudes, amount=1 → max-spectral version
    // Creates an "infinite reverb / spectral smear" effect at high values
    static thread_local std::vector<float> accum(NUM_BINS, 0.0f);
    if ((int)accum.size() != NUM_BINS) accum.assign(NUM_BINS, 0.0f);

    const float decay = 1.0f - amount * 0.3f;
    for (int k = 0; k < NUM_BINS; ++k)
    {
        accum[k]         = std::max(ch.magnitudes[k], accum[k] * decay);
        ch.morphedMags[k] = ch.magnitudes[k] * (1.0f - amount) + accum[k] * amount;
    }
}

void SpectralEngine::updateUISpectrum(ChannelState& ch)
{
    juce::ScopedLock sl(spectrumLock);
    // Smooth towards new magnitudes for display
    for (int k = 0; k < NUM_BINS; ++k)
    {
        uiMagnitudes[k]  += 0.3f * (ch.magnitudes[k]  - uiMagnitudes[k]);
        uiMorphedMags[k] += 0.3f * (ch.morphedMags[k] - uiMorphedMags[k]);
    }
}

//==============================================================================
void SpectralEngine::getMagnitudes(std::vector<float>& dest) const
{
    juce::ScopedLock sl(spectrumLock);
    dest = uiMagnitudes;
}

void SpectralEngine::getMorphedMagnitudes(std::vector<float>& dest) const
{
    juce::ScopedLock sl(spectrumLock);
    dest = uiMorphedMags;
}
