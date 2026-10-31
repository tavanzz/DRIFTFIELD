// Render offline: suona note "a caso" con una sinusoide e le passa nel motore.
// Uso: dsp_render <cartella_output>
#include "DSP.h"
#include <cstdio>
#include <string>
#include <fstream>

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    std::ofstream f (path, std::ios::binary);
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4;
    auto w32 = [&] (uint32_t v) { f.write ((const char*) &v, 4); };
    auto w16 = [&] (uint16_t v) { f.write ((const char*) &v, 2); };
    f.write ("RIFF", 4); w32 (36 + dataBytes); f.write ("WAVE", 4);
    f.write ("fmt ", 4); w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    f.write ("data", 4); w32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) std::lround (dfx::clampf (L[i], -1, 1) * 32767);
        int16_t b = (int16_t) std::lround (dfx::clampf (R[i], -1, 1) * 32767);
        f.write ((const char*) &a, 2); f.write ((const char*) &b, 2);
    }
}

int main (int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    const int sr = 48000, secs = 12, n = sr * secs, block = 256;

    // input: note casuali (sinusoide) per 6 s, poi silenzio per sentire le code
    std::vector<float> inL (n, 0.0f);
    dfx::Rng rng (42);
    const int notes[] = { 60, 63, 67, 70, 62, 65, 72, 61, 66, 69 };
    float phase = 0.0f;
    int noteIdx = 0;
    for (int i = 0; i < sr * 6; ++i)
    {
        const int noteLen = sr / 2;
        const int pos = i % noteLen;
        if (pos == 0) noteIdx = (int) (rng.next() * 10) % 10;
        const float env = std::min (1.0f, pos / 200.0f) * std::exp (-3.0f * pos / (float) noteLen);
        phase += dfx::midiToHz ((float) notes[noteIdx]) / sr;
        if (phase >= 1.0f) phase -= 1.0f;
        inL[(size_t) i] = 0.5f * env * std::sin (dfx::kTwoPi * phase);
    }

    struct Case { const char* name; dfx::Params p; };
    std::vector<Case> cases;

    { dfx::Params p; cases.push_back ({ "init", p }); }
    { dfx::Params p; p.harmAmount = 1; p.harmDecay = 0.9f; p.diffOn = false; p.dlyOn = false; p.moodOn = false; p.mix = 1; cases.push_back ({ "harmonic_only", p }); }
    { dfx::Params p; p.vowel = 0.5f; p.vowelMix = 1; p.harmAmount = 0.6f; cases.push_back ({ "vowel", p }); }
    { dfx::Params p; p.moodType = 0; p.moodDepth = 1; p.diffAmount = 0.9f; p.diffSize = 1; p.diffDecay = 0.95f; cases.push_back ({ "dreamy_infinite", p }); }
    { dfx::Params p; p.moodType = 1; p.moodDepth = 1; p.diffAmount = 0.8f; p.diffDecay = 0.9f; p.scale = 1; cases.push_back ({ "dark", p }); }
    { dfx::Params p; p.moodType = 2; p.moodDepth = 1; p.diffAmount = 0.8f; p.diffDecay = 1.0f; cases.push_back ({ "bright_max_decay", p }); }
    { dfx::Params p; p.moodType = 4; p.moodDepth = 0.6f; p.rain = 0.7f; p.lofi = 0.5f; p.grit = 0.1f; cases.push_back ({ "lofi_rain", p }); }
    { dfx::Params p; p.grit = 1; p.lofi = 1; p.rain = 1; p.dlyFeedback = 0.95f; p.dlyAmount = 1; cases.push_back ({ "extreme", p }); }
    { dfx::Params p; p.moodOn = false; p.diffAmount = 1; p.diffSize = 1; p.diffDecay = 1; p.diffDamp = 0; cases.push_back ({ "reverb_max_noshimmer", p }); }
    { dfx::Params p; p.moodType = 3; p.moodDepth = 1; p.diffAmount = 1; p.diffSize = 1; p.diffDecay = 1; p.diffDamp = 0; p.harmAmount = 1; p.harmDecay = 1; cases.push_back ({ "haunted_worstcase", p }); }
    { dfx::Params p; p.freeze = true; cases.push_back ({ "freeze_from_start", p }); }

    int failures = 0;
    for (auto& c : cases)
    {
        dfx::Engine e;
        e.prepare (sr);
        std::vector<float> L = inL, R = inL;
        for (int s = 0; s < n; s += block)
        {
            dfx::Params p = c.p;
            // per il caso "freeze_late": nessuno
            e.process (L.data() + s, R.data() + s, std::min (block, n - s), p, 120.0);
        }
        double peak = 0, sum = 0, tailSum = 0; bool bad = false;
        for (int i = 0; i < n; ++i)
        {
            if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) bad = true;
            peak = std::max<double> (peak, std::max (std::abs (L[i]), std::abs (R[i])));
            sum += L[i] * L[i] + R[i] * R[i];
            if (i > sr * 10) tailSum += L[i] * L[i] + R[i] * R[i];
        }
        const double rms = std::sqrt (sum / (2.0 * n));
        const double tailRms = std::sqrt (tailSum / (2.0 * (n - sr * 10)));
        std::printf ("%-20s peak=%.3f rms=%.4f tailRms(10-12s)=%.5f %s\n", c.name, peak, rms, tailRms, bad ? "NaN!" : "");
        if (bad) ++failures;
        writeWav (out + "/" + c.name + ".wav", L, R, sr);
    }

    // freeze attivato a metà (dopo 3 s) e mantenuto
    {
        dfx::Engine e; e.prepare (sr);
        std::vector<float> L = inL, R = inL;
        for (int s = 0; s < n; s += block)
        {
            dfx::Params p; p.diffAmount = 1; p.diffDecay = 0.5f; p.freeze = s > sr * 3;
            e.process (L.data() + s, R.data() + s, std::min (block, n - s), p, 120.0);
        }
        double tail = 0; for (int i = sr * 10; i < n; ++i) tail += L[i] * L[i];
        std::printf ("%-20s tailRms(10-12s)=%.5f (deve restare > 0)\n", "freeze_late", std::sqrt (tail / (n - sr * 10)));
        writeWav (out + "/freeze_late.wav", L, R, sr);
    }
    {
        std::vector<float> R = inL; writeWav (out + "/input_dry.wav", inL, R, sr);
    }
    return failures;
}
