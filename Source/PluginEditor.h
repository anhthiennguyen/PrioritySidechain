#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class LevelMeter : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter(std::atomic<float>& source, juce::Colour colour)
        : src(source), barColour(colour)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(bounds, 3.0f);

        // Map dB range [-60, 0] to height fraction
        float db   = juce::jlimit(-60.0f, 0.0f, displayDb);
        float frac = (db + 60.0f) / 60.0f;
        float barH = bounds.getHeight() * frac;

        g.setColour(barColour);
        g.fillRoundedRectangle(bounds.removeFromBottom(barH), 3.0f);
    }

    void timerCallback() override
    {
        float newDb = src.load();
        if (std::abs(newDb - displayDb) > 0.1f)
        {
            displayDb = newDb;
            repaint();
        }
    }

private:
    std::atomic<float>& src;
    juce::Colour barColour;
    float displayDb = -60.0f;
};

class PrioritySidechainAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PrioritySidechainAudioProcessorEditor(PrioritySidechainAudioProcessor&);
    ~PrioritySidechainAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    PrioritySidechainAudioProcessor& processor;

    // Sliders + attachments
    juce::Slider thresholdSlider, attackSlider, releaseSlider, depthSlider;
    juce::Label  thresholdLabel,  attackLabel,  releaseLabel,  depthLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    Attachment thresholdAttach, attackAttach, releaseAttach, depthAttach;

    // Meters
    LevelMeter scMeter;
    LevelMeter grMeter;
    juce::Label scMeterLabel, grMeterLabel;

    void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrioritySidechainAudioProcessorEditor)
};
