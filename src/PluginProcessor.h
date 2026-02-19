#pragma once

#include "VerbSuite/WeirdConvolutionReverb.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include <vector>

class VerbSuiteAudioProcessor : public juce::AudioProcessor {
public:
    VerbSuiteAudioProcessor();
    ~VerbSuiteAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState& parameters() { return parameters_; }
    float getStabilityCvMeter() const noexcept { return stabilityCvMeter_.load(); }

private:
    static verbsuite::WeirdControls controlsFromModeAndStability(
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
        bool freeze);

    juce::AudioProcessorValueTreeState parameters_;
    std::unique_ptr<verbsuite::WeirdConvolutionReverb> engine_;
    std::unique_ptr<verbsuite::WeirdConvolutionReverb> engineHQ_;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling_;
    std::atomic<float> stabilityCvMeter_ { 0.0f };
    float cvEnvelopeState_ = 0.0f;
    float cvEnvelopeHeld_ = 0.0f;
    int cvEnvelopeStepCounter_ = 0;
    int currentProgram_ = 0;
    bool previousFreezeParam_ = false;
    int freezeMomentarySamplesRemaining_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerbSuiteAudioProcessor)
};
