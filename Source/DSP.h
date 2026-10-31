// Driftfield - motore DSP (C++17 puro, nessuna dipendenza da JUCE)
//
// Catena:  Harmonic -> Mood -> Delay -> Diffusion -> Texture -> Mix/Output
//
//  Harmonic : banco di 8 risonatori "simpatici" accordati sulla tonalita' scelta
//             + filtro a formanti (vocali A-E-I-O-U)
//  Mood     : ensemble/chorus, tilt EQ, calore, e shimmer nel riverbero
//  Delay    : delay stereo sincronizzato al tempo, con ping-pong
//  Diffusion: riverbero FDN a 8 linee, modulato, con "Freeze" (campo infinito)
//  Texture  : pioggia, lo-fi (wow/flutter, sample-rate, bit), grit (saturazione)

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace dfx
{
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

inline float clampf (float v, float lo, float hi) { return std::min (hi, std::max (lo, v)); }
inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }
inline float midiToHz (float note) { return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f); }

// --------------------------------------------------------------------------
struct Rng
{
    uint32_t s = 0x9E3779B9u;
    explicit Rng (uint32_t seed = 0x9E3779B9u) : s (seed ? seed : 1u) {}
    float next() // [0, 1)
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) (s >> 8) * (1.0f / 16777216.0f);
    }
    float bipolar() { return next() * 2.0f - 1.0f; }
};

// Parametro con smoothing esponenziale (evita click)
struct Smoothed
{
    float cur = 0.0f, target = 0.0f, coef = 0.01f;
    void prepare (double sr, float timeSec, float v)
    {
        coef = 1.0f - std::exp (-1.0f / (timeSec * (float) sr));
        cur = target = v;
    }
    void set (float v) { target = v; }
    float next() { cur += (target - cur) * coef; return cur; }
};

struct OnePoleLP
{
    float z = 0.0f, a = 1.0f;
    void setCutoff (float hz, double sr) { a = 1.0f - std::exp (-kTwoPi * hz / (float) sr); }
    float process (float x) { z += a * (x - z); return z; }
    void reset() { z = 0.0f; }
};

struct DCBlocker
{
    float x1 = 0.0f, y1 = 0.0f, r = 0.995f;
    float process (float x) { float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }
    void reset() { x1 = y1 = 0.0f; }
};

// Linea di ritardo con lettura frazionaria (interpolazione lineare)
struct DelayLine
{
    std::vector<float> buf;
    int w = 0;

    void resize (int n) { buf.assign ((size_t) std::max (4, n), 0.0f); w = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    int size() const { return (int) buf.size(); }

    void push (float x)
    {
        buf[(size_t) w] = x;
        if (++w >= size()) w = 0;
    }

    // delay = 1 -> ultimo campione scritto
    float read (float delay) const
    {
        const int n = size();
        delay = clampf (delay, 1.0f, (float) (n - 2));
        float rp = (float) w - delay;
        if (rp < 0.0f) rp += (float) n;
        int i0 = (int) rp;
        const float f = rp - (float) i0;
        if (i0 >= n) i0 -= n;
        int i1 = i0 + 1;
        if (i1 >= n) i1 = 0;
        return buf[(size_t) i0] + f * (buf[(size_t) i1] - buf[(size_t) i0]);
    }
};

// State Variable Filter (topologia TPT, stabile anche se modulato)
struct SVF
{
    float g = 0.0f, k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, ic1 = 0.0f, ic2 = 0.0f;

    void set (float hz, float q, double sr)
    {
        hz = clampf (hz, 10.0f, 0.45f * (float) sr);
        g = std::tan (kPi * hz / (float) sr);
        k = 1.0f / std::max (0.05f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0.0f; }

    void process (float v0, float& lp, float& bp, float& hp)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }
    float lowpass (float x)  { float l, b, h; process (x, l, b, h); return l; }
    float bandpass (float x) { float l, b, h; process (x, l, b, h); return b; }
    float highpass (float x) { float l, b, h; process (x, l, b, h); return h; }
};

struct Allpass
{
    DelayLine d;
    float delay = 100.0f, g = 0.5f;
    void prepare (float delaySamples, float gain)
    {
        delay = delaySamples; g = gain;
        d.resize ((int) delaySamples + 8);
    }
    float process (float x)
    {
        const float v = d.read (delay);
        const float w = x + g * v;
        d.push (w);
        return v - g * w;
    }
};

// Pitch shifter a due testine con finestre incrociate (usato per lo shimmer)
struct PitchShifter
{
    DelayLine d;
    float window = 2400.0f, phase = 0.0f;

    void prepare (double sr)
    {
        window = (float) (0.05 * sr);
        d.resize ((int) (window * 2.0f) + 16);
        phase = 0.0f;
    }

    float process (float x, float ratio)
    {
        d.push (x);
        phase += (1.0f - ratio) / window;
        phase -= std::floor (phase);
        float p2 = phase + 0.5f;
        p2 -= std::floor (p2);
        const float w1 = 0.5f - 0.5f * std::cos (kTwoPi * phase);
        const float w2 = 0.5f - 0.5f * std::cos (kTwoPi * p2);
        return d.read (2.0f + phase * window) * w1 + d.read (2.0f + p2 * window) * w2;
    }
};

// ==========================================================================
//  HARMONIC: risonatori accordati + formanti vocali
// ==========================================================================
struct Resonator
{
    DelayLine d;
    OnePoleLP damp;
    float period = 100.0f, g = 0.0f, lpComp = 0.0f;

    void prepare (double sr, float minHz)
    {
        d.resize ((int) (sr / minHz) + 16);
        damp.setCutoff (6500.0f, sr);
        lpComp = (1.0f - damp.a) / damp.a; // ritardo di gruppo del passa-basso
    }
    void tune (float hz, float t60, double sr)
    {
        period = std::max (2.0f, (float) sr / hz - lpComp);
        g = std::pow (10.0f, -3.0f * ((float) sr / hz) / (t60 * (float) sr));
    }
    float process (float x)
    {
        const float y = x + g * damp.process (d.read (period));
        d.push (y);
        return y;
    }
};

struct Harmonic
{
    static constexpr int kNumRes = 8;
    std::array<Resonator, kNumRes> res;
    std::array<float, kNumRes> panL {}, panR {}, inGain {};
    DCBlocker dcL, dcR;
    double sr = 48000.0;
    int lastKey = -1, lastScale = -1;
    float lastDecay = -1.0f;

    // Note dell'accordo per ciascuna scala: fondamentale, terza, quinta, settima, colore
    static constexpr int kNumScales = 5;
    static constexpr int chordTones[kNumScales][5] = {
        { 0, 4, 7, 11, 14 }, // Major      -> maj9
        { 0, 3, 7, 10, 14 }, // Minor      -> m9
        { 0, 3, 7, 10, 17 }, // Dorian     -> m11
        { 0, 4, 7, 11, 18 }, // Lydian     -> maj7#11
        { 0, 4, 7,  9, 14 }, // Pentatonic -> 6/9
    };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (int i = 0; i < kNumRes; ++i)
        {
            res[(size_t) i].prepare (sr, 40.0f);
            const float p = (i % 2 == 0) ? 0.3f : 0.7f; // alterna sinistra/destra
            panL[(size_t) i] = std::cos (p * kPi * 0.5f);
            panR[(size_t) i] = std::sin (p * kPi * 0.5f);
        }
        lastKey = -1;
    }

    void update (int key, int scale, float decay)
    {
        if (key == lastKey && scale == lastScale && std::abs (decay - lastDecay) < 1.0e-4f)
            return;
        lastKey = key; lastScale = scale; lastDecay = decay;

        const int* ct = chordTones[std::clamp (scale, 0, kNumScales - 1)];
        const float base = 48.0f + (float) key; // ottava 3
        const float notes[kNumRes] = {
            base + ct[0],      base + ct[2],      base + 12 + ct[0], base + 12 + ct[1],
            base + 12 + ct[2], base + 12 + ct[3], base + 12 + ct[4], base + 24 + ct[0]
        };
        const float t60 = 0.3f * std::pow (8.0f / 0.3f, clampf (decay, 0.0f, 1.0f)); // 0.3 .. 8 s

        for (int i = 0; i < kNumRes; ++i)
        {
            auto& r = res[(size_t) i];
            r.tune (midiToHz (notes[i]), t60, sr);
            // guadagno d'ingresso: compensa la risonanza (picco ~ 1/(1-g))
            inGain[(size_t) i] = std::pow (1.0f - r.g, 0.8f) * 1.6f;
        }
    }

    void process (float in, float& outL, float& outR)
    {
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < kNumRes; ++i)
        {
            const float y = res[(size_t) i].process (in * inGain[(size_t) i]);
            l += y * panL[(size_t) i];
            r += y * panR[(size_t) i];
        }
        outL = std::tanh (dcL.process (l) * 0.5f);
        outR = std::tanh (dcR.process (r) * 0.5f);
    }
};

struct FormantFilter
{
    // F1, F2, F3 per A, E, I, O, U
    static constexpr float table[5][3] = {
        { 800.0f, 1150.0f, 2900.0f }, // A
        { 400.0f, 1700.0f, 2600.0f }, // E
        { 300.0f, 2200.0f, 3000.0f }, // I
        { 450.0f,  800.0f, 2830.0f }, // O
        { 325.0f,  700.0f, 2530.0f }, // U
    };
    std::array<SVF, 3> f;

    void set (float vowel, double sr)
    {
        const float pos = clampf (vowel, 0.0f, 1.0f) * 4.0f;
        const int i0 = std::min (3, (int) pos);
        const float t = pos - (float) i0;
        const float qs[3] = { 7.0f, 9.0f, 11.0f };
        for (int k = 0; k < 3; ++k)
        {
            const float hz = table[i0][k] + t * (table[i0 + 1][k] - table[i0][k]);
            f[(size_t) k].set (hz, qs[k], sr);
        }
    }
    float process (float x)
    {
        // k * bandpass = guadagno unitario al centro della formante
        return 1.8f * f[0].k * f[0].bandpass (x) + 1.2f * f[1].k * f[1].bandpass (x) + 0.8f * f[2].k * f[2].bandpass (x);
    }
};

// ==========================================================================
//  MOOD: ensemble + tilt + calore (lo shimmer vive nel riverbero)
// ==========================================================================
struct MoodSettings
{
    float chorusDepth, rateHz, tilt, shimmerSemis, shimmerAmt, warmth;
};

static constexpr int kNumMoods = 5;
static constexpr MoodSettings kMoods[kNumMoods] = {
    // depth  rate   tilt   shimmer  amt   warmth
    { 1.0f, 0.25f,  0.15f,  12.0f, 0.35f, 0.0f }, // Dreamy
    { 0.6f, 0.15f, -0.80f, -12.0f, 0.30f, 0.2f }, // Dark
    { 0.7f, 0.45f,  0.60f,  19.0f, 0.25f, 0.0f }, // Bright
    { 1.0f, 0.90f, -0.30f,  -5.0f, 0.35f, 0.1f }, // Haunted
    { 0.5f, 0.20f, -0.35f,  12.0f, 0.15f, 0.6f }, // Warm
};

struct Ensemble
{
    static constexpr int kVoices = 3;
    DelayLine d;
    std::array<float, kVoices> phase {};
    double sr = 48000.0;

    void prepare (double sampleRate, float phaseOffset)
    {
        sr = sampleRate;
        d.resize ((int) (0.05 * sr));
        for (int v = 0; v < kVoices; ++v)
            phase[(size_t) v] = std::fmod (phaseOffset + (float) v / kVoices, 1.0f);
    }

    float process (float x, float rateHz, float depth)
    {
        d.push (x);
        static constexpr float baseMs[kVoices] = { 11.0f, 16.5f, 23.0f };
        static constexpr float rateMul[kVoices] = { 1.0f, 0.83f, 1.21f };
        float sum = 0.0f;
        for (int v = 0; v < kVoices; ++v)
        {
            auto& p = phase[(size_t) v];
            p += rateHz * rateMul[v] / (float) sr;
            if (p >= 1.0f) p -= 1.0f;
            const float ms = baseMs[v] + depth * 4.0f * std::sin (kTwoPi * p);
            sum += d.read (ms * 0.001f * (float) sr);
        }
        return sum / (float) kVoices;
    }
};

// ==========================================================================
//  DIFFUSION: riverbero FDN 8x8 (Hadamard), modulato, con freeze e shimmer
// ==========================================================================
struct FDNReverb
{
    static constexpr int N = 8;
    std::array<DelayLine, N> lines;
    std::array<OnePoleLP, N> damp;
    std::array<float, N> gains {}, lfo {}, lfoRate {};
    std::array<Allpass, 4> apL, apR;
    PitchShifter shiftL, shiftR;
    OnePoleLP shimmerLPL, shimmerLPR;
    float lastL = 0.0f, lastR = 0.0f, tailEnv = 0.0f;
    double sr = 48000.0;
    Smoothed size;
    float decayT60 = 3.0f, dampHz = 6000.0f;

    static constexpr float baseMs[N] = { 29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.9f, 67.7f, 73.1f };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (int i = 0; i < N; ++i)
        {
            lines[(size_t) i].resize ((int) (0.17 * sr) + 64); // fino a size = 2x
            lfo[(size_t) i] = (float) i / N;
            lfoRate[(size_t) i] = 0.11f + 0.07f * (float) i;
        }
        const float apMsL[4] = { 4.7f, 3.6f, 12.7f, 9.3f };
        const float apMsR[4] = { 5.1f, 3.9f, 11.9f, 8.7f };
        for (int i = 0; i < 4; ++i)
        {
            apL[(size_t) i].prepare (apMsL[i] * 0.001f * (float) sr, i < 2 ? 0.65f : 0.55f);
            apR[(size_t) i].prepare (apMsR[i] * 0.001f * (float) sr, i < 2 ? 0.65f : 0.55f);
        }
        shiftL.prepare (sr);
        shiftR.prepare (sr);
        shimmerLPL.setCutoff (7000.0f, sr);
        shimmerLPR.setCutoff (7000.0f, sr);
        size.prepare (sr, 0.3f, 0.6f);
    }

    void setParams (float sizeNorm, float decayNorm, float dampNorm)
    {
        size.set (0.4f + 1.6f * clampf (sizeNorm, 0.0f, 1.0f));
        decayT60 = 0.4f * std::pow (20.0f / 0.4f, clampf (decayNorm, 0.0f, 1.0f)); // 0.4 .. 20 s
        dampHz = 16000.0f * std::pow (1500.0f / 16000.0f, clampf (dampNorm, 0.0f, 1.0f));
        for (auto& d : damp) d.setCutoff (dampHz, sr);
    }

    void process (float inL, float inR, bool freeze, float shimmerRatio, float shimmerAmt,
                  float& outL, float& outR)
    {
        const float s = size.next();

        // shimmer: una parte della coda viene trasposta e reiniettata
        // con un limitatore automatico: piu' la coda e' forte, meno shimmer viene reiniettato
        const float lev = 0.5f * (lastL * lastL + lastR * lastR);
        tailEnv += (lev > tailEnv ? 0.01f : 0.0005f) * (lev - tailEnv);
        const float shimGain = shimmerAmt * clampf (1.0f - std::sqrt (tailEnv) / 0.12f, 0.0f, 1.0f);
        if (! freeze && shimGain > 0.0001f)
        {
            inL += shimGain * shimmerLPL.process (shiftL.process (lastL, shimmerRatio));
            inR += shimGain * shimmerLPR.process (shiftR.process (lastR, shimmerRatio));
        }
        else
        {
            shiftL.process (lastL, shimmerRatio);
            shiftR.process (lastR, shimmerRatio);
        }

        if (freeze) { inL = 0.0f; inR = 0.0f; }

        for (auto& a : apL) inL = a.process (inL);
        for (auto& a : apR) inR = a.process (inR);

        std::array<float, N> y {};
        const float modDepth = 0.0004f * (float) sr; // ~0.4 ms
        for (int i = 0; i < N; ++i)
        {
            auto& ph = lfo[(size_t) i];
            ph += lfoRate[(size_t) i] / (float) sr;
            if (ph >= 1.0f) ph -= 1.0f;
            const float len = baseMs[i] * 0.001f * (float) sr * s;
            float v = lines[(size_t) i].read (len + modDepth * (1.0f + std::sin (kTwoPi * ph)));
            if (! freeze) v = damp[(size_t) i].process (v);
            y[(size_t) i] = v;
            const float g = freeze ? 1.0f : std::pow (10.0f, -3.0f * (len / (float) sr) / decayT60);
            gains[(size_t) i] = g;
        }

        // matrice di Hadamard (normalizzata)
        std::array<float, N> h = y;
        for (int len = 1; len < N; len <<= 1)
            for (int i = 0; i < N; i += len << 1)
                for (int j = i; j < i + len; ++j)
                {
                    const float a = h[(size_t) j], b = h[(size_t) (j + len)];
                    h[(size_t) j] = a + b;
                    h[(size_t) (j + len)] = a - b;
                }
        const float norm = 1.0f / std::sqrt ((float) N);

        for (int i = 0; i < N; ++i)
        {
            const float in = (i % 2 == 0) ? inL : inR;
            float fb = gains[(size_t) i] * h[(size_t) i] * norm;
            if (freeze) fb = clampf (fb, -4.0f, 4.0f);
            lines[(size_t) i].push (in * 0.5f + fb);
        }

        outL = 0.0f; outR = 0.0f;
        for (int i = 0; i < N; i += 2) { outL += y[(size_t) i]; outR += y[(size_t) (i + 1)]; }
        outL *= 0.5f; outR *= 0.5f;
        lastL = std::tanh (outL);
        lastR = std::tanh (outR);
    }
};

// ==========================================================================
//  DELAY stereo con ping-pong
// ==========================================================================
struct StereoDelay
{
    DelayLine dl, dr;
    OnePoleLP lpL, lpR;
    SVF hpL, hpR;
    Smoothed time;
    double sr = 48000.0;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        dl.resize ((int) (4.5 * sr));
        dr.resize ((int) (4.5 * sr));
        hpL.set (150.0f, 0.7f, sr);
        hpR.set (150.0f, 0.7f, sr);
        time.prepare (sr, 0.15f, (float) (0.5 * sr));
    }

    void setParams (float seconds, float tone)
    {
        time.set (clampf (seconds, 0.01f, 4.0f) * (float) sr);
        const float hz = 1500.0f * std::pow (12000.0f / 1500.0f, clampf (tone, 0.0f, 1.0f));
        lpL.setCutoff (hz, sr);
        lpR.setCutoff (hz, sr);
    }

    void process (float xL, float xR, float feedback, bool pingPong, float& outL, float& outR)
    {
        const float t = time.next();
        const float yL = dl.read (t), yR = dr.read (t);
        const float fL = std::tanh (hpL.highpass (lpL.process (yL)) * feedback);
        const float fR = std::tanh (hpR.highpass (lpR.process (yR)) * feedback);
        if (pingPong)
        {
            dl.push (0.5f * (xL + xR) + fR);
            dr.push (fL);
        }
        else
        {
            dl.push (xL + fL);
            dr.push (xR + fR);
        }
        outL = yL; outR = yR;
    }
};

// ==========================================================================
//  TEXTURE: pioggia, lo-fi, grit
// ==========================================================================
struct Rain
{
    struct Drop { float env = 0.0f, decay = 0.0f, pl = 0.0f, pr = 0.0f; SVF f; };
    static constexpr int kDrops = 16;
    std::array<Drop, kDrops> drops;
    int nextDrop = 0;
    Rng rng { 0xBADC0DEu };
    SVF hissL, hissR;
    double sr = 48000.0;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        hissL.set (5000.0f, 0.5f, sr);
        hissR.set (5200.0f, 0.5f, sr);
    }

    void process (float amount, float& outL, float& outR)
    {
        const float rate = 30.0f + 400.0f * amount; // gocce al secondo
        if (rng.next() < rate / (float) sr)
        {
            auto& d = drops[(size_t) nextDrop];
            nextDrop = (nextDrop + 1) % kDrops;
            const float a = 0.2f + 0.8f * rng.next();
            d.env = a * a;
            d.decay = std::exp (-1.0f / ((0.002f + 0.02f * rng.next()) * (float) sr));
            d.f.set (1500.0f + 7500.0f * rng.next(), 2.0f + 4.0f * rng.next(), sr);
            const float p = rng.next();
            d.pl = std::cos (p * kPi * 0.5f);
            d.pr = std::sin (p * kPi * 0.5f);
        }
        float l = 0.0f, r = 0.0f;
        for (auto& d : drops)
        {
            if (d.env < 1.0e-5f) continue;
            const float v = d.f.bandpass (rng.bipolar() * d.env);
            d.env *= d.decay;
            l += v * d.pl;
            r += v * d.pr;
        }
        // fruscio di fondo
        l += 0.08f * hissL.bandpass (rng.bipolar());
        r += 0.08f * hissR.bandpass (rng.bipolar());
        outL = l; outR = r;
    }
};

struct LoFi
{
    DelayLine wowL, wowR;
    SVF lpL, lpR;
    float wowPh = 0.0f, flutPh = 0.0f, holdPh = 0.0f, heldL = 0.0f, heldR = 0.0f;
    double sr = 48000.0;
    int counter = 0;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        wowL.resize ((int) (0.02 * sr));
        wowR.resize ((int) (0.02 * sr));
    }

    void process (float amt, float& l, float& r)
    {
        if (counter++ % 32 == 0)
        {
            const float hz = 20000.0f * std::pow (3000.0f / 20000.0f, amt);
            lpL.set (hz, 0.7f, sr);
            lpR.set (hz, 0.7f, sr);
        }
        // wow & flutter (nastro)
        wowPh += 0.55f / (float) sr;  if (wowPh >= 1.0f) wowPh -= 1.0f;
        flutPh += 6.5f / (float) sr;  if (flutPh >= 1.0f) flutPh -= 1.0f;
        const float ms = amt * (3.0f + 1.8f * std::sin (kTwoPi * wowPh) + 0.12f * std::sin (kTwoPi * flutPh));
        const float dSamp = 1.0f + ms * 0.001f * (float) sr;
        wowL.push (l); wowR.push (r);
        l = wowL.read (dSamp);
        r = wowR.read (dSamp);

        l = lpL.lowpass (l);
        r = lpR.lowpass (r);

        // riduzione di sample-rate e bit
        if (amt > 0.02f)
        {
            const float targetSr = (float) sr * std::pow (8000.0f / (float) sr, amt);
            holdPh += targetSr / (float) sr;
            if (holdPh >= 1.0f) { holdPh -= 1.0f; heldL = l; heldR = r; }
            const float steps = std::pow (2.0f, 16.0f - 10.0f * amt);
            l = std::round (heldL * steps) / steps;
            r = std::round (heldR * steps) / steps;
        }
    }
};

inline float grit (float x, float amt)
{
    if (amt <= 0.0001f) return x;
    const float drive = 1.0f + amt * 15.0f;
    const float bias = 0.15f * amt;
    const float sat = (std::tanh (drive * x + bias) - std::tanh (bias)) / std::pow (drive, 0.6f);
    return x + amt * (sat - x);
}

// ==========================================================================
//  ENGINE
// ==========================================================================
struct Params
{
    float mix = 0.6f, outputDb = 0.0f;
    bool freeze = false;

    bool harmOn = true;   float harmAmount = 0.5f, harmDecay = 0.5f;
    int key = 0, scale = 0; float vowel = 0.0f, vowelMix = 0.0f;

    bool moodOn = true;   int moodType = 0; float moodDepth = 0.5f;

    bool diffOn = true;   float diffAmount = 0.5f, diffSize = 0.6f, diffDecay = 0.6f, diffDamp = 0.4f;

    bool texOn = true;    float rain = 0.0f, lofi = 0.0f, grit = 0.0f;

    bool dlyOn = true;    int dlyTime = 4; float dlyAmount = 0.3f, dlyFeedback = 0.4f, dlyTone = 0.6f;
    bool pingPong = true;
};

// Divisioni di tempo del delay (in battiti)
static constexpr int kNumDelayDivs = 7;
static constexpr float kDelayBeats[kNumDelayDivs] = { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };

class Engine
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        harmonic.prepare (sr);
        ensL.prepare (sr, 0.0f);
        ensR.prepare (sr, 0.25f);
        reverb.prepare (sr);
        delay.prepare (sr);
        rain.prepare (sr);
        lofi.prepare (sr);

        tiltLPL.setCutoff (700.0f, sr);
        tiltLPR.setCutoff (700.0f, sr);

        const float t = 0.03f;
        for (auto* s : { &mix, &outGain, &harmOn, &harmAmt, &vowelMix, &vowelPos, &moodOn, &moodDepth,
                         &diffOn, &diffAmt, &texOn, &rainAmt, &lofiAmt, &gritAmt, &dlyOn, &dlyAmt, &dlyFb })
            s->prepare (sr, t, 0.0f);
        vowelPos.prepare (sr, 0.08f, 0.0f);
        envCoefA = 1.0f - std::exp (-1.0f / (0.01f * (float) sr));
        envCoefR = 1.0f - std::exp (-1.0f / (2.5f * (float) sr));
        first = true;
    }

    void process (float* L, float* R, int numSamples, const Params& p, double bpm)
    {
        // ---- parametri a livello di blocco
        auto on = [] (bool b) { return b ? 1.0f : 0.0f; };
        mix.set (p.mix);              outGain.set (dbToGain (p.outputDb));
        harmOn.set (on (p.harmOn));   harmAmt.set (p.harmAmount);
        vowelMix.set (p.vowelMix);    vowelPos.set (p.vowel);
        moodOn.set (on (p.moodOn));   moodDepth.set (p.moodDepth);
        diffOn.set (on (p.diffOn));   diffAmt.set (p.diffAmount);
        texOn.set (on (p.texOn));     rainAmt.set (p.rain); lofiAmt.set (p.lofi); gritAmt.set (p.grit);
        dlyOn.set (on (p.dlyOn));     dlyAmt.set (p.dlyAmount); dlyFb.set (p.dlyFeedback);
        if (first)
        {
            for (auto* s : { &mix, &outGain, &harmOn, &harmAmt, &vowelMix, &vowelPos, &moodOn, &moodDepth,
                             &diffOn, &diffAmt, &texOn, &rainAmt, &lofiAmt, &gritAmt, &dlyOn, &dlyAmt, &dlyFb })
                s->cur = s->target;
            first = false;
        }

        harmonic.update (p.key, p.scale, p.harmDecay);
        reverb.setParams (p.diffSize, p.diffDecay, p.diffDamp);

        const double safeBpm = (bpm > 20.0 && bpm < 400.0) ? bpm : 120.0;
        const int div = std::clamp (p.dlyTime, 0, kNumDelayDivs - 1);
        delay.setParams ((float) (kDelayBeats[div] * 60.0 / safeBpm), p.dlyTone);

        const MoodSettings& mood = kMoods[std::clamp (p.moodType, 0, kNumMoods - 1)];

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = L[i], dryR = R[i];
            float l = dryL, r = dryR;

            const float mx = mix.next(), og = outGain.next();
            const float hOn = harmOn.next(), hAmt = harmAmt.next(), vMix = vowelMix.next(), vPos = vowelPos.next();
            const float mOn = moodOn.next(), mDepth = moodDepth.next();
            const float dOn = diffOn.next(), dAmt = diffAmt.next();
            const float tOn = texOn.next(), rAmt = rainAmt.next(), lAmt = lofiAmt.next(), gAmt = gritAmt.next();
            const float yOn = dlyOn.next(), yAmt = dlyAmt.next(), yFb = dlyFb.next();

            // ---------------- HARMONIC
            {
                float hl, hr;
                harmonic.process (0.5f * (l + r), hl, hr);
                const float a = hOn * hAmt;
                l += a * hl * 1.5f;
                r += a * hr * 1.5f;

                if ((ctrlCounter & 31) == 0)
                {
                    formL.set (vPos, sr);
                    formR.set (std::min (1.0f, vPos + 0.02f), sr);
                }
                const float v = hOn * vMix;
                if (v > 0.0001f)
                {
                    l += v * (formL.process (l) - l);
                    r += v * (formR.process (r) - r);
                }
                else
                {
                    formL.process (l); formR.process (r);
                }
            }

            // ---------------- MOOD
            {
                const float depth = mOn * mDepth;
                const float cd = mood.chorusDepth * depth;
                const float cl = ensL.process (l, mood.rateHz, 0.4f + cd);
                const float cr = ensR.process (r, mood.rateHz, 0.4f + cd);
                l = l * (1.0f - 0.5f * cd) + cl * 0.9f * cd;
                r = r * (1.0f - 0.5f * cd) + cr * 0.9f * cd;

                const float tilt = mood.tilt * depth;
                const float lowG = std::pow (10.0f, -tilt * 0.2f), highG = std::pow (10.0f, tilt * 0.2f);
                const float lpl = tiltLPL.process (l), lpr = tiltLPR.process (r);
                l = lpl * lowG + (l - lpl) * highG;
                r = lpr * lowG + (r - lpr) * highG;

                const float w = mood.warmth * depth;
                if (w > 0.001f)
                {
                    const float k = 1.0f + 2.0f * w;
                    l = l + w * (std::tanh (l * k) / std::tanh (k) - l);
                    r = r + w * (std::tanh (r * k) / std::tanh (k) - r);
                }
            }

            // ---------------- DELAY
            {
                float dl, dr;
                delay.process (l, r, yFb * 1.05f, p.pingPong, dl, dr);
                const float a = yOn * yAmt;
                l += a * dl;
                r += a * dr;
            }

            // ---------------- DIFFUSION
            {
                const float depth = mOn * mDepth;
                const float shimAmt = mood.shimmerAmt * depth * 0.6f;
                const float ratio = std::pow (2.0f, mood.shimmerSemis / 12.0f);
                float rl, rr;
                reverb.process (l, r, p.freeze && p.diffOn, ratio, shimAmt, rl, rr);
                const float a = dOn * dAmt;
                l = l * (1.0f - 0.45f * a) + rl * 0.6f * a;
                r = r * (1.0f - 0.45f * a) + rr * 0.6f * a;
            }

            // ---------------- TEXTURE
            {
                const float lev = 0.5f * (std::abs (l) + std::abs (r));
                env += (lev > env ? envCoefA : envCoefR) * (lev - env);

                float tl = l, tr = r;
                lofi.process (tOn * lAmt, tl, tr);
                tl = grit (tl, tOn * gAmt);
                tr = grit (tr, tOn * gAmt);

                float rnl, rnr;
                rain.process (rAmt, rnl, rnr);
                const float rainLevel = tOn * rAmt * 0.25f * std::min (1.0f, 0.15f + env * 6.0f) * std::min (1.0f, env * 15.0f);
                l = tl + rnl * rainLevel;
                r = tr + rnr * rainLevel;
            }

            ++ctrlCounter;

            // ---------------- MIX + OUTPUT (soft clip di sicurezza)
            float oL = (dryL * (1.0f - mx) + l * mx) * og;
            float oR = (dryR * (1.0f - mx) + r * mx) * og;
            if (! std::isfinite (oL) || ! std::isfinite (oR)) { oL = oR = 0.0f; }
            L[i] = softClip (oL);
            R[i] = softClip (oR);
        }
    }

private:
    static float softClip (float x)
    {
        const float a = std::abs (x);
        if (a < 0.9f) return x;
        const float s = x < 0.0f ? -1.0f : 1.0f;
        return s * (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f));
    }

    double sr = 48000.0;
    Harmonic harmonic;
    FormantFilter formL, formR;
    Ensemble ensL, ensR;
    OnePoleLP tiltLPL, tiltLPR;
    FDNReverb reverb;
    StereoDelay delay;
    Rain rain;
    LoFi lofi;

    Smoothed mix, outGain, harmOn, harmAmt, vowelMix, vowelPos, moodOn, moodDepth,
             diffOn, diffAmt, texOn, rainAmt, lofiAmt, gritAmt, dlyOn, dlyAmt, dlyFb;
    float env = 0.0f, envCoefA = 0.01f, envCoefR = 0.0001f;
    uint32_t ctrlCounter = 0;
    bool first = true;
};

} // namespace dfx
