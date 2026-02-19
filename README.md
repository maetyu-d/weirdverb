# weirdVERB (JUCE AU/VST3)

Single JUCE plugin with:
- `Mode` switch (9 weird convolution personalities)
- `Stability` macro (realistic -> unstable -> autonomous)
- Tempo-sync options for elastic IR breathing
- Sidechain `Stability CV` audio-rate modulation mode

The DSP treats the IR as a dynamic, process-like signal and adds strong per-mode variation.

## Build

```bash
cmake -S . -B build -DJUCE_DIR=/Users/md/JUCE
cmake --build build -j
```

This builds:
- `verb_suite_demo` (CLI WAV renderer)
- `weirdVERB` plugin targets (`AU`, `VST3`) if JUCE is found

## New controls

- `Breath Sync`: `Free`, `1 bar`, `1/2`, `1/4`, `1/8`, `1/16`, `1/8T`, `1/4D`
- `Breath Rate`: LFO rate in Hz (used when `Breath Sync = Free`)
- `Breath Depth`: modulation depth of elastic IR time
- `Stability CV`: `Off` or `Audio-rate`
- `CV Amount`: bipolar depth (`-1..1`) applied to sidechain signal for Stability modulation
- `CV Smoothing`: `Raw` (audio-rate bipolar) or `Envelope` (unipolar follower)
- `CV Filter Time`: `Fast` / `Medium` / `Slow` envelope response (used by `CV Smoothing = Envelope`)

When `Stability CV` is `Audio-rate`, the sidechain input modulates Stability per-sample.
The editor shows a live CV meter on the right edge.

## Factory presets (10)

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

## Modes

1. Living Signal
2. Uncanny Causality
3. Spectral Ghost
4. Rainforest Memory
5. Process Imprint
6. Digital Failure
7. Anti-Space
8. Afterimage
9. Habit Room

## What got weirder

- Expanded IR bank includes procedural non-room IRs (morse-like, body-pulse, process/noise signatures)
- Time-variant tri-band convolution with per-band IR assignment/swap
- Non-causal pre-ringing and reversible early reflections
- Spectral/phase-like IR destabilization and random polarity reseeding
- Convolution feedback with resonant/self-excited behavior
- Stochastic frame dropping and blockwise degradation in digital mode
- Autonomous drone emergence at low Stability, especially in Habit Room

## Demo renders

```bash
./build/verb_suite_demo living
./build/verb_suite_demo causal
./build/verb_suite_demo spectral
./build/verb_suite_demo memory
./build/verb_suite_demo imprint
./build/verb_suite_demo digital
./build/verb_suite_demo antispace
./build/verb_suite_demo afterimage
./build/verb_suite_demo habit
```
