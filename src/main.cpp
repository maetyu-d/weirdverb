#include "VerbSuite/WeirdConvolutionReverb.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void writeWavStereo16(const std::string& path, const std::vector<float>& left, const std::vector<float>& right, int sampleRate) {
    const std::size_t n = std::min(left.size(), right.size());
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open output WAV");
    }

    const int16_t channels = 2;
    const int16_t bitsPerSample = 16;
    const int32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
    const int16_t blockAlign = channels * (bitsPerSample / 8);
    const int32_t dataSize = static_cast<int32_t>(n * channels * (bitsPerSample / 8));
    const int32_t chunkSize = 36 + dataSize;

    out.write("RIFF", 4);
    out.write(reinterpret_cast<const char*>(&chunkSize), 4);
    out.write("WAVE", 4);

    out.write("fmt ", 4);
    const int32_t subchunk1Size = 16;
    const int16_t audioFormat = 1;
    out.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    out.write(reinterpret_cast<const char*>(&audioFormat), 2);
    out.write(reinterpret_cast<const char*>(&channels), 2);
    out.write(reinterpret_cast<const char*>(&sampleRate), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataSize), 4);

    for (std::size_t i = 0; i < n; ++i) {
        const float cl = std::clamp(left[i], -1.0f, 1.0f);
        const float cr = std::clamp(right[i], -1.0f, 1.0f);
        const int16_t li = static_cast<int16_t>(cl * 32767.0f);
        const int16_t ri = static_cast<int16_t>(cr * 32767.0f);
        out.write(reinterpret_cast<const char*>(&li), 2);
        out.write(reinterpret_cast<const char*>(&ri), 2);
    }
}

verbsuite::WeirdMode parseMode(const std::string& value) {
    using verbsuite::WeirdMode;
    if (value == "living") return WeirdMode::LivingSignal;
    if (value == "causal") return WeirdMode::UncannyCausality;
    if (value == "spectral") return WeirdMode::SpectralGhost;
    if (value == "memory") return WeirdMode::RainforestMemory;
    if (value == "imprint") return WeirdMode::ProcessImprint;
    if (value == "digital") return WeirdMode::DigitalFailure;
    if (value == "antispace") return WeirdMode::AntiSpace;
    if (value == "afterimage") return WeirdMode::Afterimage;
    if (value == "habit") return WeirdMode::HabitRoom;
    return WeirdMode::LivingSignal;
}

} // namespace

int main(int argc, char** argv) {
    constexpr int sampleRate = 48000;
    constexpr std::size_t totalSamples = sampleRate * 8;
    constexpr std::size_t blockSize = 64;

    std::string modeArg = "living";
    if (argc > 1) {
        modeArg = argv[1];
    }

    verbsuite::WeirdConvolutionReverb reverb(sampleRate, blockSize, parseMode(modeArg));

    verbsuite::WeirdControls controls;
    controls.memory = 0.85f;
    controls.coherence = 0.35f;
    controls.entropy = 0.72f;
    controls.resistance = 0.28f;
    controls.stability = 0.22f;
    controls.wet = 0.80f;
    controls.dry = 0.35f;
    reverb.setControls(controls);

    std::vector<float> left(totalSamples, 0.0f);
    std::vector<float> right(totalSamples, 0.0f);

    // Source signal: sparse impulses + tone burst + noisy tail for IR-indexing behavior.
    for (std::size_t i = 0; i < totalSamples; ++i) {
        float x = 0.0f;
        if (i % (sampleRate / 2) == 0) {
            x += 1.0f;
        }
        if (i > sampleRate && i < sampleRate * 3) {
            x += 0.35f * std::sin(2.0f * 3.14159265358979323846f * 220.0f * static_cast<float>(i) / static_cast<float>(sampleRate));
        }
        if (i > sampleRate * 4) {
            const float saw = std::fmod(static_cast<float>(i) * 0.0037f, 1.0f) * 2.0f - 1.0f;
            x += 0.2f * saw;
        }

        left[i] = x;
        right[i] = (i % 2 == 0) ? x * 0.8f : x * 0.2f;
    }

    for (std::size_t base = 0; base < totalSamples; base += blockSize) {
        const std::size_t n = std::min(blockSize, totalSamples - base);
        reverb.processBlock(left.data() + base, right.data() + base, n);
    }

    const std::string outputPath = "weird_" + modeArg + ".wav";
    writeWavStereo16(outputPath, left, right, sampleRate);

    std::cout << "Rendered mode=" << modeArg << " to " << outputPath << '\n';
    return 0;
}
