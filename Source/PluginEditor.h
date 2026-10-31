#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// --------------------------------------------------------------------------
class DFLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DFLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

// Manopola con etichetta
struct Knob : public juce::Component
{
    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& text);
    void resized() override;

    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Menu a tendina con etichetta
struct Choice : public juce::Component
{
    Choice (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& text);
    void resized() override;

    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

// Una "carta" colorata per ciascun modulo
class ModuleCard : public juce::Component, private juce::Timer
{
public:
    ModuleCard (juce::AudioProcessorValueTreeState& s, const juce::String& title, const juce::String& subtitle,
                const juce::String& powerParamId, juce::Colour top, juce::Colour bottom);

    void addKnob (const juce::String& paramId, const juce::String& text);
    void addChoice (const juce::String& paramId, const juce::String& text);
    void addSwitch (const juce::String& paramId, const juce::String& text);

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& state;
    juce::String title, subtitle;
    juce::Colour cTop, cBottom;
    juce::ToggleButton power;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Choice>> choices;
    std::vector<std::unique_ptr<juce::ToggleButton>> switches;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> switchAttachments;
    bool lastOn = true;
};

// --------------------------------------------------------------------------
class DriftfieldEditor : public juce::AudioProcessorEditor
{
public:
    explicit DriftfieldEditor (DriftfieldProcessor&);
    ~DriftfieldEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void loadPreset (int index);

    DriftfieldProcessor& proc;
    DFLookAndFeel lnf;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::TextButton freezeButton { "FREEZE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;
    std::unique_ptr<Knob> mixKnob, outKnob;

    std::unique_ptr<ModuleCard> harmonic, mood, diffusion, texture, delay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DriftfieldEditor)
};
