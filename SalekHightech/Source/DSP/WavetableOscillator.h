#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>

namespace salek
{

/**
    A single band-limited wavetable "frame" set: one waveform rendered at
    multiple mip levels (one per octave range) so we never alias no matter
    how high the played note is. This is real additive-synthesis table
    generation, not a lookup of a raw waveform.
*/
class WavetableFrame
{
public:
    static constexpr int tableSize   = 2048;
    static constexpr int numMipLevels = 10; // covers ~20Hz..20kHz at 44.1k+

    // Build mip-mapped, band-limited tables from a raw single-cycle waveform
    // (e.g. imported from a WAV, drawn in the wavetable editor, or generated
    // from a harmonic recipe). Uses an FFT to get harmonic content, then
    // resynthesizes each mip level with harmonics above Nyquist/2 removed.
    void buildFromHarmonics (const std::vector<std::complex<float>>& harmonics, double sampleRate);
    void buildFromRawCycle  (const std::vector<float>& rawCycle, double sampleRate);

    // Read a band-limited sample at phase [0,1) for a given fundamental freq.
    float read (float phase, float frequencyHz, double sampleRate) const noexcept;

private:
    int mipLevelForFrequency (float frequencyHz, double sampleRate) const noexcept;
    std::array<std::vector<float>, numMipLevels> mips; // each mip: tableSize+1 samples (last==first, for interpolation)
};

/** A full wavetable = an ordered set of frames the user can morph across. */
struct Wavetable
{
    std::vector<WavetableFrame> frames;
    juce::String name { "INIT" };

    // Load a standard Serum-style multi-frame wavetable WAV (N * 2048 or N*4096 samples)
    static Wavetable loadFromWavFile (const juce::File& file, double sampleRate);
    static Wavetable makeDefaultAnalogStack (double sampleRate); // saw/square/tri morph, built-in
};

enum class WarpMode  { Off, WavefoldSym, WavefoldAsym, PhaseDistortion, Bend, Sync };

/**
    One oscillator: wavetable playback + frame-morph + warp/fold/PD + optional
    FM/PM input from another oscillator, exactly the signal path SALEK
    HIGHTECH's OSC panels drive.
*/
class WavetableOscillator
{
public:
    void prepare (double sampleRate) noexcept;
    void setWavetable (const Wavetable* table) noexcept { wavetable = table; }

    void setFrequency (float hz) noexcept          { frequencyHz = hz; }
    void setFramePosition (float pos01) noexcept    { framePos = juce::jlimit (0.0f, 1.0f, pos01); }
    void setWarpMode (WarpMode m) noexcept          { warpMode = m; }
    void setWarpAmount (float amt01) noexcept       { warpAmount = juce::jlimit (0.0f, 1.0f, amt01); }
    void setPhaseModInput (float pmSample) noexcept { pmIn = pmSample; }   // for FM/PM chaining between oscillators
    void setUnisonVoices (int n) noexcept           { unisonVoices = juce::jlimit (1, 16, n); }
    void setUnisonDetuneCents (float c) noexcept    { unisonDetune = c; }
    void setUnisonSpread (float s01) noexcept       { unisonSpread = juce::jlimit (0.0f, 1.0f, s01); }
    void reset() noexcept;

    // Renders one sample. `fmSample` = external FM modulator audio (linear FM).
    float renderSample (float fmSample = 0.0f) noexcept;

private:
    float readFrame (float phase) const noexcept;   // wavetable read w/ frame morph (crossfades 2 nearest frames)
    float applyWarp (float phase, float raw) const noexcept;

    const Wavetable* wavetable = nullptr;
    double sampleRate = 44100.0;
    float frequencyHz = 440.0f;
    float framePos = 0.0f;
    WarpMode warpMode = WarpMode::Off;
    float warpAmount = 0.0f;
    float pmIn = 0.0f;

    int unisonVoices = 1;
    float unisonDetune = 8.0f;
    float unisonSpread = 0.5f;
    std::array<double, 16> unisonPhase {};
};

} // namespace salek
