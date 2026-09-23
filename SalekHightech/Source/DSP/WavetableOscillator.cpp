#include "WavetableOscillator.h"

namespace salek
{

// ---------------------------------------------------------------- WavetableFrame

void WavetableFrame::buildFromHarmonics (const std::vector<std::complex<float>>& harmonics, double sampleRate)
{
    const int numHarmonics = (int) harmonics.size();

    for (int mip = 0; mip < numMipLevels; ++mip)
    {
        // Each mip level covers roughly one octave band; higher mip index ->
        // higher fundamental -> fewer harmonics allowed before Nyquist.
        const double approxFundamental = 20.0 * std::pow (2.0, mip); // 20,40,80,...~20kHz
        const int maxHarmonic = juce::jlimit (1, numHarmonics - 1,
            (int) std::floor ((sampleRate * 0.45) / approxFundamental));

        juce::dsp::FFT fft ((int) std::log2 (tableSize));
        std::vector<std::complex<float>> spectrum (tableSize, {0.0f, 0.0f});

        for (int h = 1; h <= maxHarmonic && h < numHarmonics; ++h)
            spectrum[(size_t) h] = harmonics[(size_t) h];

        // mirror for inverse real FFT
        std::vector<float> timeDomain (tableSize * 2, 0.0f);
        for (int i = 0; i < tableSize; ++i)
        {
            timeDomain[(size_t) i * 2]     = spectrum[(size_t) i].real();
            timeDomain[(size_t) i * 2 + 1] = spectrum[(size_t) i].imag();
        }
        fft.performRealOnlyInverseTransform (timeDomain.data());

        std::vector<float> table (tableSize + 1);
        float peak = 1.0e-9f;
        for (int i = 0; i < tableSize; ++i)
        {
            table[(size_t) i] = timeDomain[(size_t) i];
            peak = std::max (peak, std::abs (table[(size_t) i]));
        }
        for (int i = 0; i < tableSize; ++i)
            table[(size_t) i] /= peak; // normalize to [-1,1]
        table[tableSize] = table[0]; // wrap sample for interpolation

        mips[(size_t) mip] = std::move (table);
    }
}

void WavetableFrame::buildFromRawCycle (const std::vector<float>& rawCycle, double sampleRate)
{
    // Resample rawCycle to tableSize, FFT it to get harmonic content, then
    // hand off to buildFromHarmonics so every mip is properly band-limited.
    juce::dsp::FFT fft ((int) std::log2 (tableSize));
    std::vector<float> resampled (tableSize, 0.0f);

    const int srcLen = (int) rawCycle.size();
    for (int i = 0; i < tableSize; ++i)
    {
        const double srcPos = (double) i / tableSize * srcLen;
        const int i0 = (int) srcPos % srcLen;
        const int i1 = (i0 + 1) % srcLen;
        const float frac = (float) (srcPos - std::floor (srcPos));
        resampled[(size_t) i] = juce::jmap (frac, rawCycle[(size_t) i0], rawCycle[(size_t) i1]);
    }

    std::vector<float> fftBuf (tableSize * 2, 0.0f);
    for (int i = 0; i < tableSize; ++i)
        fftBuf[(size_t) i] = resampled[(size_t) i];

    fft.performRealOnlyForwardTransform (fftBuf.data());

    std::vector<std::complex<float>> harmonics (tableSize / 2, {0.0f, 0.0f});
    for (int i = 0; i < tableSize / 2; ++i)
        harmonics[(size_t) i] = { fftBuf[(size_t) i * 2], fftBuf[(size_t) i * 2 + 1] };

    buildFromHarmonics (harmonics, sampleRate);
}

int WavetableFrame::mipLevelForFrequency (float frequencyHz, double sampleRate) const noexcept
{
    juce::ignoreUnused (sampleRate);
    const float f = std::max (20.0f, frequencyHz);
    int mip = (int) std::floor (std::log2 (f / 20.0f));
    return juce::jlimit (0, numMipLevels - 1, mip);
}

float WavetableFrame::read (float phase, float frequencyHz, double sampleRate) const noexcept
{
    const int mip = mipLevelForFrequency (frequencyHz, sampleRate);
    const auto& table = mips[(size_t) mip];
    if (table.empty())
        return 0.0f;

    const float pos = phase * (float) tableSize;
    const int i0 = (int) pos;
    const int i1 = i0 + 1;
    const float frac = pos - (float) i0;
    return juce::jmap (frac, table[(size_t) i0], table[(size_t) i1]);
}

// ---------------------------------------------------------------- Wavetable

Wavetable Wavetable::makeDefaultAnalogStack (double sampleRate)
{
    Wavetable wt;
    wt.name = "SALEK ANALOG STACK";

    // 5 morphable frames: pure sine -> saw -> square -> triangle -> hard pulse
    const int numFrames = 5;
    const int numHarm = 512;

    for (int f = 0; f < numFrames; ++f)
    {
        std::vector<std::complex<float>> h (numHarm, {0.0f, 0.0f});
        const float t = (float) f / (float) (numFrames - 1);

        for (int n = 1; n < numHarm; ++n)
        {
            float sawAmp   = 1.0f / n;
            float sqAmp    = (n % 2 == 1) ? 1.0f / n : 0.0f;
            float triAmp   = (n % 2 == 1) ? 1.0f / (n * n) : 0.0f;
            float pulseAmp = (n % 3 == 0) ? 0.8f / n : 0.2f / n;

            float amp = 0.0f;
            if (t < 0.33f)      amp = juce::jmap (t, 0.0f, 0.33f, 0.0f, sawAmp);
            else if (t < 0.66f) amp = juce::jmap (t, 0.33f, 0.66f, sawAmp, sqAmp);
            else                amp = juce::jmap (t, 0.66f, 1.0f, sqAmp, pulseAmp);

            float phaseSign = (n % 2 == 0) ? -1.0f : 1.0f;
            h[(size_t) n] = { 0.0f, amp * phaseSign };
        }

        WavetableFrame frame;
        frame.buildFromHarmonics (h, sampleRate);
        wt.frames.push_back (std::move (frame));
    }
    return wt;
}

Wavetable Wavetable::loadFromWavFile (const juce::File& file, double sampleRate)
{
    Wavetable wt;
    wt.name = file.getFileNameWithoutExtension();

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr)
        return makeDefaultAnalogStack (sampleRate);

    const int totalSamples = (int) reader->lengthInSamples;
    const int frameSize = 2048; // standard Serum-style single-cycle frame length
    const int numFrames = juce::jmax (1, totalSamples / frameSize);

    juce::AudioBuffer<float> buf (1, totalSamples);
    reader->read (&buf, 0, totalSamples, 0, true, false);

    for (int f = 0; f < numFrames; ++f)
    {
        std::vector<float> cycle (frameSize);
        for (int i = 0; i < frameSize; ++i)
            cycle[(size_t) i] = buf.getSample (0, f * frameSize + i);

        WavetableFrame frame;
        frame.buildFromRawCycle (cycle, sampleRate);
        wt.frames.push_back (std::move (frame));
    }
    return wt;
}

// ---------------------------------------------------------------- WavetableOscillator

void WavetableOscillator::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    reset();
}

void WavetableOscillator::reset() noexcept
{
    for (auto& p : unisonPhase)
        p = 0.0;
}

float WavetableOscillator::readFrame (float phase) const noexcept
{
    if (wavetable == nullptr || wavetable->frames.empty())
        return 0.0f;

    const float fPos = framePos * (float) (wavetable->frames.size() - 1);
    const int f0 = (int) fPos;
    const int f1 = juce::jmin ((int) wavetable->frames.size() - 1, f0 + 1);
    const float frac = fPos - (float) f0;

    const float s0 = wavetable->frames[(size_t) f0].read (phase, frequencyHz, sampleRate);
    const float s1 = wavetable->frames[(size_t) f1].read (phase, frequencyHz, sampleRate);
    return juce::jmap (frac, s0, s1);
}

float WavetableOscillator::applyWarp (float phase, float raw) const noexcept
{
    switch (warpMode)
    {
        case WarpMode::Off:
            return raw;

        case WarpMode::WavefoldSym:
        {
            // Symmetric wavefolding: amplify then reflect at +-1
            float g = 1.0f + warpAmount * 6.0f;
            float x = raw * g;
            while (x > 1.0f || x < -1.0f)
            {
                if (x > 1.0f)  x = 2.0f - x;
                if (x < -1.0f) x = -2.0f - x;
            }
            return x;
        }

        case WarpMode::WavefoldAsym:
        {
            float g = 1.0f + warpAmount * 6.0f;
            float x = raw * g + warpAmount * 0.5f; // DC-offset bias -> asymmetric fold
            while (x > 1.0f || x < -1.0f)
            {
                if (x > 1.0f)  x = 2.0f - x;
                if (x < -1.0f) x = -2.0f - x;
            }
            return x - warpAmount * 0.25f;
        }

        case WarpMode::PhaseDistortion:
        {
            // Classic Casio-CZ style phase distortion: warp the READ phase,
            // not the amplitude. Re-read the table at the bent phase.
            float bent = phase;
            const float amt = warpAmount;
            if (phase < 0.5f)
                bent = std::pow (phase * 2.0f, 1.0f + amt * 4.0f) * 0.5f;
            else
                bent = 1.0f - std::pow ((1.0f - phase) * 2.0f, 1.0f + amt * 4.0f) * 0.5f;
            return readFrame (bent);
        }

        case WarpMode::Bend:
        {
            // Asymmetric time-bend (skews the waveform toward one side)
            float bendAmt = 0.05f + warpAmount * 0.9f;
            float bent = phase < bendAmt
                ? phase / bendAmt * 0.5f
                : 0.5f + (phase - bendAmt) / (1.0f - bendAmt) * 0.5f;
            return readFrame (bent);
        }

        case WarpMode::Sync:
        {
            // Hard sync: multiply phase rate, wrap
            float mult = 1.0f + warpAmount * 7.0f;
            float syncedPhase = std::fmod (phase * mult, 1.0f);
            return readFrame (syncedPhase);
        }
    }
    return raw;
}

float WavetableOscillator::renderSample (float fmSample) noexcept
{
    if (wavetable == nullptr)
        return 0.0f;

    float sum = 0.0f;
    const int voices = unisonVoices;

    for (int v = 0; v < voices; ++v)
    {
        // Spread detune across unison voices, centre voice stays in tune
        const float voiceOffset = voices > 1
            ? juce::jmap ((float) v, 0.0f, (float) (voices - 1), -1.0f, 1.0f)
            : 0.0f;
        const float detuneCents = voiceOffset * unisonDetune;
        const float detuneRatio = std::pow (2.0f, detuneCents / 1200.0f);
        const float pan = voiceOffset * unisonSpread; // used by voice mixer upstream

        double phaseInc = (double) (frequencyHz * detuneRatio) / sampleRate;

        // Linear FM: modulator sample bends the effective phase increment
        phaseInc *= (1.0 + (double) fmSample);

        // PM: modulator sample directly offsets the read phase
        double readPhase = std::fmod (unisonPhase[(size_t) v] + (double) pmIn, 1.0);
        if (readPhase < 0.0) readPhase += 1.0;

        float raw = readFrame ((float) readPhase);
        float warped = applyWarp ((float) readPhase, raw);

        juce::ignoreUnused (pan); // panning applied by SynthVoice's stereo mixer
        sum += warped;

        unisonPhase[(size_t) v] += phaseInc;
        if (unisonPhase[(size_t) v] >= 1.0) unisonPhase[(size_t) v] -= 1.0;
        if (unisonPhase[(size_t) v] < 0.0)  unisonPhase[(size_t) v] += 1.0;
    }

    return sum / std::sqrt ((float) voices); // equal-power sum so unison doesn't blow up gain
}

} // namespace salek
