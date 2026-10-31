#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP.h"

namespace ids
{
    #define DF_ID(name) static const juce::String name { #name };
    DF_ID (mix) DF_ID (output) DF_ID (freeze)
    DF_ID (harmOn) DF_ID (harmAmount) DF_ID (harmDecay) DF_ID (key) DF_ID (scale) DF_ID (vowel) DF_ID (vowelMix)
    DF_ID (moodOn) DF_ID (moodType) DF_ID (moodDepth)
    DF_ID (diffOn) DF_ID (diffAmount) DF_ID (diffSize) DF_ID (diffDecay) DF_ID (diffDamp)
    DF_ID (texOn) DF_ID (rain) DF_ID (lofi) DF_ID (grit)
    DF_ID (dlyOn) DF_ID (dlyTime) DF_ID (dlyAmount) DF_ID (dlyFeedback) DF_ID (dlyTone) DF_ID (pingPong)
    #undef DF_ID
}

struct FactoryPreset
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> values; // valori "reali" (non normalizzati)
};

class DriftfieldProcessor : public juce::AudioProcessor
{
public:
    DriftfieldProcessor();
    ~DriftfieldProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const std::vector<FactoryPreset>& factoryPresets();

private:
    float get (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }

    dfx::Engine engine;
    std::vector<float> monoScratch;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DriftfieldProcessor)
};
