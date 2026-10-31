// Smoke test: istanzia il processor, carica ogni preset, elabora audio,
// crea l'editor e ne salva uno screenshot PNG.  Uso: plugin_smoke <out.png>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    DriftfieldProcessor proc;
    proc.setPlayConfigDetails (2, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    juce::Random rnd (1);
    int failures = 0;

    for (int pr = 0; pr < proc.getNumPrograms(); ++pr)
    {
        proc.setCurrentProgram (pr);
        float peak = 0.0f;
        for (int b = 0; b < 200; ++b)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    buf.setSample (ch, i, b < 60 ? 0.3f * std::sin ((float) (b * 512 + i) * 0.0627f) : 0.0f);
            proc.processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                {
                    const float v = buf.getSample (ch, i);
                    if (! std::isfinite (v)) ++failures;
                    peak = std::max (peak, std::abs (v));
                }
        }
        std::printf ("preset %-18s peak=%.3f\n", proc.getProgramName (pr).toRawUTF8(), peak);
    }

    // stato: salva e ricarica
    juce::MemoryBlock mb;
    proc.getStateInformation (mb);
    proc.setStateInformation (mb.getData(), (int) mb.getSize());
    std::printf ("state size: %d bytes\n", (int) mb.getSize());

    proc.setCurrentProgram (1);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (1060, 600);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "ui.png"));
    out.deleteFile();
    juce::FileOutputStream fos (out);
    juce::PNGImageFormat().writeImageToStream (img, fos);
    std::printf ("screenshot: %s\n", out.getFullPathName().toRawUTF8());
    ed.reset();
    return failures;
}
