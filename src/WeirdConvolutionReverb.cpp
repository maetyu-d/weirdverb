#include "VerbSuite/WeirdConvolutionReverb.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace verbsuite {
namespace {
constexpr float kPi = 3.14159265358979323846f;

float softClip(float x) {
    return std::tanh(x);
}

float clamp01(float x) {
    return std::clamp(x, 0.0f, 1.0f);
}

} // namespace

WeirdConvolutionReverb::WeirdConvolutionReverb(double sampleRate, std::size_t blockSize, WeirdMode mode)
    : sampleRate_(sampleRate),
      blockSize_(blockSize),
      mode_(mode),
      rng_(0xC0FFEEu) {
    buildIRBank();
    reset();
}

std::string WeirdConvolutionReverb::modeName(WeirdMode mode) {
    switch (mode) {
    case WeirdMode::LivingSignal: return "Living Signal";
    case WeirdMode::UncannyCausality: return "Uncanny Causality";
    case WeirdMode::SpectralGhost: return "Spectral Ghost";
    case WeirdMode::RainforestMemory: return "Rainforest Memory";
    case WeirdMode::ProcessImprint: return "Process Imprint";
    case WeirdMode::DigitalFailure: return "Digital Failure";
    case WeirdMode::AntiSpace: return "Anti-Space";
    case WeirdMode::Afterimage: return "Afterimage";
    case WeirdMode::HabitRoom: return "Habit Room";
    }
    return "Unknown";
}

WeirdMode WeirdConvolutionReverb::modeFromIndex(int index) {
    if (index < 0) {
        index = 0;
    }
    if (index >= modeCount()) {
        index = modeCount() - 1;
    }
    return static_cast<WeirdMode>(index);
}

void WeirdConvolutionReverb::setMode(WeirdMode newMode) {
    mode_ = newMode;
}

void WeirdConvolutionReverb::setControls(const WeirdControls& newControls) {
    controls_ = newControls;
}

void WeirdConvolutionReverb::reset() {
    const std::size_t maxIR = 6144;
    inputHistory_.assign(maxIR * 2, 0.0f);
    feedbackHistory_.assign(maxIR * 2, 0.0f);
    historyWrite_ = 0;
    frameCounter_ = 0;
    featureEnvelope_ = 0.0f;
    featureBrightness_ = 0.0f;
    previousMono_ = 0.0f;
    learnedBias_ = 0.0f;
    autonomousDronePhase_ = 0.0f;
    lpState_ = 0.0f;
    hpState_ = 0.0f;
    dynamicStability_ = controls_.stability;
    lofiHoldCounter_ = 0;
    lofiHoldPeriod_ = 1;
    lofiInputHeld_ = 0.0f;
    lofiWetHeld_ = 0.0f;
    lofiWowPhase_ = 0.0f;

    activeIRLow_ = irBank_.front();
    activeIRMid_ = irBank_[1];
    activeIRHigh_ = irBank_[2];
    zeroIndex_ = 0;
}

std::string WeirdConvolutionReverb::modeName() const {
    return modeName(mode_);
}

void WeirdConvolutionReverb::buildIRBank() {
    irBank_.clear();
    // Core bank.
    irBank_.push_back(generateIR(640, 0.30f, 0.30f, 0.05f));
    irBank_.push_back(generateIR(960, 0.80f, 0.60f, 0.25f));
    irBank_.push_back(generateIR(1408, 1.60f, 0.80f, 0.55f));
    irBank_.push_back(generateIR(2048, 2.80f, 0.95f, 0.88f));
    irBank_.push_back(generateMorseIR(1536, 0.40f));
    irBank_.push_back(generateMorseIR(2048, 0.78f));
    irBank_.push_back(generateBodyIR(1664, 1.6f));
    irBank_.push_back(generateBodyIR(2304, 3.1f));

    // Wild bank.
    irBank_.push_back(generateIR(384, 0.10f, 0.98f, 0.98f));
    irBank_.push_back(generateIR(3072, 5.60f, 0.25f, 0.95f));
    irBank_.push_back(generateMorseIR(4096, 0.96f));
    irBank_.push_back(generateBodyIR(4096, 7.2f));
    irBank_.push_back(generateIR(819, 0.23f, 0.99f, 0.12f));
    irBank_.push_back(generateMorseIR(1200, 0.10f));
    irBank_.push_back(generateBodyIR(5120, 0.4f));
    irBank_.push_back(generateIR(4608, 7.80f, 1.0f, 0.50f));

    // Distort/scramble wild entries for stronger character.
    for (std::size_t b = 8; b < irBank_.size(); ++b) {
        auto& ir = irBank_[b];
        for (std::size_t i = 0; i < ir.size(); ++i) {
            if ((i % 17) == 0) {
                ir[i] = -ir[i];
            }
            if ((i % 31) == 0) {
                ir[i] *= 1.8f;
            }
            ir[i] = std::tanh(ir[i] * 2.8f);
        }
    }
}

std::vector<float> WeirdConvolutionReverb::generateIR(std::size_t length, float decaySeconds, float diffusion, float tone) {
    std::vector<float> ir(length, 0.0f);
    std::mt19937 localRng(static_cast<uint32_t>(length * 1337 + static_cast<int>(decaySeconds * 100.0f)));
    std::uniform_real_distribution<float> noise(-1.0f, 1.0f);

    const float sr = 48000.0f;
    const float decay = std::max(0.02f, decaySeconds);
    float lpState = 0.0f;

    for (std::size_t i = 0; i < length; ++i) {
        const float t = static_cast<float>(i) / sr;
        const float env = std::exp(-t / decay);
        const float excitation = noise(localRng) * (0.25f + 0.75f * diffusion);
        const float alpha = 0.02f + 0.35f * tone;
        lpState += alpha * (excitation - lpState);
        const float shimmer = std::sin(2.0f * kPi * (120.0f + 1200.0f * tone) * t) * 0.07f;
        ir[i] = env * (lpState + shimmer);
    }

    if (!ir.empty()) {
        ir[0] += 1.0f;
    }
    return ir;
}

std::vector<float> WeirdConvolutionReverb::generateMorseIR(std::size_t length, float density) {
    std::vector<float> ir(length, 0.0f);
    const int step = std::max(6, static_cast<int>(64.0f - density * 52.0f));
    for (std::size_t i = 0; i < length; ++i) {
        const float env = std::exp(-static_cast<float>(i) / static_cast<float>(length) * 4.0f);
        const bool gate = ((i / static_cast<std::size_t>(step)) % 5) < 2;
        const float tone = std::sin(2.0f * kPi * (330.0f + 90.0f * density) * static_cast<float>(i) / 48000.0f);
        ir[i] = (gate ? 0.65f : -0.18f) * env * tone;
    }
    if (!ir.empty()) {
        ir[0] += 0.9f;
    }
    return ir;
}

std::vector<float> WeirdConvolutionReverb::generateBodyIR(std::size_t length, float pulseHz) {
    std::vector<float> ir(length, 0.0f);
    for (std::size_t i = 0; i < length; ++i) {
        const float t = static_cast<float>(i) / 48000.0f;
        const float breath = std::max(0.0f, std::sin(2.0f * kPi * pulseHz * t));
        const float click = (std::fmod(static_cast<float>(i), 347.0f) < 4.0f) ? 0.6f : 0.0f;
        const float env = std::exp(-t * 2.2f);
        ir[i] = env * (0.45f * breath + click);
    }
    if (!ir.empty()) {
        ir[0] += 0.55f;
    }
    return ir;
}

std::vector<float> WeirdConvolutionReverb::morphIR(const std::vector<float>& a, const std::vector<float>& b, float t) {
    const std::size_t n = std::max(a.size(), b.size());
    std::vector<float> out(n, 0.0f);
    for (std::size_t i = 0; i < n; ++i) {
        const float av = i < a.size() ? a[i] : 0.0f;
        const float bv = i < b.size() ? b[i] : 0.0f;
        out[i] = av + (bv - av) * t;
    }
    return out;
}

void WeirdConvolutionReverb::updateFeatureTracking(float mono) {
    featureEnvelope_ = featureEnvelope_ * 0.993f + std::abs(mono) * 0.007f;
    const float hf = std::abs(mono - previousMono_);
    featureBrightness_ = featureBrightness_ * 0.986f + hf * 0.014f;
    previousMono_ = mono;
}

void WeirdConvolutionReverb::updateLivingIR() {
    const float instability = 1.0f - dynamicStability_;
    const float movingIndexA = clamp01(featureEnvelope_ * 5.0f + controls_.entropy * 0.35f + instability * 0.2f);
    const float movingIndexB = clamp01(featureBrightness_ * 7.5f + controls_.memory * 0.25f);

    const std::size_t bankStart = controls_.wildIrBank ? 8u : 0u;
    const std::size_t bankCount = 8u;

    const auto sampleBank = [this, bankStart](float idx) {
        const float pos = idx * static_cast<float>(bankCount - 1);
        const std::size_t i0 = static_cast<std::size_t>(pos);
        const std::size_t i1 = std::min(i0 + 1, bankCount - 1);
        return morphIR(irBank_[bankStart + i0], irBank_[bankStart + i1], pos - static_cast<float>(i0));
    };

    std::vector<float> baseA = sampleBank(movingIndexA);
    std::vector<float> baseB = sampleBank(movingIndexB);

    const float modeSkew = static_cast<float>(static_cast<int>(mode_)) / static_cast<float>(modeCount() - 1);
    const float morph = 0.5f + 0.48f * std::sin(static_cast<float>(frameCounter_) * (0.0007f + modeSkew * 0.0005f));

    activeIRMid_ = morphIR(baseA, baseB, morph);
    activeIRLow_ = morphIR(baseA, irBank_[bankStart], 0.4f + 0.5f * controls_.memory);
    activeIRHigh_ = morphIR(baseB, irBank_[bankStart + bankCount - 1], 0.45f + 0.45f * controls_.entropy);

    applyElasticTime(activeIRLow_);
    applyElasticTime(activeIRMid_);
    applyElasticTime(activeIRHigh_);

    applyIRModulation(activeIRLow_);
    applyIRModulation(activeIRMid_);
    applyIRModulation(activeIRHigh_);

    applySpectralMisalignment(activeIRMid_);
    applySpectralMisalignment(activeIRHigh_);

    if (mode_ == WeirdMode::UncannyCausality || instability > 0.6f) {
        zeroIndex_ = std::min<std::size_t>(activeIRMid_.size() / 2, 256);
        const std::size_t early = std::max<std::size_t>(16, activeIRMid_.size() / 5);
        std::reverse(activeIRMid_.begin(), activeIRMid_.begin() + early);
        if (!activeIRLow_.empty()) {
            std::reverse(activeIRLow_.begin(), activeIRLow_.begin() + std::min<std::size_t>(activeIRLow_.size(), 48));
        }
    } else {
        zeroIndex_ = 0;
    }

    if (mode_ == WeirdMode::HabitRoom || mode_ == WeirdMode::RainforestMemory) {
        const float learn = 0.00005f + 0.005f * instability;
        for (std::size_t i = 0; i < activeIRMid_.size(); ++i) {
            const float h = feedbackHistory_[(historyWrite_ + feedbackHistory_.size() - i - 1) % feedbackHistory_.size()];
            activeIRMid_[i] += h * learn;
        }
    }

    if (mode_ == WeirdMode::DigitalFailure && randomUniform(0.0f, 1.0f) < controls_.entropy * 0.28f) {
        // Blockwise broken update: leave one band stale.
        if (randomUniform(0.0f, 1.0f) < 0.5f) {
            activeIRLow_.assign(activeIRMid_.begin(), activeIRMid_.begin() + std::min(activeIRMid_.size(), activeIRLow_.size()));
        } else {
            activeIRHigh_.assign(activeIRMid_.begin(), activeIRMid_.begin() + std::min(activeIRMid_.size(), activeIRHigh_.size()));
        }
    }
}

void WeirdConvolutionReverb::applyIRModulation(std::vector<float>& ir) {
    if (ir.empty()) {
        return;
    }

    const float instability = 1.0f - dynamicStability_;
    const float lfo = std::sin(static_cast<float>(frameCounter_) * (0.00028f + 0.0004f * controls_.entropy));
    const float microSpeed = 1.0f + lfo * (0.08f + 0.23f * instability);

    std::vector<float> resampled(ir.size(), 0.0f);
    for (std::size_t i = 0; i < resampled.size(); ++i) {
        const float src = static_cast<float>(i) * microSpeed;
        const std::size_t i0 = static_cast<std::size_t>(src);
        const std::size_t i1 = std::min(i0 + 1, ir.size() - 1);
        const float t = src - static_cast<float>(i0);
        const float a = i0 < ir.size() ? ir[i0] : 0.0f;
        const float b = i1 < ir.size() ? ir[i1] : 0.0f;
        resampled[i] = a + (b - a) * t;
    }
    ir.swap(resampled);

    const std::size_t grainStep = std::max<std::size_t>(4, static_cast<std::size_t>(18 - controls_.entropy * 12.0f));
    for (std::size_t i = grainStep; i < ir.size(); i += grainStep) {
        const std::size_t jitter = static_cast<std::size_t>(randomUniform(0.0f, 18.0f + controls_.entropy * 24.0f));
        const std::size_t j = std::min(ir.size() - 1, i + jitter);
        std::swap(ir[i], ir[j]);
    }

    const int bits = static_cast<int>(2 + controls_.coherence * 10.0f + dynamicStability_ * 4.0f);
    const float q = static_cast<float>(1 << bits);
    const int hold = 1 + static_cast<int>(controls_.entropy * 9.0f + instability * 4.0f);

    for (std::size_t i = 0; i < ir.size(); ++i) {
        if ((i % static_cast<std::size_t>(hold)) != 0) {
            ir[i] = ir[i - (i % static_cast<std::size_t>(hold))];
        }

        ir[i] = std::round(ir[i] * q) / q;

        const float drive = 1.1f + 4.0f * controls_.memory + 3.0f * instability;
        ir[i] = softClip(ir[i] * drive);
    }
}

void WeirdConvolutionReverb::applyElasticTime(std::vector<float>& ir) {
    if (ir.empty()) {
        return;
    }

    float breathing = 1.0f;
    const float instability = 1.0f - dynamicStability_;
    float breathHz = std::max(0.01f, controls_.breathRateHz);
    if (controls_.tempoSync) {
        const float beats = std::max(0.0625f, controls_.breathBeats);
        const float beatHz = std::max(30.0f, controls_.bpm) / 60.0f;
        breathHz = beatHz / beats;
    }
    if (mode_ == WeirdMode::LivingSignal || mode_ == WeirdMode::Afterimage || mode_ == WeirdMode::HabitRoom || mode_ == WeirdMode::UncannyCausality) {
        const float phase = 2.0f * kPi * breathHz * (static_cast<float>(frameCounter_) / static_cast<float>(sampleRate_));
        const float lfo = 0.5f + 0.5f * std::sin(phase * (0.8f + instability * 1.8f));
        breathing = 1.0f + (lfo * 2.0f - 1.0f) * (0.65f + 0.95f * controls_.breathDepth);
        breathing = std::max(0.35f, breathing);
    }

    const std::size_t outSize = std::max<std::size_t>(128, static_cast<std::size_t>(static_cast<float>(ir.size()) * breathing));
    std::vector<float> stretched(outSize, 0.0f);

    for (std::size_t i = 0; i < outSize; ++i) {
        const float src = static_cast<float>(i) / std::max(0.001f, breathing);
        const std::size_t i0 = std::min<std::size_t>(static_cast<std::size_t>(src), ir.size() - 1);
        const std::size_t i1 = std::min<std::size_t>(i0 + 1, ir.size() - 1);
        const float t = src - static_cast<float>(i0);
        stretched[i] = ir[i0] + (ir[i1] - ir[i0]) * t;
    }

    if ((mode_ == WeirdMode::Afterimage || mode_ == WeirdMode::SpectralGhost) && (frameCounter_ % 5 == 0)) {
        const std::size_t start = static_cast<std::size_t>(randomUniform(0.0f, static_cast<float>(stretched.size() * 0.70f)));
        const std::size_t len = std::min<std::size_t>(96, stretched.size() - start);
        float hold = 0.0f;
        for (std::size_t i = 0; i < len; ++i) {
            hold = 0.985f * hold + 0.015f * stretched[start + i];
            stretched[start + i] = hold;
        }
    }

    ir.swap(stretched);
}

void WeirdConvolutionReverb::applySpectralMisalignment(std::vector<float>& ir) {
    if (ir.size() < 4) {
        return;
    }

    const bool doIt = (mode_ == WeirdMode::SpectralGhost || mode_ == WeirdMode::ProcessImprint || mode_ == WeirdMode::Afterimage || controls_.coherence < 0.6f);
    if (!doIt) {
        return;
    }

    const float instability = 1.0f - dynamicStability_;
    const float rot = 0.03f + 0.9f * (1.0f - controls_.coherence) + 0.5f * instability;
    for (std::size_t i = 2; i < ir.size(); ++i) {
        const float a = ir[i];
        const float b = ir[i - 1];
        ir[i] = a * std::cos(rot) - b * std::sin(rot);
    }

    for (std::size_t i = 0; i < ir.size(); i += 8) {
        if (randomUniform(0.0f, 1.0f) < controls_.entropy * (0.2f + 0.5f * instability)) {
            ir[i] = -ir[i];
        }
    }
}

float WeirdConvolutionReverb::sampleHistory(int delay) const {
    const std::size_t n = inputHistory_.size();
    const std::size_t idx = (historyWrite_ + n - (static_cast<std::size_t>(std::max(delay, 0)) % n)) % n;
    return inputHistory_[idx];
}

float WeirdConvolutionReverb::convolveSample(float inputSample, const std::vector<float>& ir, std::size_t zeroIndex) {
    float wet = 0.0f;
    const std::size_t irSize = std::min(ir.size(), inputHistory_.size() - 1);
    if (irSize == 0) {
        return 0.0f;
    }

    const float instability = 1.0f - dynamicStability_;
    const float feedbackAmt = (0.01f + 0.25f * controls_.memory + 0.12f * instability) * (1.0f - 0.72f * controls_.resistance);
    const std::size_t tapCap = std::min<std::size_t>(
        irSize,
        (mode_ == WeirdMode::DigitalFailure ? 64u : 112u) + static_cast<std::size_t>(dynamicStability_ * 192.0f));
    const std::size_t stride = std::max<std::size_t>(2, 2 + static_cast<std::size_t>(instability * 5.0f + controls_.entropy * 3.0f));

    for (std::size_t k = 0; k < tapCap; k += stride) {
        int delay = static_cast<int>(k) - static_cast<int>(zeroIndex);
        if (delay < 0) {
            delay = 0;
        }

        float x = sampleHistory(delay);
        if (mode_ == WeirdMode::RainforestMemory || mode_ == WeirdMode::HabitRoom || instability > 0.7f) {
            const std::size_t fi = (historyWrite_ + feedbackHistory_.size() - k - 1) % feedbackHistory_.size();
            x += feedbackHistory_[fi] * feedbackAmt;
        }

        if (mode_ == WeirdMode::DigitalFailure && (k % 5 == 0) && randomUniform(0.0f, 1.0f) < controls_.entropy * 0.35f) {
            continue;
        }

        wet += x * ir[k];
    }

    wet += inputSample * 0.10f;
    wet = softClip(wet);

    if (mode_ == WeirdMode::RainforestMemory || mode_ == WeirdMode::HabitRoom) {
        const float freq = 50.0f + 2100.0f * clamp01(learnedBias_ + controls_.entropy * 0.5f);
        const float resonant = std::sin(2.0f * kPi * freq * static_cast<float>(frameCounter_) / static_cast<float>(sampleRate_));
        wet += resonant * featureEnvelope_ * (0.03f + 0.22f * instability);
    }

    return wet;
}

float WeirdConvolutionReverb::randomUniform(float lo, float hi) {
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(rng_);
}

void WeirdConvolutionReverb::processBlock(float* left, float* right, std::size_t numSamples, const float* stabilityCv, float cvAmount) {
    for (std::size_t i = 0; i < numSamples; ++i) {
        const float cv = stabilityCv != nullptr ? stabilityCv[i] : 0.0f;
        dynamicStability_ = clamp01(controls_.stability + cvAmount * cv);
        const float instability = 1.0f - dynamicStability_;

        const float inL = left[i];
        const float inR = right[i];
        const float monoIn = 0.5f * (inL + inR);

        const float lofiDepth = clamp01(0.35f + 0.45f * controls_.entropy + 0.35f * instability);
        lofiHoldPeriod_ = 1 + static_cast<std::size_t>(lofiDepth * (mode_ == WeirdMode::DigitalFailure ? 22.0f : 12.0f));
        const bool refreshLoFiFrame = (lofiHoldCounter_++ % lofiHoldPeriod_) == 0;
        if (refreshLoFiFrame && !controls_.freeze) {
            lofiInputHeld_ = monoIn;
        }

        lofiWowPhase_ += 0.00011f + 0.0018f * lofiDepth;
        if (lofiWowPhase_ > 1.0f) {
            lofiWowPhase_ -= 1.0f;
        }
        const float wow = std::sin(2.0f * kPi * lofiWowPhase_);
        const float mono = lofiInputHeld_ * (1.0f + wow * (0.02f + 0.06f * lofiDepth));
        if (controls_.freeze) {
            inputHistory_[historyWrite_] *= 0.998f;
        } else {
            inputHistory_[historyWrite_] = mono;
        }

        updateFeatureTracking(mono);

        const std::size_t baseRate = std::max<std::size_t>(16, blockSize_ / 2);
        const std::size_t updateRate = (mode_ == WeirdMode::DigitalFailure)
            ? std::max<std::size_t>(24, baseRate * 8)
            : baseRate;
        if (!controls_.freeze && (frameCounter_++ % updateRate) == 0) {
            updateLivingIR();
        }

        // Three-band split: different IRs can occupy different spaces.
        lpState_ += 0.08f * (mono - lpState_);
        const float low = lpState_;
        const float hpIn = mono - lpState_;
        hpState_ += 0.04f * (hpIn - hpState_);
        const float high = hpIn - hpState_;
        const float mid = mono - low - high;

        std::size_t lowZero = zeroIndex_ / 2;
        std::size_t midZero = zeroIndex_;
        std::size_t highZero = zeroIndex_ + static_cast<std::size_t>(instability * 48.0f);

        const bool swapBands = (mode_ == WeirdMode::SpectralGhost || mode_ == WeirdMode::ProcessImprint)
            && (((frameCounter_ / 1024) % 2) == 0);
        const auto& irLow = swapBands ? activeIRHigh_ : activeIRLow_;
        const auto& irHigh = swapBands ? activeIRLow_ : activeIRHigh_;

        float wet = lofiWetHeld_;
        if (refreshLoFiFrame && !controls_.freeze) {
            const float wetLow = convolveSample(low, irLow, lowZero);
            const float wetMid = convolveSample(mid, activeIRMid_, midZero);
            const float wetHigh = convolveSample(high, irHigh, highZero);
            wet = 0.55f * wetLow + 0.95f * wetMid + 1.25f * wetHigh;
            lofiWetHeld_ = wet;
        }

        if (mode_ == WeirdMode::Afterimage || mode_ == WeirdMode::HabitRoom) {
            learnedBias_ = 0.998f * learnedBias_ + 0.002f * clamp01(featureBrightness_ * 16.0f + featureEnvelope_ * 3.0f);
            const float residue = std::sin(2.0f * kPi * (140.0f + learnedBias_ * 1600.0f) * static_cast<float>(frameCounter_) / static_cast<float>(sampleRate_));
            wet += residue * (0.01f + 0.12f * controls_.memory * instability);
        }

        if (instability > 0.75f || mode_ == WeirdMode::HabitRoom) {
            autonomousDronePhase_ += (0.0006f + 0.0022f * controls_.entropy + 0.001f * featureEnvelope_);
            if (autonomousDronePhase_ > 1.0f) {
                autonomousDronePhase_ -= 1.0f;
            }
            const float drone = std::sin(2.0f * kPi * autonomousDronePhase_)
                + 0.35f * std::sin(2.0f * kPi * autonomousDronePhase_ * 2.618f);
            wet += drone * 0.08f * instability;
        }

        if (mode_ == WeirdMode::DigitalFailure) {
            if (randomUniform(0.0f, 1.0f) < controls_.entropy * 0.02f) {
                wet = -wet;
            }
            const float step = 1.0f / (3.0f + controls_.coherence * 12.0f);
            wet = std::round(wet / step) * step;
        }

        // Intentional low-rate zippering + dynamic bit-depth drift as part of the aesthetic.
        const float loFiStep = 1.0f / (6.0f + controls_.coherence * 10.0f + dynamicStability_ * 8.0f);
        if ((i % (2 + static_cast<std::size_t>(lofiDepth * 6.0f))) != 0) {
            wet = lofiWetHeld_;
        }
        wet = std::round(wet / loFiStep) * loFiStep;

        float outL = controls_.dry * inL + controls_.wet * wet;
        float outR = controls_.dry * inR + controls_.wet * wet;

        if (mode_ == WeirdMode::AntiSpace) {
            const float wide = std::abs(inL - inR);
            const float collapse = clamp01(1.0f - wide * 4.3f - instability * 0.3f);
            const float monoWet = 0.5f * (outL + outR);
            outL = monoWet + (outL - monoWet) * collapse * (0.12f + 0.88f * controls_.coherence);
            outR = monoWet + (outR - monoWet) * collapse * (0.12f + 0.88f * controls_.coherence);

            if (i < 96) {
                const float pan = 0.25f + 0.5f * controls_.entropy;
                outL += inR * pan;
                outR += inL * pan;
            }
        }

        left[i] = softClip(outL);
        right[i] = softClip(outR);
        feedbackHistory_[historyWrite_] = wet;
        historyWrite_ = (historyWrite_ + 1) % inputHistory_.size();
    }
}

} // namespace verbsuite
