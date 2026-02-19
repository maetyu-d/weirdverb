# weirdVERB (JUCE AU/VST3)

Experimental lo-fi convolution reverb with living IR behavior, sidechain CV modulation, and highly unstable space modes.

## Features

- 9 weird convolution modes (`Mode`)
- `Stability` macro (realistic -> unstable -> autonomous)
- Tempo-sync elastic IR breathing
- Sidechain audio-rate `Stability CV` with smoothing + filter time
- Lo-fi memory engine for lower CPU and deliberate digital texture
- Freeze (`Latch` / `Momentary`)
- Dry / Wet / Output controls
- Randomize + Lock Core
- 10 factory presets
- `HQ Export` (2x oversampling in offline renders)

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
