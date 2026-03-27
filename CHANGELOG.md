# Changelog

All notable changes to Spectral Morpher are documented here.  
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).  
Versioning follows [Semantic Versioning](https://semver.org/).

---

## [Unreleased]

### Planned
- MIDI CC mapping for all parameters
- Spectral convolution mode (load impulse response)
- Per-band Freeze with threshold gate
- Dark / light UI theme toggle

---

## [1.0.0] — 2026-03-27

### Added
- Real-time STFT spectral engine (2048-pt FFT, 75% overlap, Hann window)
- **Morph** — spectral peak-hold accumulator with variable decay
- **Shift** — frequency-bin shifting ±24 semitones
- **Freeze** — locks current FFT frame as a sustained drone
- **Tilt** — log-frequency spectral slope EQ
- **Harmonics** — even-harmonic folder (2× bin injection)
- **Mix** — dry/wet blend
- **Gain** — output level ±24 dB
- Dual-layer live spectrum analyser (input + morphed)
- Custom dark LookAndFeel with cyan/purple accent palette
- VST3 and AU formats
- CMake build system with JUCE submodule support
- GitHub Actions CI: macOS arm64, macOS universal, Windows x64, Linux x64
- Full APVTS preset save/restore
- Phase vocoder reconstruction for artefact-free time-domain output

[Unreleased]: https://github.com/YOUR_USERNAME/SpectralMorpher/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/YOUR_USERNAME/SpectralMorpher/releases/tag/v1.0.0
