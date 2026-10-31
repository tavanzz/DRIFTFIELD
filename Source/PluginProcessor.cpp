#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static const juce::StringArray kKeyNames   { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static const juce::StringArray kScaleNames { "Major", "Minor", "Dorian", "Lydian", "Pentatonic" };
static const juce::StringArray kMoodNames  { "Dreamy", "Dark", "Bright", "Haunted", "Warm" };
static const juce::StringArray kDelayNames { "1/16", "1/8T", "1/8", "1/8D", "1/4", "1/4D", "1/2" };

APVTS::ParameterLayout DriftfieldProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    int v = 1;

    auto pct = [&] (const juce::String& id, const juce::String& name, float def)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, v }, name, juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float x, int) { return juce::String (juce::roundToInt (x * 100.0f)) + "%"; })));
    };
    auto toggle = [&] (const juce::String& id, const juce::String& name, bool def)
    {
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, v }, name, def));
    };
    auto choice = [&] (const juce::String& id, const juce::String& name, const juce::StringArray& items, int def)
    {
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, v }, name, items, def));
    };

    // Globali
    pct (ids::mix, "Mix", 0.6f);
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::output, v }, "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    toggle (ids::freeze, "Freeze", false);

    // Harmonic
    toggle (ids::harmOn, "Harmonic On", true);
    pct (ids::harmAmount, "Harmonic Amount", 0.5f);
    pct (ids::harmDecay, "Harmonic Decay", 0.5f);
    choice (ids::key, "Key", kKeyNames, 0);
    choice (ids::scale, "Scale", kScaleNames, 0);
    pct (ids::vowel, "Vowel", 0.0f);
    pct (ids::vowelMix, "Vowel Mix", 0.0f);

    // Mood
    toggle (ids::moodOn, "Mood On", true);
    choice (ids::moodType, "Mood", kMoodNames, 0);
    pct (ids::moodDepth, "Mood Depth", 0.5f);

    // Diffusion
    toggle (ids::diffOn, "Diffusion On", true);
    pct (ids::diffAmount, "Diffusion Amount", 0.5f);
    pct (ids::diffSize, "Size", 0.6f);
    pct (ids::diffDecay, "Decay", 0.6f);
    pct (ids::diffDamp, "Damping", 0.4f);

    // Texture
    toggle (ids::texOn, "Texture On", true);
    pct (ids::rain, "Rain", 0.0f);
    pct (ids::lofi, "Lo-Fi", 0.0f);
    pct (ids::grit, "Grit", 0.0f);

    // Delay
    toggle (ids::dlyOn, "Delay On", true);
    choice (ids::dlyTime, "Delay Time", kDelayNames, 4);
    pct (ids::dlyAmount, "Delay Amount", 0.3f);
    pct (ids::dlyFeedback, "Feedback", 0.4f);
    pct (ids::dlyTone, "Tone", 0.6f);
    toggle (ids::pingPong, "Ping-Pong", true);

    return { p.begin(), p.end() };
}

const std::vector<FactoryPreset>& DriftfieldProcessor::factoryPresets()
{
    using namespace ids;
    static const std::vector<FactoryPreset> presets {
        { "Init", {} },
        { "Rain Window",     { { harmAmount, 0.3f }, { moodType, 4 }, { moodDepth, 0.6f }, { diffAmount, 0.5f }, { diffSize, 0.7f },
                               { rain, 0.6f }, { lofi, 0.5f }, { grit, 0.1f }, { dlyAmount, 0.2f } } },
        { "Glass Choir",     { { harmAmount, 0.7f }, { harmDecay, 0.8f }, { vowel, 0.1f }, { vowelMix, 0.6f }, { moodDepth, 0.8f },
                               { diffAmount, 0.7f }, { diffSize, 0.9f }, { diffDecay, 0.8f } } },
        { "Infinite Field",  { { harmAmount, 0.4f }, { moodDepth, 1.0f }, { diffAmount, 0.9f }, { diffSize, 1.0f }, { diffDecay, 0.95f },
                               { diffDamp, 0.3f }, { dlyAmount, 0.15f } } },
        { "Dark Cathedral",  { { scale, 1 }, { moodType, 1 }, { moodDepth, 0.9f }, { diffAmount, 0.8f }, { diffSize, 1.0f },
                               { diffDecay, 0.85f }, { diffDamp, 0.6f } } },
        { "Tape Memories",   { { harmAmount, 0.25f }, { moodType, 4 }, { moodDepth, 0.7f }, { lofi, 0.8f }, { grit, 0.3f },
                               { dlyAmount, 0.35f }, { dlyFeedback, 0.5f }, { dlyTone, 0.3f } } },
        { "Haunted Vowels",  { { harmAmount, 0.5f }, { vowel, 0.6f }, { vowelMix, 0.8f }, { moodType, 3 }, { moodDepth, 0.9f },
                               { diffAmount, 0.6f } } },
        { "Bright Shimmer",  { { scale, 3 }, { harmAmount, 0.4f }, { moodType, 2 }, { moodDepth, 1.0f }, { diffAmount, 0.7f },
                               { diffDecay, 0.8f }, { diffDamp, 0.2f } } },
        { "Ping Pong Dreams",{ { dlyTime, 3 }, { dlyAmount, 0.6f }, { dlyFeedback, 0.6f }, { pingPong, 1 }, { diffAmount, 0.5f },
                               { moodDepth, 0.7f } } },
        { "Gritty Storm",    { { grit, 0.7f }, { rain, 0.8f }, { moodType, 1 }, { moodDepth, 0.6f }, { diffAmount, 0.6f },
                               { diffDecay, 0.7f } } },
        { "Pentatonic Pads", { { scale, 4 }, { harmAmount, 0.8f }, { harmDecay, 0.9f }, { moodDepth, 0.8f }, { diffAmount, 0.7f },
                               { diffSize, 0.9f }, { diffDecay, 0.85f }, { dlyOn, 0 } } },
        { "Lo-fi Sine",      { { harmAmount, 0.45f }, { moodType, 4 }, { moodDepth, 0.8f }, { rain, 0.45f }, { lofi, 0.6f },
                               { diffAmount, 0.55f }, { dlyAmount, 0.25f }, { dlyTime, 5 } } },
    };
    return presets;
}

DriftfieldProcessor::DriftfieldProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

bool DriftfieldProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void DriftfieldProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    monoScratch.assign ((size_t) std::max (1, samplesPerBlock) * 2, 0.0f);
    engine.prepare (sampleRate);
}

void DriftfieldProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = buffer.getNumChannels();
    const int n = buffer.getNumSamples();
    if (numCh == 0 || n == 0) return;

    dfx::Params p;
    p.mix = get (ids::mix);             p.outputDb = get (ids::output);       p.freeze = get (ids::freeze) > 0.5f;
    p.harmOn = get (ids::harmOn) > 0.5f; p.harmAmount = get (ids::harmAmount); p.harmDecay = get (ids::harmDecay);
    p.key = (int) get (ids::key);       p.scale = (int) get (ids::scale);
    p.vowel = get (ids::vowel);         p.vowelMix = get (ids::vowelMix);
    p.moodOn = get (ids::moodOn) > 0.5f; p.moodType = (int) get (ids::moodType); p.moodDepth = get (ids::moodDepth);
    p.diffOn = get (ids::diffOn) > 0.5f; p.diffAmount = get (ids::diffAmount);
    p.diffSize = get (ids::diffSize);   p.diffDecay = get (ids::diffDecay);   p.diffDamp = get (ids::diffDamp);
    p.texOn = get (ids::texOn) > 0.5f;  p.rain = get (ids::rain); p.lofi = get (ids::lofi); p.grit = get (ids::grit);
    p.dlyOn = get (ids::dlyOn) > 0.5f;  p.dlyTime = (int) get (ids::dlyTime); p.dlyAmount = get (ids::dlyAmount);
    p.dlyFeedback = get (ids::dlyFeedback); p.dlyTone = get (ids::dlyTone); p.pingPong = get (ids::pingPong) > 0.5f;

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;

    for (int ch = getTotalNumInputChannels(); ch < numCh; ++ch)
        buffer.clear (ch, 0, n);

    float* L = buffer.getWritePointer (0);
    if (numCh >= 2)
    {
        engine.process (L, buffer.getWritePointer (1), n, p, bpm);
    }
    else
    {
        // mono: elabora in stereo e somma
        monoScratch.resize ((size_t) n);
        std::copy (L, L + n, monoScratch.begin());
        engine.process (L, monoScratch.data(), n, p, bpm);
        for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + monoScratch[(size_t) i]);
    }
}

int DriftfieldProcessor::getNumPrograms() { return (int) factoryPresets().size(); }

const juce::String DriftfieldProcessor::getProgramName (int index)
{
    const auto& ps = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) ps.size()) ? ps[(size_t) index].name : juce::String();
}

void DriftfieldProcessor::setCurrentProgram (int index)
{
    const auto& ps = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) ps.size())) return;
    currentPreset = index;

    // tutto ai valori di default (tranne la tonalita'), poi applica il preset
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (rp->getParameterID() != ids::key)
                rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : ps[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}

void DriftfieldProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DriftfieldProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentPreset = (int) tree.getProperty ("preset", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* DriftfieldProcessor::createEditor()
{
    return new DriftfieldEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DriftfieldProcessor();
}
