#include "PluginEditor.h"

#include <random>

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
constexpr const char* kFreezeParam = "freeze";
constexpr const char* kFreezeModeParam = "freeze_mode";
constexpr const char* kOversampleHqParam = "oversample_hq";

void styleSmallLabel(juce::Label& label) {
    label.setColour(juce::Label::textColourId, juce::Colour(0xffd7c7b2).withAlpha(0.82f));
    label.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::plain)));
    label.setJustificationType(juce::Justification::centredLeft);
}

void styleCombo(juce::ComboBox& combo) {
    combo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff26211c));
    combo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff5a4a3a));
    combo.setColour(juce::ComboBox::textColourId, juce::Colour(0xffefe2d1));
    combo.setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff26211c));
    combo.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffb48861));
}

void styleKnob(juce::Slider& slider, const juce::String& suffix) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 74, 18);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff4a3e34));
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffb08d6e));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe8d9c7));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffeadac7));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff241f1a));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff5a4a3a));
}

void styleButton(juce::Button& button) {
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2c251f));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff6a4a31));
    button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffeadbc8));
    button.setColour(juce::TextButton::textColourOnId, juce::Colour(0xfffff2e2));
}

void setParamValue(juce::AudioProcessorValueTreeState& params, const char* id, float normalized01) {
    if (auto* p = params.getParameter(id)) {
        p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, normalized01));
    }
}
} // namespace

VerbSuiteAudioProcessorEditor::VerbSuiteAudioProcessorEditor(VerbSuiteAudioProcessor& audioProcessor)
    : juce::AudioProcessorEditor(&audioProcessor),
      processor_(audioProcessor) {
    juce::StringArray modeNames;
    for (int i = 0; i < verbsuite::WeirdConvolutionReverb::modeCount(); ++i) {
        modeNames.add(verbsuite::WeirdConvolutionReverb::modeName(verbsuite::WeirdConvolutionReverb::modeFromIndex(i)));
    }

    modeBox_.addItemList(modeNames, 1);
    breathSyncBox_.addItemList({ "Free", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/4D" }, 1);
    cvModeBox_.addItemList({ "Off", "Audio-rate" }, 1);
    cvSmoothingBox_.addItemList({ "Raw", "Envelope" }, 1);
    cvFilterTimeBox_.addItemList({ "Fast", "Medium", "Slow" }, 1);
    freezeModeBox_.addItemList({ "Latch", "Momentary" }, 1);

    for (int i = 0; i < processor_.getNumPrograms(); ++i) {
        presetBox_.addItem(processor_.getProgramName(i), i + 1);
    }
    presetBox_.onChange = [this] {
        const int idx = presetBox_.getSelectedId() - 1;
        if (idx >= 0 && idx != processor_.getCurrentProgram()) {
            processor_.setCurrentProgram(idx);
        }
    };

    title_.setText("Very Weird Convolution", juce::dontSendNotification);
    title_.setColour(juce::Label::textColourId, juce::Colour(0xfff1e0cd));
    title_.setJustificationType(juce::Justification::centredLeft);
    title_.setFont(juce::Font(juce::FontOptions(24.0f, juce::Font::plain)));
    addAndMakeVisible(title_);

    presetLabel_.setText("Preset", juce::dontSendNotification);
    modeLabel_.setText("Mode", juce::dontSendNotification);
    breathSyncLabel_.setText("Breath Sync", juce::dontSendNotification);
    cvModeLabel_.setText("Stability CV", juce::dontSendNotification);
    cvSmoothingLabel_.setText("CV Smoothing", juce::dontSendNotification);
    cvFilterTimeLabel_.setText("CV Filter Time", juce::dontSendNotification);
    freezeModeLabel_.setText("Freeze Mode", juce::dontSendNotification);
    stabilityLabel_.setText("Stability", juce::dontSendNotification);
    breathRateLabel_.setText("Breath Rate", juce::dontSendNotification);
    breathDepthLabel_.setText("Breath Depth", juce::dontSendNotification);
    cvAmountLabel_.setText("CV Amount", juce::dontSendNotification);
    dryLabel_.setText("Dry", juce::dontSendNotification);
    wetLabel_.setText("Wet", juce::dontSendNotification);
    outputLabel_.setText("Output", juce::dontSendNotification);

    for (auto* l : { &presetLabel_, &modeLabel_, &breathSyncLabel_, &cvModeLabel_, &cvSmoothingLabel_, &cvFilterTimeLabel_,
                     &freezeModeLabel_, &stabilityLabel_, &breathRateLabel_, &breathDepthLabel_, &cvAmountLabel_,
                     &dryLabel_, &wetLabel_, &outputLabel_ }) {
        styleSmallLabel(*l);
        addAndMakeVisible(*l);
    }

    for (auto* c : { &presetBox_, &modeBox_, &breathSyncBox_, &cvModeBox_, &cvSmoothingBox_, &cvFilterTimeBox_, &freezeModeBox_ }) {
        styleCombo(*c);
        addAndMakeVisible(*c);
    }

    styleKnob(stabilitySlider_, "");
    styleKnob(breathRateSlider_, " Hz");
    styleKnob(breathDepthSlider_, "");
    styleKnob(cvAmountSlider_, "");
    styleKnob(drySlider_, "");
    styleKnob(wetSlider_, "");
    styleKnob(outputSlider_, " dB");

    for (auto* s : { &stabilitySlider_, &breathRateSlider_, &breathDepthSlider_, &cvAmountSlider_, &drySlider_, &wetSlider_, &outputSlider_ }) {
        addAndMakeVisible(*s);
    }

    freezeButton_.setButtonText("Freeze");
    hqExportButton_.setButtonText("HQ Export");
    lockCoreButton_.setButtonText("Lock Core");
    lockCoreButton_.setClickingTogglesState(true);
    randomizeButton_.setButtonText("Randomize");

    styleButton(freezeButton_);
    styleButton(hqExportButton_);
    styleButton(lockCoreButton_);
    styleButton(randomizeButton_);

    addAndMakeVisible(freezeButton_);
    addAndMakeVisible(hqExportButton_);
    addAndMakeVisible(lockCoreButton_);
    addAndMakeVisible(randomizeButton_);

    randomizeButton_.onClick = [this] {
        std::mt19937 rng(static_cast<uint32_t>(juce::Time::getMillisecondCounter()));
        std::uniform_real_distribution<float> uni(0.0f, 1.0f);

        const bool lockCore = lockCoreButton_.getToggleState();
        const auto maybeSet = [this, lockCore, &uni, &rng](const char* id, bool isCore = false) {
            if (isCore && lockCore) {
                return;
            }
            setParamValue(processor_.parameters(), id, uni(rng));
        };

        maybeSet(kModeParam, true);
        maybeSet(kStabilityParam, true);
        maybeSet(kBreathSyncParam);
        maybeSet(kBreathRateParam);
        maybeSet(kBreathDepthParam);
        maybeSet(kCvModeParam);
        maybeSet(kCvAmountParam);
        maybeSet(kCvSmoothingParam);
        maybeSet(kCvFilterTimeParam);
        maybeSet(kDryParam, true);
        maybeSet(kWetParam, true);
        maybeSet(kOutputParam, true);
        maybeSet(kFreezeModeParam);
        maybeSet(kOversampleHqParam);
    };

    modeAttachment_ = std::make_unique<ChoiceAttachment>(processor_.parameters(), kModeParam, modeBox_);
    breathSyncAttachment_ = std::make_unique<ChoiceAttachment>(processor_.parameters(), kBreathSyncParam, breathSyncBox_);
    freezeModeAttachment_ = std::make_unique<ChoiceAttachment>(processor_.parameters(), kFreezeModeParam, freezeModeBox_);
    cvModeAttachment_ = std::make_unique<ChoiceAttachment>(processor_.parameters(), kCvModeParam, cvModeBox_);
    cvSmoothingAttachment_ = std::make_unique<ChoiceAttachment>(processor_.parameters(), kCvSmoothingParam, cvSmoothingBox_);
    cvFilterTimeAttachment_ = std::make_unique<ChoiceAttachment>(processor_.parameters(), kCvFilterTimeParam, cvFilterTimeBox_);

    stabilityAttachment_ = std::make_unique<SliderAttachment>(processor_.parameters(), kStabilityParam, stabilitySlider_);
    breathRateAttachment_ = std::make_unique<SliderAttachment>(processor_.parameters(), kBreathRateParam, breathRateSlider_);
    breathDepthAttachment_ = std::make_unique<SliderAttachment>(processor_.parameters(), kBreathDepthParam, breathDepthSlider_);
    cvAmountAttachment_ = std::make_unique<SliderAttachment>(processor_.parameters(), kCvAmountParam, cvAmountSlider_);
    dryAttachment_ = std::make_unique<SliderAttachment>(processor_.parameters(), kDryParam, drySlider_);
    wetAttachment_ = std::make_unique<SliderAttachment>(processor_.parameters(), kWetParam, wetSlider_);
    outputAttachment_ = std::make_unique<SliderAttachment>(processor_.parameters(), kOutputParam, outputSlider_);

    freezeAttachment_ = std::make_unique<ButtonAttachment>(processor_.parameters(), kFreezeParam, freezeButton_);
    hqExportAttachment_ = std::make_unique<ButtonAttachment>(processor_.parameters(), kOversampleHqParam, hqExportButton_);

    presetBox_.setSelectedId(processor_.getCurrentProgram() + 1, juce::dontSendNotification);

    setSize(980, 540);
    startTimerHz(30);
}

void VerbSuiteAudioProcessorEditor::paint(juce::Graphics& g) {
    const juce::Colour bgTop(0xff140f0c);
    const juce::Colour bgBot(0xff0f0b09);
    g.setGradientFill(juce::ColourGradient(bgTop, 0.0f, 0.0f, bgBot, 0.0f, static_cast<float>(getHeight()), false));
    g.fillAll();

    auto bounds = getLocalBounds().toFloat().reduced(14.0f);
    g.setColour(juce::Colour(0xff231c17));
    g.fillRoundedRectangle(bounds, 18.0f);

    g.setColour(juce::Colour(0xff8f7158).withAlpha(0.42f));
    g.drawRoundedRectangle(bounds, 18.0f, 1.0f);

    // Heavy oxidized rust haze.
    auto rustBand = bounds.reduced(8.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff8d3d1f).withAlpha(0.45f), rustBand.getX(), rustBand.getCentreY(), juce::Colour(0xff2f2018).withAlpha(0.0f), rustBand.getRight(), rustBand.getBottom(), false));
    g.fillRoundedRectangle(rustBand, 14.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff5f2e1a).withAlpha(0.30f), rustBand.getCentreX(), rustBand.getY(), juce::Colours::transparentBlack, rustBand.getCentreX(), rustBand.getBottom(), false));
    g.fillRoundedRectangle(rustBand.reduced(10.0f), 12.0f);

    // Grimy brushed streaks.
    g.setColour(juce::Colour(0xffc5b096).withAlpha(0.08f));
    for (int y = 22; y < getHeight() - 20; y += 5) {
        g.drawHorizontalLine(y, 26.0f, static_cast<float>(getWidth() - 70));
    }
    g.setColour(juce::Colour(0xff3a2a1f).withAlpha(0.25f));
    for (int x = 38; x < getWidth() - 80; x += 47) {
        g.drawVerticalLine(x, 30.0f, static_cast<float>(getHeight() - 34));
    }

    const float meterHeight = static_cast<float>(getHeight()) * 0.33f;
    const float meterY = (static_cast<float>(getHeight()) - meterHeight) * 0.5f + 40.0f;
    const auto meterBounds = juce::Rectangle<float>(static_cast<float>(getWidth() - 56), meterY, 16.0f, meterHeight);
    g.setColour(juce::Colour(0xff251d19));
    g.fillRoundedRectangle(meterBounds, 5.0f);
    g.setColour(juce::Colour(0xffb69a80).withAlpha(0.28f));
    g.drawRoundedRectangle(meterBounds, 5.0f, 1.0f);

    const float meter = juce::jlimit(0.0f, 1.0f, processor_.getStabilityCvMeter());
    const float meterMaxFill = 0.82f;
    const float fillH = meterBounds.getHeight() * meter * meterMaxFill;
    auto fillRect = meterBounds.withY(meterBounds.getBottom() - fillH).withHeight(fillH);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffcf9a68), fillRect.getCentreX(), fillRect.getY(), juce::Colour(0xff7f4c2b), fillRect.getCentreX(), fillRect.getBottom(), false));
    g.fillRoundedRectangle(fillRect, 4.0f);

    g.setColour(juce::Colour(0xfff0ddc9).withAlpha(0.84f));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::plain)));
    g.drawFittedText("CV", getWidth() - 64, static_cast<int>(meterY) - 16, 34, 14, juce::Justification::centred, 1);
}

void VerbSuiteAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(30);
    const int cvGutter = 84; // Keep knobs clear of the right-side CV meter.
    area.removeFromRight(cvGutter);

    title_.setBounds(area.removeFromTop(36));
    area.removeFromTop(14);

    auto topRow = area.removeFromTop(62);
    const int topGap = 10;
    const int topCols = 7;
    const int topColW = (topRow.getWidth() - (topGap * (topCols - 1))) / topCols;

    auto addComboColumn = [&](juce::Label& label, juce::ComboBox& box) {
        auto col = topRow.removeFromLeft(topColW);
        label.setBounds(col.removeFromTop(18));
        col.removeFromTop(4);
        box.setBounds(col.removeFromTop(34));
        topRow.removeFromLeft(topGap);
    };

    addComboColumn(presetLabel_, presetBox_);
    addComboColumn(modeLabel_, modeBox_);
    addComboColumn(breathSyncLabel_, breathSyncBox_);
    addComboColumn(cvModeLabel_, cvModeBox_);
    addComboColumn(cvSmoothingLabel_, cvSmoothingBox_);
    addComboColumn(cvFilterTimeLabel_, cvFilterTimeBox_);
    addComboColumn(freezeModeLabel_, freezeModeBox_);

    area.removeFromTop(18);

    auto buttonRow = area.removeFromTop(34);
    buttonRow.translate(0, 10);
    const int btnGap = 12;
    const int btnW = 116;
    freezeButton_.setBounds(buttonRow.removeFromLeft(btnW));
    buttonRow.removeFromLeft(btnGap);
    hqExportButton_.setBounds(buttonRow.removeFromLeft(btnW + 6));
    buttonRow.removeFromLeft(btnGap);
    lockCoreButton_.setBounds(buttonRow.removeFromLeft(btnW + 6));
    buttonRow.removeFromLeft(btnGap);
    randomizeButton_.setBounds(buttonRow.removeFromLeft(btnW + 14));

    area.removeFromTop(18);

    const int knobCols = 7;
    const int knobGap = 12;
    const int knobColW = (area.getWidth() - (knobGap * (knobCols - 1))) / knobCols;
    auto knobRow = area;
    const int knobH = juce::jlimit(150, 210, knobRow.getHeight() - 20);
    auto addKnobColumn = [&](juce::Label& label, juce::Slider& slider) {
        auto col = knobRow.removeFromLeft(knobColW);
        auto labelArea = col.removeFromTop(20);
        labelArea.translate(0, 40);
        label.setBounds(labelArea);
        col.removeFromTop(2);
        slider.setBounds(col.withSizeKeepingCentre(knobColW, knobH));
        knobRow.removeFromLeft(knobGap);
    };

    addKnobColumn(stabilityLabel_, stabilitySlider_);
    addKnobColumn(breathRateLabel_, breathRateSlider_);
    addKnobColumn(breathDepthLabel_, breathDepthSlider_);
    addKnobColumn(cvAmountLabel_, cvAmountSlider_);
    addKnobColumn(dryLabel_, drySlider_);
    addKnobColumn(wetLabel_, wetSlider_);
    addKnobColumn(outputLabel_, outputSlider_);
}

void VerbSuiteAudioProcessorEditor::timerCallback() {
    const int expectedProgramId = processor_.getCurrentProgram() + 1;
    if (presetBox_.getSelectedId() != expectedProgramId) {
        presetBox_.setSelectedId(expectedProgramId, juce::dontSendNotification);
    }
    repaint();
}
