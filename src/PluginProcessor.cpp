#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace {
constexpr const char* kModeParam = "mode";
constexpr const char* kStabilityParam = "stability";
constexpr const char* kBreathSyncParam = "breath_sync";
constexpr const char* kBreathRateParam = "breath_rate";
constexpr const char* kBreathDepthParam = "breath_depth";
constexpr const char* kCvModeParam = "stability_cv_mode";
constexpr const char* kCvAmountParam = "stability_cv_amount";
constexpr const char* kCvSmoothingParam = "stability_cv_smoothing";
constexpr const char* kCvFilterTimeParam = "stability_cv_filter_time";
constexpr const char* kDryParam = "dry";
constexpr const char* kWetParam = "wet";
constexpr const char* kOutputParam = "output";
constexpr const char* kIrBankParam = "ir_bank";
constexpr const char* kFreezeParam = "freeze";
constexpr const char* kFreezeModeParam = "freeze_mode";
constexpr const char* kOversampleHqParam = "oversample_hq";

float breathSyncToBeats(int syncIndex) {
    switch (syncIndex) {
    case 1: return 4.0f;
    case 2: return 2.0f;
    case 3: return 1.0f;
    case 4: return 0.5f;
    case 5: return 0.25f;
    case 6: return 1.0f / 3.0f;
    case 7: return 1.5f;
    default: return 1.0f;
    }
}

float cvFilterTimeSeconds(int index) {
    switch (index) {
    case 1: return 0.120f;
    case 2: return 0.600f;
    default: return 0.020f;
    }
}

struct FactoryPreset {
    const char* name;
    int mode;
    int irBank;
    float stability;
    int breathSync;
    float breathRate;
    float breathDepth;
    int cvMode;
    float cvAmount;
    int cvSmoothing;
    int cvFilterTime;
    float dry;
    float wet;
    float output;
    int freezeMode;
    bool oversampleHq;
};

const std::array<FactoryPreset, 10> kFactoryPresets {{
    { "Null Tape",        0, 0, 0.97f, 1, 0.06f, 0.18f, 0,  0.00f, 0, 0, 0.74f, 0.38f, 0.92f, 0, false },
    { "Precrime Bloom",   1, 0, 0.06f, 5, 4.80f, 1.00f, 0,  0.00f, 1, 0, 0.15f, 0.95f, 0.84f, 0, true  },
    { "Shattered Prism",  2, 1, 0.18f, 6, 6.20f, 0.94f, 1,  0.92f, 0, 0, 0.30f, 0.90f, 0.78f, 1, false },
    { "Swamp Antenna",    3, 1, 0.08f, 2, 0.14f, 1.00f, 1, -0.88f, 1, 2, 0.08f, 1.00f, 0.72f, 1, true  },
    { "Packet Graveyard", 4, 1, 0.31f, 0, 7.70f, 0.86f, 1,  0.75f, 0, 0, 0.42f, 0.86f, 0.88f, 0, false },
    { "Broken Choir RAM", 5, 1, 0.03f, 5, 7.95f, 1.00f, 1,  1.00f, 0, 0, 0.10f, 1.00f, 0.68f, 0, false },
    { "Inverted Corridor",6, 0, 0.83f, 3, 0.55f, 0.41f, 1, -0.46f, 1, 1, 0.92f, 0.46f, 0.95f, 0, false },
    { "Ash Afterimage",   7, 1, 0.12f, 7, 0.22f, 1.00f, 1, -1.00f, 1, 2, 0.20f, 0.96f, 0.80f, 1, true  },
    { "Self-Taught Room", 8, 1, 0.01f, 0, 8.00f, 1.00f, 1,  1.00f, 0, 0, 0.00f, 1.00f, 0.70f, 0, false },
    { "Lofi Leviathan",   0, 1, 0.22f, 4, 3.60f, 1.00f, 1,  0.66f, 1, 2, 0.25f, 0.95f, 0.76f, 0, true  },
}};

void setChoiceParameter(juce::AudioProcessorValueTreeState& params, const char* id, int value) {
    if (auto* p = params.getParameter(id)) {
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(value)));
    }
}

void setFloatParameter(juce::AudioProcessorValueTreeState& params, const char* id, float value) {
    if (auto* p = params.getParameter(id)) {
        p->setValueNotifyingHost(p->convertTo0to1(value));
    }
}

void setBoolParameter(juce::AudioProcessorValueTreeState& params, const char* id, bool value) {
    if (auto* p = params.getParameter(id)) {
        p->setValueNotifyingHost(value ? 1.0f : 0.0f);
    }
}
} // namespace

VerbSuiteAudioProcessor::VerbSuiteAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this, nullptr, "PARAMETERS", createParameterLayout()) {
    setCurrentProgram(0);
}

void VerbSuiteAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    engine_ = std::make_unique<verbsuite::WeirdConvolutionReverb>(sampleRate, static_cast<std::size_t>(samplesPerBlock), verbsuite::WeirdMode::LivingSignal);
    engineHQ_ = std::make_unique<verbsuite::WeirdConvolutionReverb>(sampleRate * 2.0, static_cast<std::size_t>(samplesPerBlock * 2), verbsuite::WeirdMode::LivingSignal);
    engine_->reset();
    engineHQ_->reset();

    oversampling_ = std::make_unique<juce::dsp::Oversampling<float>>(2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    oversampling_->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling_->reset();

    stabilityCvMeter_.store(0.0f);
    cvEnvelopeState_ = 0.0f;
    cvEnvelopeHeld_ = 0.0f;
    cvEnvelopeStepCounter_ = 0;
    previousFreezeParam_ = false;
    freezeMomentarySamplesRemaining_ = 0;
}

void VerbSuiteAudioProcessor::releaseResources() {
    engine_.reset();
    engineHQ_.reset();
    oversampling_.reset();
}

bool VerbSuiteAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) {
        return false;
    }
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
        return false;
    }

    const auto sidechain = layouts.getChannelSet(true, 1);
    if (!sidechain.isDisabled() && sidechain != juce::AudioChannelSet::mono() && sidechain != juce::AudioChannelSet::stereo()) {
        return false;
    }
    return true;
}

void VerbSuiteAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    if (!engine_) {
        return;
    }

    auto mainBuffer = getBusBuffer(buffer, true, 0);
    auto* left = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() > 1 ? mainBuffer.getWritePointer(1) : mainBuffer.getWritePointer(0);

    const auto modeIndex = static_cast<int>(parameters_.getRawParameterValue(kModeParam)->load());
    const auto stability = parameters_.getRawParameterValue(kStabilityParam)->load();
    const auto breathSync = static_cast<int>(parameters_.getRawParameterValue(kBreathSyncParam)->load());
    const auto breathRateHz = parameters_.getRawParameterValue(kBreathRateParam)->load();
    const auto breathDepth = parameters_.getRawParameterValue(kBreathDepthParam)->load();
    const auto irBank = static_cast<int>(parameters_.getRawParameterValue(kIrBankParam)->load());
    const auto cvMode = static_cast<int>(parameters_.getRawParameterValue(kCvModeParam)->load());
    const auto cvAmount = parameters_.getRawParameterValue(kCvAmountParam)->load();
    const auto cvSmoothing = static_cast<int>(parameters_.getRawParameterValue(kCvSmoothingParam)->load());
    const auto cvFilterTime = static_cast<int>(parameters_.getRawParameterValue(kCvFilterTimeParam)->load());
    const auto dry = parameters_.getRawParameterValue(kDryParam)->load();
    const auto wet = parameters_.getRawParameterValue(kWetParam)->load();
    const auto output = parameters_.getRawParameterValue(kOutputParam)->load();
    const bool freezeParam = parameters_.getRawParameterValue(kFreezeParam)->load() > 0.5f;
    const int freezeMode = static_cast<int>(parameters_.getRawParameterValue(kFreezeModeParam)->load());
    const bool oversampleHq = parameters_.getRawParameterValue(kOversampleHqParam)->load() > 0.5f;

    bool freezeActive = freezeParam;
    if (freezeMode == 1) { // momentary
        if (freezeParam && !previousFreezeParam_) {
            freezeMomentarySamplesRemaining_ = std::max(1, static_cast<int>(0.22 * getSampleRate()));
        }
        freezeActive = freezeMomentarySamplesRemaining_ > 0;
        freezeMomentarySamplesRemaining_ = std::max(0, freezeMomentarySamplesRemaining_ - mainBuffer.getNumSamples());
    }
    previousFreezeParam_ = freezeParam;

    double bpm = 120.0;
    if (auto* playHead = getPlayHead()) {
        if (auto position = playHead->getPosition()) {
            if (auto hostBpm = position->getBpm()) {
                if (*hostBpm > 0.0) {
                    bpm = *hostBpm;
                }
            }
        }
    }

    const auto mode = verbsuite::WeirdConvolutionReverb::modeFromIndex(modeIndex);
    const bool tempoSync = breathSync > 0;
    const float breathBeats = breathSyncToBeats(breathSync);

    const auto controls = controlsFromModeAndStability(
        mode,
        stability,
        breathRateHz,
        breathDepth,
        tempoSync,
        breathBeats,
        static_cast<float>(bpm),
        irBank == 1,
        dry,
        wet,
        freezeActive);

    engine_->setMode(mode);
    engine_->setControls(controls);
    if (engineHQ_) {
        engineHQ_->setMode(mode);
        engineHQ_->setControls(controls);
    }

    const float* cvSignal = nullptr;
    std::vector<float> cvScratch;

    const float tau = cvFilterTimeSeconds(cvFilterTime);
    const float sr = std::max(1.0f, static_cast<float>(getSampleRate()));
    const float attackTau = std::max(0.001f, tau * 0.30f);
    const float releaseTau = std::max(0.005f, tau * 2.40f);
    const float attackAlpha = 1.0f - std::exp(-1.0f / (attackTau * sr));
    const float releaseAlpha = 1.0f - std::exp(-1.0f / (releaseTau * sr));
    const int stepPeriod = cvFilterTime == 0 ? 12 : (cvFilterTime == 1 ? 48 : 160);

    if (cvMode == 1 && getBusCount(true) > 1) {
        auto sidechainBuffer = getBusBuffer(buffer, true, 1);
        if (sidechainBuffer.getNumChannels() > 0) {
            cvScratch.resize(static_cast<std::size_t>(mainBuffer.getNumSamples()));
            const auto* scL = sidechainBuffer.getReadPointer(0);
            const auto* scR = sidechainBuffer.getNumChannels() > 1 ? sidechainBuffer.getReadPointer(1) : scL;

            float meterPeak = 0.0f;
            for (int i = 0; i < mainBuffer.getNumSamples(); ++i) {
                const float raw = juce::jlimit(-1.0f, 1.0f, 0.5f * (scL[i] + scR[i]));
                float shaped = raw;
                if (cvSmoothing == 1) {
                    const float target = std::abs(raw);
                    const float alpha = target > cvEnvelopeState_ ? attackAlpha : releaseAlpha;
                    cvEnvelopeState_ += alpha * (target - cvEnvelopeState_);
                    if ((cvEnvelopeStepCounter_++ % stepPeriod) == 0) {
                        cvEnvelopeHeld_ = cvEnvelopeState_;
                    }
                    shaped = cvEnvelopeHeld_;
                }
                cvScratch[static_cast<std::size_t>(i)] = shaped;
                meterPeak = std::max(meterPeak, std::abs(shaped));
            }
            const float held = std::max(stabilityCvMeter_.load() * 0.92f, meterPeak);
            stabilityCvMeter_.store(held);
            cvSignal = cvScratch.data();
        }
    }
    if (cvMode != 1) {
        stabilityCvMeter_.store(stabilityCvMeter_.load() * 0.90f);
    }

    const bool doHQ = oversampleHq && isNonRealtime() && oversampling_ && engineHQ_;
    if (doHQ) {
        juce::dsp::AudioBlock<float> block(mainBuffer);
        auto upBlock = oversampling_->processSamplesUp(block);

        auto* upL = upBlock.getChannelPointer(0);
        auto* upR = upBlock.getNumChannels() > 1 ? upBlock.getChannelPointer(1) : upBlock.getChannelPointer(0);

        std::vector<float> cvUp;
        const float* cvUpPtr = nullptr;
        if (cvSignal != nullptr) {
            const auto upSamples = static_cast<std::size_t>(upBlock.getNumSamples());
            cvUp.resize(upSamples, 0.0f);
            for (std::size_t i = 0; i < upSamples; ++i) {
                const std::size_t src = std::min<std::size_t>(cvScratch.size() - 1, i / 2);
                cvUp[i] = cvScratch[src];
            }
            cvUpPtr = cvUp.data();
        }

        engineHQ_->processBlock(upL, upR, static_cast<std::size_t>(upBlock.getNumSamples()), cvUpPtr, cvAmount);
        oversampling_->processSamplesDown(block);
    } else {
        engine_->processBlock(left, right, static_cast<std::size_t>(mainBuffer.getNumSamples()), cvSignal, cvAmount);
    }

    const float gain = juce::Decibels::decibelsToGain(output);
    mainBuffer.applyGain(gain);

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch) {
        buffer.clear(ch, 0, buffer.getNumSamples());
    }
}

juce::AudioProcessorEditor* VerbSuiteAudioProcessor::createEditor() {
    return new VerbSuiteAudioProcessorEditor(*this);
}

int VerbSuiteAudioProcessor::getNumPrograms() {
    return static_cast<int>(kFactoryPresets.size());
}

int VerbSuiteAudioProcessor::getCurrentProgram() {
    return currentProgram_;
}

void VerbSuiteAudioProcessor::setCurrentProgram(int index) {
    const int clamped = juce::jlimit(0, getNumPrograms() - 1, index);
    currentProgram_ = clamped;
    const auto& p = kFactoryPresets[static_cast<std::size_t>(clamped)];

    setChoiceParameter(parameters_, kModeParam, p.mode);
    setChoiceParameter(parameters_, kIrBankParam, p.irBank);
    setFloatParameter(parameters_, kStabilityParam, p.stability);
    setChoiceParameter(parameters_, kBreathSyncParam, p.breathSync);
    setFloatParameter(parameters_, kBreathRateParam, p.breathRate);
    setFloatParameter(parameters_, kBreathDepthParam, p.breathDepth);
    setChoiceParameter(parameters_, kCvModeParam, p.cvMode);
    setFloatParameter(parameters_, kCvAmountParam, p.cvAmount);
    setChoiceParameter(parameters_, kCvSmoothingParam, p.cvSmoothing);
    setChoiceParameter(parameters_, kCvFilterTimeParam, p.cvFilterTime);
    setFloatParameter(parameters_, kDryParam, p.dry);
    setFloatParameter(parameters_, kWetParam, p.wet);
    setFloatParameter(parameters_, kOutputParam, p.output);
    setChoiceParameter(parameters_, kFreezeModeParam, p.freezeMode);
    setBoolParameter(parameters_, kOversampleHqParam, p.oversampleHq);
    setBoolParameter(parameters_, kFreezeParam, false);
}

const juce::String VerbSuiteAudioProcessor::getProgramName(int index) {
    const int clamped = juce::jlimit(0, getNumPrograms() - 1, index);
    return kFactoryPresets[static_cast<std::size_t>(clamped)].name;
}

void VerbSuiteAudioProcessor::changeProgramName(int, const juce::String&) {
    // Factory preset names are fixed.
}

const juce::String VerbSuiteAudioProcessor::getName() const {
    return JucePlugin_Name;
}

void VerbSuiteAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VerbSuiteAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(parameters_.state.getType())) {
        parameters_.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout VerbSuiteAudioProcessor::createParameterLayout() {
    juce::StringArray modeNames;
    for (int i = 0; i < verbsuite::WeirdConvolutionReverb::modeCount(); ++i) {
        modeNames.add(verbsuite::WeirdConvolutionReverb::modeName(verbsuite::WeirdConvolutionReverb::modeFromIndex(i)));
    }

    juce::StringArray breathSyncNames { "Free", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/4D" };
    juce::StringArray cvModeNames { "Off", "Audio-rate" };
    juce::StringArray cvSmoothingNames { "Raw", "Envelope" };
    juce::StringArray cvFilterTimeNames { "Fast", "Medium", "Slow" };
    juce::StringArray freezeModeNames { "Latch", "Momentary" };
    juce::StringArray irBankNames { "Core", "Wild" };

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterChoice>(kModeParam, "Mode", modeNames, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(kIrBankParam, "IR Bank", irBankNames, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(kStabilityParam, "Stability", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(kBreathSyncParam, "Breath Sync", breathSyncNames, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(kBreathRateParam, "Breath Rate", juce::NormalisableRange<float>(0.03f, 8.0f, 0.001f, 0.4f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(kBreathDepthParam, "Breath Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(kCvModeParam, "Stability CV", cvModeNames, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(kCvAmountParam, "CV Amount", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(kCvSmoothingParam, "CV Smoothing", cvSmoothingNames, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(kCvFilterTimeParam, "CV Filter Time", cvFilterTimeNames, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(kDryParam, "Dry", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(kWetParam, "Wet", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.72f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(kOutputParam, "Output", juce::NormalisableRange<float>(-18.0f, 12.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(kFreezeParam, "Freeze", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(kFreezeModeParam, "Freeze Mode", freezeModeNames, 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>(kOversampleHqParam, "HQ Export", false));

    return { params.begin(), params.end() };
}

verbsuite::WeirdControls VerbSuiteAudioProcessor::controlsFromModeAndStability(
    verbsuite::WeirdMode mode,
    float stability,
    float breathRateHz,
    float breathDepth,
    bool tempoSync,
    float breathBeats,
    float bpm,
    bool wildIrBank,
    float dry,
    float wet,
    bool freeze) {
    const float s = juce::jlimit(0.0f, 1.0f, stability);
    const float u = 1.0f - s;

    verbsuite::WeirdControls c;
    c.stability = s;
    c.breathRateHz = juce::jlimit(0.03f, 8.0f, breathRateHz);
    c.breathDepth = juce::jlimit(0.0f, 1.0f, breathDepth);
    c.tempoSync = tempoSync;
    c.breathBeats = juce::jmax(0.0625f, breathBeats);
    c.bpm = juce::jlimit(30.0f, 260.0f, bpm);
    c.wildIrBank = wildIrBank;
    c.freeze = freeze;

    switch (mode) {
    case verbsuite::WeirdMode::LivingSignal:
        c.memory = 0.55f + 0.35f * u;
        c.coherence = 0.70f - 0.45f * u;
        c.entropy = 0.25f + 0.60f * u;
        c.resistance = 0.72f - 0.35f * u;
        break;
    case verbsuite::WeirdMode::UncannyCausality:
        c.memory = 0.70f + 0.25f * u;
        c.coherence = 0.62f - 0.48f * u;
        c.entropy = 0.32f + 0.58f * u;
        c.resistance = 0.68f - 0.40f * u;
        break;
    case verbsuite::WeirdMode::SpectralGhost:
        c.memory = 0.66f + 0.22f * u;
        c.coherence = 0.45f - 0.35f * u;
        c.entropy = 0.40f + 0.55f * u;
        c.resistance = 0.60f - 0.35f * u;
        break;
    case verbsuite::WeirdMode::RainforestMemory:
        c.memory = 0.80f + 0.20f * u;
        c.coherence = 0.48f - 0.32f * u;
        c.entropy = 0.36f + 0.48f * u;
        c.resistance = 0.55f - 0.30f * u;
        break;
    case verbsuite::WeirdMode::ProcessImprint:
        c.memory = 0.62f + 0.30f * u;
        c.coherence = 0.50f - 0.35f * u;
        c.entropy = 0.48f + 0.43f * u;
        c.resistance = 0.66f - 0.33f * u;
        break;
    case verbsuite::WeirdMode::DigitalFailure:
        c.memory = 0.48f + 0.42f * u;
        c.coherence = 0.58f - 0.52f * u;
        c.entropy = 0.52f + 0.46f * u;
        c.resistance = 0.62f - 0.37f * u;
        break;
    case verbsuite::WeirdMode::AntiSpace:
        c.memory = 0.54f + 0.30f * u;
        c.coherence = 0.74f - 0.40f * u;
        c.entropy = 0.20f + 0.66f * u;
        c.resistance = 0.70f - 0.35f * u;
        break;
    case verbsuite::WeirdMode::Afterimage:
        c.memory = 0.78f + 0.20f * u;
        c.coherence = 0.60f - 0.42f * u;
        c.entropy = 0.28f + 0.63f * u;
        c.resistance = 0.68f - 0.40f * u;
        break;
    case verbsuite::WeirdMode::HabitRoom:
        c.memory = 0.85f + 0.15f * u;
        c.coherence = 0.40f - 0.30f * u;
        c.entropy = 0.38f + 0.58f * u;
        c.resistance = 0.50f - 0.30f * u;
        break;
    }

    c.memory = juce::jlimit(0.0f, 1.0f, c.memory);
    c.coherence = juce::jlimit(0.0f, 1.0f, c.coherence);
    c.entropy = juce::jlimit(0.0f, 1.0f, c.entropy);
    c.resistance = juce::jlimit(0.0f, 1.0f, c.resistance);
    c.dry = juce::jlimit(0.0f, 1.0f, dry);
    c.wet = juce::jlimit(0.0f, 1.0f, wet);
    return c;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VerbSuiteAudioProcessor();
}
