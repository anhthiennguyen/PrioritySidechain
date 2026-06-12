#include "PluginProcessor.h"
#include "PluginEditor.h"

PrioritySidechainAudioProcessorEditor::PrioritySidechainAudioProcessorEditor(
    PrioritySidechainAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      thresholdAttach(p.apvts, "threshold", thresholdSlider),
      attackAttach   (p.apvts, "attack",    attackSlider),
      releaseAttach  (p.apvts, "release",   releaseSlider),
      depthAttach    (p.apvts, "depth",     depthSlider),
      scMeter(p.scLevelDb,       juce::Colours::limegreen),
      grMeter(p.gainReductionDb, juce::Colours::orangered)
{
    setupKnob(thresholdSlider, thresholdLabel, "Threshold");
    setupKnob(attackSlider,    attackLabel,    "Attack");
    setupKnob(releaseSlider,   releaseLabel,   "Release");
    setupKnob(depthSlider,     depthLabel,     "Depth");

    for (auto [label, text] : std::initializer_list<std::pair<juce::Label*, const char*>>{
             {&scMeterLabel, "SC"}, {&grMeterLabel, "GR"}})
    {
        label->setText(text, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::FontOptions(11.0f));
        label->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(label);
    }

    addAndMakeVisible(scMeter);
    addAndMakeVisible(grMeter);

    setSize(440, 260);
}

PrioritySidechainAudioProcessorEditor::~PrioritySidechainAudioProcessorEditor() {}

void PrioritySidechainAudioProcessorEditor::setupKnob(juce::Slider& s,
                                                        juce::Label& l,
                                                        const juce::String& text)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff89b4fa));
    addAndMakeVisible(s);

    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::FontOptions(12.0f));
    l.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(l);
}

void PrioritySidechainAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e2e));

    g.setColour(juce::Colour(0xff45475a));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(4.0f), 8.0f, 1.5f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.drawText("Priority Sidechain", getLocalBounds().removeFromTop(30),
               juce::Justification::centred);
}

void PrioritySidechainAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(30); // title

    // Meter strip on the right (two narrow columns: SC | GR)
    auto meterStrip = area.removeFromRight(70);
    auto meterLabelRow = meterStrip.removeFromBottom(18);
    scMeterLabel.setBounds(meterLabelRow.removeFromLeft(35));
    grMeterLabel.setBounds(meterLabelRow);

    auto scCol = meterStrip.removeFromLeft(35).reduced(6, 0);
    auto grCol = meterStrip.reduced(6, 0);
    scMeter.setBounds(scCol);
    grMeter.setBounds(grCol);

    // 4 knobs filling the remaining area
    const int knobW = area.getWidth() / 4;
    for (auto [knob, label] : std::initializer_list<std::pair<juce::Slider*, juce::Label*>>{
             {&thresholdSlider, &thresholdLabel},
             {&attackSlider,    &attackLabel},
             {&releaseSlider,   &releaseLabel},
             {&depthSlider,     &depthLabel}})
    {
        auto col = area.removeFromLeft(knobW);
        label->setBounds(col.removeFromBottom(20));
        knob->setBounds(col);
    }
}
