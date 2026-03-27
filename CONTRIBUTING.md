# Contributing to Spectral Morpher

Thanks for your interest in contributing! Here's everything you need to get started.

---

## Development setup

```bash
git clone https://github.com/YOUR_USERNAME/SpectralMorpher.git
cd SpectralMorpher
git submodule update --init --recursive   # fetch JUCE

# macOS Apple Silicon
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_OSX_ARCHITECTURES="arm64"
cmake --build build --parallel
```

The `Debug` build enables JUCE's leak detector and assertion checks — always develop with it.

---

## Project layout

```
Source/
  PluginProcessor.h/cpp   — AudioProcessor + APVTS parameter definitions
  PluginEditor.h/cpp      — UI components and LookAndFeel
  SpectralEngine.h/cpp    — All DSP: FFT, spectral processing, overlap-add
```

If you're adding a new effect, the right place is **SpectralEngine**. New parameters go in `PluginProcessor::createParameterLayout()`, and UI controls go in `PluginEditor`.

---

## Code style

- **C++17**, JUCE conventions throughout
- 4-space indentation, no tabs
- Class names: `PascalCase` — methods: `camelCase` — constants: `UPPER_SNAKE_CASE`
- Keep DSP code on the audio thread only — no allocations, no locks inside `processBlock()`
- All new parameters must be registered in the APVTS and persisted via `getStateInformation`

---

## Pull request checklist

- [ ] Builds cleanly on macOS arm64 with no warnings
- [ ] No allocations inside `processBlock()` or `processFrame()`
- [ ] New parameters have a display lambda in `createParameterLayout()`
- [ ] State save/load tested (save preset, reload project, verify values)
- [ ] PR description explains the DSP approach if adding a new algorithm

---

## Reporting bugs

Use the **Bug Report** issue template. Include your macOS version, Ableton version, and plugin format (VST3 or AU).

---

## DSP contribution ideas

- MIDI-controllable Morph via MPE
- Spectral convolution (load an IR and convolve in frequency domain)
- Per-band Freeze with a spectral gate threshold
- Spectral gate / noise floor reduction
- LFO modulation of Shift and Morph

---

## Licence

By contributing you agree that your code will be released under the [MIT License](LICENSE).
