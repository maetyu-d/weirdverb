#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace verbsuite {

enum class WeirdMode : std::uint8_t {
    LivingSignal,
    UncannyCausality,
    SpectralGhost,
    RainforestMemory,
    ProcessImprint,
    DigitalFailure,
    AntiSpace,
    Afterimage,
    HabitRoom
};

struct WeirdControls {
    float memory = 0.5f;      // Replaces room size.
    float coherence = 0.5f;   // Phase/image lock.
    float entropy = 0.5f;     // Disorder probability.
    float resistance = 0.5f;  // Feedback damping.
    float stability = 0.5f;   // Realistic -> unstable -> autonomous.
    float breathRateHz = 0.25f;
    float breathDepth = 1.0f;
    float breathBeats = 1.0f; // Used when tempoSync is true.
    float bpm = 120.0f;
    bool tempoSync = false;
    bool wildIrBank = false;
    bool freeze = false;
    float wet = 0.7f;
    float dry = 0.3f;
};

class WeirdConvolutionReverb {
public:
    WeirdConvolutionReverb(double sampleRate, std::size_t blockSize, WeirdMode mode);

    void setMode(WeirdMode newMode);
    void setControls(const WeirdControls& newControls);

    void reset();
    void processBlock(float* left, float* right, std::size_t numSamples, const float* stabilityCv = nullptr, float cvAmount = 0.0f);

    [[nodiscard]] std::string modeName() const;

    static constexpr int modeCount() noexcept { return 9; }
    static std::string modeName(WeirdMode mode);
    static WeirdMode modeFromIndex(int index);

private:
    void buildIRBank();
    void updateFeatureTracking(float mono);
    void updateLivingIR();
    void applyIRModulation(std::vector<float>& ir);
    void applySpectralMisalignment(std::vector<float>& ir);
    void applyElasticTime(std::vector<float>& ir);

    [[nodiscard]] float convolveSample(float inputSample, const std::vector<float>& ir, std::size_t zeroIndex);
    [[nodiscard]] float sampleHistory(int delay) const;
    [[nodiscard]] float randomUniform(float lo, float hi);

    static std::vector<float> generateIR(std::size_t length, float decaySeconds, float diffusion, float tone);
    static std::vector<float> generateMorseIR(std::size_t length, float density);
    static std::vector<float> generateBodyIR(std::size_t length, float pulseHz);
    static std::vector<float> morphIR(const std::vector<float>& a, const std::vector<float>& b, float t);

    double sampleRate_;
    std::size_t blockSize_;

    WeirdMode mode_;
    WeirdControls controls_;

    std::vector<std::vector<float>> irBank_;
    std::vector<float> activeIRLow_;
    std::vector<float> activeIRMid_;
    std::vector<float> activeIRHigh_;

    std::vector<float> inputHistory_;
    std::vector<float> feedbackHistory_;
    std::size_t historyWrite_ = 0;

    float featureEnvelope_ = 0.0f;
    float featureBrightness_ = 0.0f;
    float previousMono_ = 0.0f;
    float learnedBias_ = 0.0f;
    float autonomousDronePhase_ = 0.0f;

    float lpState_ = 0.0f;
    float hpState_ = 0.0f;

    std::size_t frameCounter_ = 0;
    std::size_t zeroIndex_ = 0;
    float dynamicStability_ = 0.5f;
    std::size_t lofiHoldCounter_ = 0;
    std::size_t lofiHoldPeriod_ = 1;
    float lofiInputHeld_ = 0.0f;
    float lofiWetHeld_ = 0.0f;
    float lofiWowPhase_ = 0.0f;

    std::mt19937 rng_;
};

} // namespace verbsuite
