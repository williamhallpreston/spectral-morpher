<div align="center">

# 🎛️ Spectral Morpher

**A real-time spectral morphing VST3/AU plugin built with JUCE**

[![CI](https://github.com/YOUR_USERNAME/SpectralMorpher/actions/workflows/build.yml/badge.svg)](https://github.com/YOUR_USERNAME/SpectralMorpher/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows%20%7C%20Linux-lightgrey)](#building)
[![Format](https://img.shields.io/badge/format-VST3%20%7C%20AU-orange)](#installation)
[![JUCE](https://img.shields.io/badge/built%20with-JUCE%207%2B-green)](https://juce.com)

*Freeze, shift, tilt, and morph audio in the frequency domain — in real time.*

</div>

---

## Overview

Spectral Morpher processes audio using a **phase vocoder / STFT pipeline**, giving you direct access to the frequency-domain representation of sound. Instead of working with samples, it works with *spectral bins* — enabling effects that are impossible with traditional time-domain processing.

```
Input Audio
    │
    ▼
Overlap-Add STFT (2048-pt FFT · 75% overlap · Hann window)
    │
    ├── Spectral Freeze   — lock a frame as a sustained drone
    ├── Spectral Shift    — translate bins ±24 semitones
    ├── Spectral Tilt     — log-frequency slope EQ
    ├── Harmonic Enhancer — fold energy into 2× bins
    └── Morph Accumulator — peak-hold spectral smear with decay
    │
    ▼
Phase Vocoder Reconstruction → Overlap-Add IFFT → Output
```

---

## Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| **Morph** | 0–100% | Spectral peak-hold accumulator. At 0% = clean. At 100% = infinite spectral sustain / bloom |
| **Shift** | ±24 st | Moves all spectral bins up or down in frequency, independent of pitch |
| **Freeze** | on/off | Locks the current FFT snapshot — converts any transient into a held drone |
| **Tilt** | ±100% | Spectral slope EQ applied in frequency domain. Positive = bright, negative = warm |
| **Harmonics** | 0–100% | Folds energy from bin *k* into bin *2k*, adding soft even-harmonic saturation |
| **Mix** | 0–100% | Dry/wet blend |
| **Gain** | −24 to +12 dB | Output level |

### Creative patches

| Patch | Settings |
|-------|----------|
| **Spectral reverb tail** | Morph 70%, Mix 60%, Tilt −20% |
| **Drone from any hit** | Freeze ON, Harmonics 40%, Mix 100% |
| **Alien pitch shift** | Shift +7st, Morph 30%, Harmonics 20% |
| **Warm spectral smear** | Morph 50%, Tilt −40%, Shift −3st |
| **Air and sheen** | Harmonics 80%, Tilt +30%, Morph 10% |

---

## Building

### Prerequisites

| Tool | Version |
|------|---------|
| CMake | 3.22+ |
| C++ compiler | C++17 (Clang 12+, MSVC 2019+, GCC 11+) |
| JUCE | 7.x or 8.x |

### 1 — Clone with submodule

```bash
git clone https://github.com/YOUR_USERNAME/SpectralMorpher.git
cd SpectralMorpher
git submodule update --init --recursive
```

> **No JUCE submodule?** Pass `-DJUCE_DIR=/path/to/JUCE` to CMake instead.

### 2 — Configure

```bash
mkdir build && cd build

# macOS — Apple Silicon (recommended)
cmake .. -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="12.0"

# macOS — Universal Binary (arm64 + x86_64)
cmake .. -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="12.0"

# Windows
cmake .. -G "Visual Studio 17 2022" -A x64

# Linux
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### 3 — Build

```bash
cmake --build . --config Release --parallel
```

### 4 — Output locations

| Platform | Path |
|----------|------|
| macOS VST3 | `build/SpectralMorpher_artefacts/Release/VST3/SpectralMorpher.vst3` |
| macOS AU | `build/SpectralMorpher_artefacts/Release/AU/SpectralMorpher.component` |
| Windows VST3 | `build\SpectralMorpher_artefacts\Release\VST3\SpectralMorpher.vst3` |
| Linux VST3 | `build/SpectralMorpher_artefacts/Release/VST3/SpectralMorpher.vst3` |

---

## Installation

### macOS

```bash
# VST3
cp -r build/**/*.vst3 ~/Library/Audio/Plug-Ins/VST3/

# AU
cp -r build/**/*.component ~/Library/Audio/Plug-Ins/Components/
```

> **Gatekeeper:** On first load macOS may block the plugin. Go to
> *System Settings → Privacy & Security* and click **Allow**.
> For Ableton Live on Apple Silicon: right-click the `.vst3` → Get Info → uncheck **Open with Rosetta**.

### Windows

Copy `SpectralMorpher.vst3` to `C:\Program Files\Common Files\VST3\`, then rescan in your DAW.

---

## DAW Setup

<details>
<summary><strong>Ableton Live</strong></summary>

1. *Preferences → Plug-ins → VST3 Plug-in Folder* — set to `~/Library/Audio/Plug-Ins/VST3/`
2. Enable **AU Plug-ins** on macOS (recommended over VST3 for stability in Live)
3. Click **Rescan**
4. Browse to *Plug-ins → SpectralMorpher* and drag onto an Audio track

</details>

<details>
<summary><strong>Logic Pro</strong></summary>

Build the AU format (macOS only). Copy the `.component` to `~/Library/Audio/Plug-Ins/Components/`.
Logic auto-scans on launch — plugin appears under *Audio FX → YOUR_STUDIO → Spectral Morpher*.

</details>

<details>
<summary><strong>Reaper / Bitwig / Cubase</strong></summary>

Point your DAW's VST3 scan path to `~/Library/Audio/Plug-Ins/VST3/` (macOS) or
`C:\Program Files\Common Files\VST3\` (Windows) and rescan.

</details>

---

## Project structure

```
SpectralMorpher/
├── .github/
│   ├── workflows/build.yml          ← CI: macOS arm64, Windows, Linux
│   └── ISSUE_TEMPLATE/              ← Bug report & feature request forms
├── Source/
│   ├── PluginProcessor.h/cpp        ← AudioProcessor + APVTS parameters
│   ├── PluginEditor.h/cpp           ← UI: LookAndFeel + spectrum analyser
│   └── SpectralEngine.h/cpp         ← Core DSP: FFT, spectral ops, phase vocoder
├── CMakeLists.txt
├── CHANGELOG.md
├── CONTRIBUTING.md
└── LICENSE                          ← MIT
```

---

## Contributing

Contributions are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md) for setup instructions, code style, and the PR checklist.

---

## Technical notes

- **FFT size:** 2048 samples — balances frequency resolution and latency
- **Hop size:** 512 samples (75% overlap) — standard for phase vocoder quality
- **Window:** Hann — optimal sidelobe suppression for spectral processing
- **Phase vocoder:** Full unwrapped-phase accumulation for artefact-free reconstruction
- **Thread safety:** All parameters use `std::atomic`; UI spectrum copy protected by `CriticalSection`
- **No allocations** in `processBlock()` — all buffers pre-allocated in `prepareToPlay()`

---

## License

[MIT](LICENSE) © 2026 SpectralMorpher Contributors

JUCE is used under the [JUCE License](https://juce.com/juce-privacy-policy). A commercial JUCE license is required if you distribute this plugin commercially.
