#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout
PrioritySidechainAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "attack", "Attack",
        juce::NormalisableRange<float>(0.1f, 500.0f, 0.1f, 0.4f), 10.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release",
        juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f, 0.4f), 200.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "depth", "Depth",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    return { params.begin(), params.end() };
}

PrioritySidechainAudioProcessor::BusesProperties
PrioritySidechainAudioProcessor::getBusesProperties()
{
    return BusesProperties()
        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)
        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true);
}

PrioritySidechainAudioProcessor::PrioritySidechainAudioProcessor()
    : AudioProcessor(getBusesProperties()),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

PrioritySidechainAudioProcessor::~PrioritySidechainAudioProcessor() {}

void PrioritySidechainAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    envelopeLevel = 0.0f;
}

void PrioritySidechainAudioProcessor::releaseResources() {}

bool PrioritySidechainAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void PrioritySidechainAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto mainInput  = getBusBuffer(buffer, true,  0);
    auto sidechain  = getBusBuffer(buffer, true,  1);
    auto mainOutput = getBusBuffer(buffer, false, 0);

    const float threshDb  = *apvts.getRawParameterValue("threshold");
    const float attackMs  = *apvts.getRawParameterValue("attack");
    const float releaseMs = *apvts.getRawParameterValue("release");
    const float depthDb   = *apvts.getRawParameterValue("depth");

    const float sr           = float(currentSampleRate);
    const float attackCoeff  = std::exp(-1.0f / (sr * attackMs  * 0.001f));
    const float releaseCoeff = std::exp(-1.0f / (sr * releaseMs * 0.001f));
    const float threshLin    = juce::Decibels::decibelsToGain(threshDb);
    const float depthLin     = juce::Decibels::decibelsToGain(depthDb);   // e.g. 0.1 for -20 dB

    const int numSamples    = buffer.getNumSamples();
    const int scChannels    = sidechain.getNumChannels();
    const int outChannels   = mainOutput.getNumChannels();
    const int inChannels    = mainInput.getNumChannels();

    float blockPeakGR = 0.0f; // track worst-case GR this block for the meter

    for (int s = 0; s < numSamples; ++s)
    {
        // Peak level from sidechain bus
        float scPeak = 0.0f;
        for (int ch = 0; ch < scChannels; ++ch)
            scPeak = std::max(scPeak, std::abs(sidechain.getSample(ch, s)));

        // Ballistic envelope follower
        const float coeff = scPeak > envelopeLevel ? attackCoeff : releaseCoeff;
        envelopeLevel = coeff * envelopeLevel + (1.0f - coeff) * scPeak;

        // Gain reduction: when envelope > threshold, duck toward depthLin
        float gain = 1.0f;
        if (envelopeLevel > threshLin)
        {
            // Normalised excess [0, 1]
            float excess = (envelopeLevel - threshLin) / (1.0f - threshLin + 1e-9f);
            excess = juce::jlimit(0.0f, 1.0f, excess);
            gain = 1.0f - excess * (1.0f - depthLin);
        }

        blockPeakGR = std::min(blockPeakGR, gain); // lower = more reduction

        for (int ch = 0; ch < outChannels; ++ch)
        {
            const int srcCh = ch < inChannels ? ch : 0;
            mainOutput.setSample(ch, s, mainInput.getSample(srcCh, s) * gain);
        }
    }

    // Update meters (dB, negative means reduction)
    scLevelDb.store(juce::Decibels::gainToDecibels(envelopeLevel, -100.0f));
    gainReductionDb.store(juce::Decibels::gainToDecibels(blockPeakGR, -60.0f));
}

void PrioritySidechainAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void PrioritySidechainAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* PrioritySidechainAudioProcessor::createEditor()
{
    return new PrioritySidechainAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PrioritySidechainAudioProcessor();
}
