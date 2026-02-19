#pragma once

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

class VerbSuiteAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit VerbSuiteAudioProcessorEditor(VerbSuiteAudioProcessor&);
    ~VerbSuiteAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VerbSuiteAudioProcessor& processor_;

    juce::ComboBox modeBox_;
    juce::ComboBox breathSyncBox_;
    juce::ComboBox presetBox_;
    juce::ComboBox cvModeBox_;
    juce::ComboBox irBankBox_;
    juce::ComboBox cvSmoothingBox_;
    juce::ComboBox cvFilterTimeBox_;
    juce::ComboBox freezeModeBox_;
    juce::Slider stabilitySlider_;
    juce::Slider breathRateSlider_;
    juce::Slider breathDepthSlider_;
    juce::Slider cvAmountSlider_;
    juce::Slider drySlider_;
    juce::Slider wetSlider_;
    juce::Slider outputSlider_;
    juce::ToggleButton freezeButton_;
    juce::ToggleButton hqExportButton_;
    juce::ToggleButton lockCoreButton_;
    juce::TextButton randomizeButton_;

    juce::Label title_;
    juce::Label modeLabel_;
    juce::Label stabilityLabel_;
    juce::Label presetLabel_;
    juce::Label irBankLabel_;
    juce::Label breathSyncLabel_;
    juce::Label breathRateLabel_;
    juce::Label breathDepthLabel_;
    juce::Label cvModeLabel_;
    juce::Label cvAmountLabel_;
    juce::Label cvSmoothingLabel_;
    juce::Label cvFilterTimeLabel_;
    juce::Label freezeModeLabel_;
    juce::Label dryLabel_;
    juce::Label wetLabel_;
    juce::Label outputLabel_;

    using ChoiceAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ChoiceAttachment> modeAttachment_;
    std::unique_ptr<ChoiceAttachment> irBankAttachment_;
    std::unique_ptr<ChoiceAttachment> breathSyncAttachment_;
    std::unique_ptr<ChoiceAttachment> freezeModeAttachment_;
    std::unique_ptr<ChoiceAttachment> cvModeAttachment_;
    std::unique_ptr<ChoiceAttachment> cvSmoothingAttachment_;
    std::unique_ptr<ChoiceAttachment> cvFilterTimeAttachment_;
    std::unique_ptr<SliderAttachment> stabilityAttachment_;
    std::unique_ptr<SliderAttachment> breathRateAttachment_;
    std::unique_ptr<SliderAttachment> breathDepthAttachment_;
    std::unique_ptr<SliderAttachment> cvAmountAttachment_;
    std::unique_ptr<SliderAttachment> dryAttachment_;
    std::unique_ptr<SliderAttachment> wetAttachment_;
    std::unique_ptr<SliderAttachment> outputAttachment_;
    std::unique_ptr<ButtonAttachment> freezeAttachment_;
    std::unique_ptr<ButtonAttachment> hqExportAttachment_;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerbSuiteAudioProcessorEditor)
};
