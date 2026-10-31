#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg1     { 0xff0b0d1f };
    const juce::Colour kBg2     { 0xff1a0f2e };
    const juce::Colour kText    { 0xffeef0ff };
    const juce::Colour kTextDim { 0xff9aa0c8 };
    const juce::Colour kAccent  { 0xffff4fa3 };

    juce::Font font (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
    }
}

// ==========================================================================
DFLookAndFeel::DFLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colours::black.withAlpha (0.35f));
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha (0.18f));
    setColour (juce::ComboBox::arrowColourId, kText);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff161833));
    setColour (juce::PopupMenu::textColourId, kText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kAccent.withAlpha (0.6f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::Label::textColourId, kText);
    setColour (juce::TextButton::textColourOffId, kText);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xee161833));
    setColour (juce::BubbleComponent::outlineColourId, juce::Colours::white.withAlpha (0.2f));
    setColour (juce::TooltipWindow::textColourId, kText);
}

void DFLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                      float startAngle, float endAngle, juce::Slider& s)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float track = juce::jmax (3.0f, radius * 0.14f);
    const float arcR = radius - track * 0.5f;

    // corpo della manopola
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (centre.x - radius * 0.72f, centre.y - radius * 0.72f, radius * 1.44f, radius * 1.44f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (centre.x - radius * 0.72f, centre.y - radius * 0.72f, radius * 1.44f, radius * 1.44f, 1.0f);

    // traccia
    juce::Path bg;
    bg.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.strokePath (bg, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // valore
    juce::Path val;
    val.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
    const auto c = s.findColour (juce::Slider::rotarySliderFillColourId);
    g.setColour (c.withAlpha (0.35f));
    g.strokePath (val, juce::PathStrokeType (track * 2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c);
    g.strokePath (val, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // indicatore
    const float r1 = radius * 0.25f, r2 = radius * 0.62f;
    const juce::Point<float> p1 (centre.x + r1 * std::sin (angle), centre.y - r1 * std::cos (angle));
    const juce::Point<float> p2 (centre.x + r2 * std::sin (angle), centre.y - r2 * std::cos (angle));
    g.setColour (juce::Colours::white);
    g.drawLine ({ p1, p2 }, 2.2f);
}

void DFLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat();

    if (b.getComponentID() == "power")
    {
        const float d = juce::jmin (r.getWidth(), r.getHeight()) - 4.0f;
        auto c = juce::Rectangle<float> (d, d).withCentre (r.getCentre());
        g.setColour (on ? juce::Colours::white : juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (c);
        g.setColour (juce::Colours::white.withAlpha (highlighted ? 0.9f : 0.6f));
        g.drawEllipse (c, 1.2f);
        // simbolo di accensione
        juce::Path p;
        const auto cc = c.getCentre();
        const float rr = d * 0.26f;
        p.addCentredArc (cc.x, cc.y, rr, rr, 0.0f, 0.7f, juce::MathConstants<float>::twoPi - 0.7f, true);
        p.startNewSubPath (cc.x, cc.y - rr * 1.25f);
        p.lineTo (cc.x, cc.y - rr * 0.2f);
        g.setColour (on ? juce::Colour (0xff1a1a2e) : juce::Colours::white.withAlpha (0.8f));
        g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }

    // interruttore a pillola + testo
    auto sw = r.removeFromLeft (34.0f).withSizeKeepingCentre (30.0f, 16.0f);
    g.setColour (on ? juce::Colours::white.withAlpha (0.9f) : juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (sw, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawRoundedRectangle (sw, 8.0f, 1.0f);
    const float knobX = on ? sw.getRight() - 14.0f : sw.getX() + 2.0f;
    g.setColour (on ? juce::Colour (0xff1a1a2e) : juce::Colours::white.withAlpha (0.8f));
    g.fillEllipse (knobX, sw.getY() + 2.0f, 12.0f, 12.0f);
    g.setColour (kText);
    g.setFont (font (12.0f, true));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (4.0f), juce::Justification::centredLeft);
}

void DFLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    if (on)
    {
        juce::ColourGradient grad (juce::Colour (0xff7a5cff), r.getX(), r.getY(), kAccent, r.getRight(), r.getBottom(), false);
        g.setGradientFill (grad);
    }
    else
        g.setColour (juce::Colours::black.withAlpha (down ? 0.55f : 0.35f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (highlighted ? 0.45f : 0.2f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

void DFLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<int> (w, h).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha (0.8f));
    g.fillPath (arrow);
}

juce::Font DFLookAndFeel::getComboBoxFont (juce::ComboBox&) { return font (13.0f, true); }
juce::Font DFLookAndFeel::getPopupMenuFont() { return font (14.0f); }

// ==========================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::white);
    slider.setPopupDisplayEnabled (true, true, nullptr, 800);
    slider.setDoubleClickReturnValue (true, (double) s.getParameter (paramId)->convertFrom0to1 (s.getParameter (paramId)->getDefaultValue()));
    addAndMakeVisible (slider);

    label.setText (text.toUpperCase(), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (font (11.0f, true));
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, paramId, slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromBottom (16));
    slider.setBounds (r);
}

Choice::Choice (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& text)
{
    if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (paramId)))
        box.addItemList (cp->choices, 1);
    box.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (box);

    label.setText (text.toUpperCase(), juce::dontSendNotification);
    label.setFont (font (10.5f, true));
    label.setColour (juce::Label::textColourId, kText.withAlpha (0.75f));
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, paramId, box);
}

void Choice::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromLeft (52));
    box.setBounds (r.reduced (0, 2));
}

// ==========================================================================
ModuleCard::ModuleCard (juce::AudioProcessorValueTreeState& s, const juce::String& t, const juce::String& sub,
                        const juce::String& powerParamId, juce::Colour top, juce::Colour bottom)
    : state (s), title (t), subtitle (sub), cTop (top), cBottom (bottom)
{
    power.setComponentID ("power");
    power.setTooltip ("Accendi / spegni il modulo");
    addAndMakeVisible (power);
    powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, powerParamId, power);
    lastOn = power.getToggleState();
    startTimerHz (15);
}

void ModuleCard::addKnob (const juce::String& id, const juce::String& text)
{
    knobs.push_back (std::make_unique<Knob> (state, id, text));
    addAndMakeVisible (*knobs.back());
}

void ModuleCard::addChoice (const juce::String& id, const juce::String& text)
{
    choices.push_back (std::make_unique<Choice> (state, id, text));
    addAndMakeVisible (*choices.back());
}

void ModuleCard::addSwitch (const juce::String& id, const juce::String& text)
{
    switches.push_back (std::make_unique<juce::ToggleButton> (text));
    addAndMakeVisible (*switches.back());
    switchAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, id, *switches.back()));
}

void ModuleCard::timerCallback()
{
    if (power.getToggleState() != lastOn)
    {
        lastOn = power.getToggleState();
        repaint();
    }
}

void ModuleCard::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);

    // ombra
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (r.translated (0.0f, 4.0f), 14.0f);

    juce::ColourGradient grad (cTop, r.getX(), r.getY(), cBottom, r.getRight(), r.getBottom(), false);
    grad.addColour (0.55, cTop.interpolatedWith (cBottom, 0.5f).brighter (0.1f));
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 14.0f);

    // riflesso diagonale
    juce::Path sheen;
    sheen.startNewSubPath (r.getX(), r.getY() + r.getHeight() * 0.35f);
    sheen.lineTo (r.getRight(), r.getY() + r.getHeight() * 0.12f);
    sheen.lineTo (r.getRight(), r.getY());
    sheen.lineTo (r.getX(), r.getY());
    sheen.closeSubPath();
    g.saveState();
    juce::Path clip; clip.addRoundedRectangle (r, 14.0f);
    g.reduceClipRegion (clip);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillPath (sheen);
    g.restoreState();

    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawRoundedRectangle (r, 14.0f, 1.0f);

    auto header = r.reduced (14.0f, 10.0f).removeFromTop (40.0f);
    g.setColour (kText);
    g.setFont (font (17.0f, true));
    g.drawText (title.toUpperCase(), header.removeFromTop (22.0f), juce::Justification::centredLeft);
    g.setColour (kText.withAlpha (0.7f));
    g.setFont (font (11.0f));
    g.drawText (subtitle, header, juce::Justification::centredLeft);
}

void ModuleCard::paintOverChildren (juce::Graphics& g)
{
    if (! power.getToggleState())
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        r.removeFromTop (54.0f);
        g.setColour (juce::Colour (0xff0b0d1f).withAlpha (0.55f));
        g.fillRoundedRectangle (r, 14.0f);
    }
}

void ModuleCard::resized()
{
    auto r = getLocalBounds().reduced (12, 10);
    auto header = r.removeFromTop (44);
    power.setBounds (header.removeFromRight (30).withSizeKeepingCentre (28, 28));
    r.removeFromTop (6);

    for (auto& c : choices)
    {
        c->setBounds (r.removeFromTop (30));
        r.removeFromTop (6);
    }
    for (auto& s : switches)
    {
        s->setBounds (r.removeFromTop (26));
        r.removeFromTop (6);
    }
    r.removeFromTop (4);

    if (knobs.empty()) return;

    if (knobs.size() == 1)
    {
        knobs[0]->setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 130), juce::jmin (r.getHeight(), 146)));
        return;
    }

    const int cols = 2;
    const int rows = ((int) knobs.size() + 1) / 2;
    const int cellW = r.getWidth() / cols;
    const int cellH = juce::jmin (r.getHeight() / rows, 140);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int row = (int) i / cols, col = (int) i % cols;
        juce::Rectangle<int> cell (r.getX() + col * cellW, r.getY() + row * cellH, cellW, cellH);
        // l'ultima manopola dispari va al centro
        if (i == knobs.size() - 1 && knobs.size() % 2 == 1)
            cell = juce::Rectangle<int> (r.getX(), r.getY() + row * cellH, r.getWidth(), cellH);
        knobs[i]->setBounds (cell.withSizeKeepingCentre (juce::jmin (cellW, 112), cellH).reduced (2));
    }
}

// ==========================================================================
DriftfieldEditor::DriftfieldEditor (DriftfieldProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;

    harmonic  = std::make_unique<ModuleCard> (s, "Harmonic", "Risonanze nella tua tonalita'", ids::harmOn,
                                              juce::Colour (0xff59607a), juce::Colour (0xff23263a));
    mood      = std::make_unique<ModuleCard> (s, "Mood", "Carattere, chorus e shimmer", ids::moodOn,
                                              juce::Colour (0xff3d7bff), juce::Colour (0xff1b2f8f));
    diffusion = std::make_unique<ModuleCard> (s, "Diffusion", "Spazio infinito", ids::diffOn,
                                              juce::Colour (0xffb04dff), juce::Colour (0xff4b1a8a));
    texture   = std::make_unique<ModuleCard> (s, "Texture", "Pioggia, nastro e grana", ids::texOn,
                                              juce::Colour (0xffff4f9a), juce::Colour (0xff8a1a4b));
    delay     = std::make_unique<ModuleCard> (s, "Delay", "Echi a tempo", ids::dlyOn,
                                              juce::Colour (0xffff8a4a), juce::Colour (0xff8a3a1a));

    harmonic->addChoice (ids::key, "Key");
    harmonic->addChoice (ids::scale, "Scale");
    harmonic->addKnob (ids::harmAmount, "Amount");
    harmonic->addKnob (ids::harmDecay, "Ring");
    harmonic->addKnob (ids::vowel, "Vowel");
    harmonic->addKnob (ids::vowelMix, "Voice");

    mood->addChoice (ids::moodType, "Mood");
    mood->addKnob (ids::moodDepth, "Depth");

    diffusion->addKnob (ids::diffAmount, "Amount");
    diffusion->addKnob (ids::diffSize, "Size");
    diffusion->addKnob (ids::diffDecay, "Decay");
    diffusion->addKnob (ids::diffDamp, "Damping");

    texture->addKnob (ids::rain, "Rain");
    texture->addKnob (ids::lofi, "Lo-Fi");
    texture->addKnob (ids::grit, "Grit");

    delay->addChoice (ids::dlyTime, "Time");
    delay->addSwitch (ids::pingPong, "Ping-Pong");
    delay->addKnob (ids::dlyAmount, "Amount");
    delay->addKnob (ids::dlyFeedback, "Feedback");
    delay->addKnob (ids::dlyTone, "Tone");

    for (auto* c : { harmonic.get(), mood.get(), diffusion.get(), texture.get(), delay.get() })
        addAndMakeVisible (*c);

    // header
    const auto& presets = DriftfieldProcessor::factoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        presetBox.addItem (presets[(size_t) i].name, i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { loadPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    prevButton.onClick = [this] { loadPreset ((proc.getCurrentProgram() - 1 + proc.getNumPrograms()) % proc.getNumPrograms()); };
    nextButton.onClick = [this] { loadPreset ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    freezeButton.setClickingTogglesState (true);
    freezeButton.setTooltip ("Congela la coda del riverbero all'infinito");
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, ids::freeze, freezeButton);
    addAndMakeVisible (freezeButton);

    mixKnob = std::make_unique<Knob> (s, ids::mix, "Mix");
    outKnob = std::make_unique<Knob> (s, ids::output, "Output");
    mixKnob->slider.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    outKnob->slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff7a5cff));
    addAndMakeVisible (*mixKnob);
    addAndMakeVisible (*outKnob);

    setResizable (true, true);
    setResizeLimits (900, 520, 1600, 900);
    setSize (1060, 600);
}

DriftfieldEditor::~DriftfieldEditor()
{
    setLookAndFeel (nullptr);
}

void DriftfieldEditor::loadPreset (int index)
{
    proc.setCurrentProgram (index);
    presetBox.setSelectedId (index + 1, juce::dontSendNotification);
}

void DriftfieldEditor::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    juce::ColourGradient bg (kBg1, 0.0f, 0.0f, kBg2, r.getRight(), r.getBottom(), false);
    g.setGradientFill (bg);
    g.fillAll();

    // stelle
    juce::Random rnd (7);
    for (int i = 0; i < 90; ++i)
    {
        const float x = rnd.nextFloat() * r.getWidth(), y = rnd.nextFloat() * r.getHeight();
        g.setColour (juce::Colours::white.withAlpha (0.05f + 0.25f * rnd.nextFloat()));
        const float d = 0.8f + 1.4f * rnd.nextFloat();
        g.fillEllipse (x, y, d, d);
    }

    // titolo con gradiente
    auto titleArea = juce::Rectangle<float> (24.0f, 14.0f, 340.0f, 44.0f);
    juce::GlyphArrangement ga;
    ga.addLineOfText (font (34.0f, true), "DRIFTFIELD", titleArea.getX(), titleArea.getY() + 34.0f);
    juce::Path textPath;
    ga.createPath (textPath);
    juce::ColourGradient tg (juce::Colour (0xff6fa8ff), titleArea.getX(), 0.0f, kAccent, titleArea.getX() + 230.0f, 0.0f, false);
    tg.addColour (0.5, juce::Colour (0xffb04dff));
    g.setGradientFill (tg);
    g.fillPath (textPath);

    g.setColour (kTextDim);
    g.setFont (font (12.0f));
    g.drawText ("texture machine  -  v" JucePlugin_VersionString, 26, 58, 300, 16, juce::Justification::centredLeft);
}

void DriftfieldEditor::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (86).reduced (20, 14);

    outKnob->setBounds (header.removeFromRight (64));
    header.removeFromRight (6);
    mixKnob->setBounds (header.removeFromRight (64));
    header.removeFromRight (18);
    freezeButton.setBounds (header.removeFromRight (96).withSizeKeepingCentre (96, 34));
    header.removeFromRight (18);
    auto presetArea = header.removeFromRight (300).withSizeKeepingCentre (300, 34);
    nextButton.setBounds (presetArea.removeFromRight (34));
    presetArea.removeFromRight (4);
    prevButton.setBounds (presetArea.removeFromLeft (34));
    presetArea.removeFromLeft (4);
    presetBox.setBounds (presetArea);

    auto cards = r.reduced (20, 0).withTrimmedBottom (20);
    const int gap = 12;
    const int w = (cards.getWidth() - gap * 4) / 5;
    for (auto* c : { harmonic.get(), mood.get(), diffusion.get(), texture.get(), delay.get() })
    {
        c->setBounds (cards.removeFromLeft (w));
        cards.removeFromLeft (gap);
    }
}
