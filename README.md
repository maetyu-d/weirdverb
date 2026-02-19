# weirdVERB

Experimental and very weird lo-fi convolution reverb with "living" IR behavior, sidechain CV modulation, and highly unstable space modes. Built in JUCE (AU/VST3).

## Features

weirdVEB features "living" convolution, whereby the impulse response continuously morphs across a synthetic IR bank driven by the input’s envelope and brightness, combined with time-variant processing such as elastic (free or tempo-synced) IR “breathing,” micro-Doppler resampling, grain-like shuffling, and IR-only bit reduction and waveshaping. The input can be split into low, mid, and high bands for evolving, role-swapping spatial convolution, while causality shifts, reversed early sections, and phase/polarity perturbations introduce ghosted transients and spectral destabilisation. Convolution feedback and resonant self-excitation enable semi-autonomous drone behavior in unstable states, supported by a lo-fi memory engine with bounded taps, adaptive stride, sample-and-hold wet updates, and quantised texture for character, as well as CPU-efficieny. Sidechain audio can modulate Stability at audio rate with optional envelope smoothing and stepped hold, and freeze modes (latch or momentary) capture and decay the current tail, with optional 2× oversampling for high-quality offline export.

## Build

```bash
cmake -S . -B build -DJUCE_DIR=/Users/md/JUCE
cmake --build build -j
```

Outputs:
- AU: `build/weirdVERB_artefacts/AU/weirdVERB.component`
- VST3: `build/weirdVERB_artefacts/VST3/weirdVERB.vst3`

## Logic Pro Install

1. Build the project.
2. Copy AU component into Logic’s user components folder:

```bash
ditto "build/weirdVERB_artefacts/AU/weirdVERB.component" "$HOME/Library/Audio/Plug-Ins/Components/weirdVERB.component"
killall -9 AudioComponentRegistrar 2>/dev/null || true
```

3. Restart Logic Pro.
4. Open **Plugin Manager** and rescan if needed.
5. Insert `weirdVERB` on an audio aux/track.

## Quick Start (Logic)

1. Pick a preset from `Preset`.
2. Set `Dry` around `0.3-0.6` and `Wet` around `0.5-1.0`.
3. For rhythmic motion, set `Breath Sync` to `1/4` or `1/8`.
4. For modulation by sidechain signal:
- Route sidechain input in Logic
- Set `Stability CV` to `Audio-rate`
- Increase `CV Amount`
- Choose `CV Smoothing` and `CV Filter Time`

## Factory Presets

1. Null Tape
2. Precrime Bloom
3. Shattered Prism
4. Swamp Antenna
5. Packet Graveyard
6. Broken Choir RAM
7. Inverted Corridor
8. Ash Afterimage
9. Self-Taught Room
10. Lofi Leviathan

## Notes

- `HQ Export` is intended for offline rendering/bounce, not live low-latency use.
- If Logic appears to cache old plugin binaries, clear cache by killing `AudioComponentRegistrar` and rescanning.
