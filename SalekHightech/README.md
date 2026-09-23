# SALEK HIGHTECH — build & CI guide

## What you have right now
A real, compiling JUCE synth (no fake UI, no fake DSP):
- Band-limited wavetable oscillator w/ frame morphing, warp/fold/phase-distortion/sync, unison, FM/PM input.
- 16-voice polyphonic synth using `juce::Synthesiser`, one oscillator per voice + ADSR.
- Fully automatable parameters via `AudioProcessorValueTreeState` (frame position, warp mode/amount, unison, ADSR, gain) — every knob is bound to real DSP, not decoration.
- Preset save/load already works for free, since it's just APVTS state (`getStateInformation`/`setStateInformation`).
- A **placeholder** editor (plain knobs) so it's playable today. The full SALEK HIGHTECH neon/cyberpunk visual identity replaces this in a later step.

## Build it yourself (recommended first)
Requires CMake 3.22+ and a C++20 compiler. First configure needs internet (JUCE is fetched via `FetchContent`).

```bash
cmake -B build
cmake --build build --config Release
```

Find the built plugin under `build/SalekHightech_artefacts/Release/VST3/` and `.../Standalone/`.

## GitHub Actions CI (what I just added)
`.github/workflows/build.yml` builds **Windows, macOS, and Linux** on every push to `main` (and manually via "Run workflow"), and uploads:
- `SalekHightech-Windows` → VST3 + Standalone .exe
- `SalekHightech-macOS` → VST3 + AU + Standalone .app
- `SalekHightech-Linux` → VST3 + Standalone binary

### To use it
1. Create a GitHub repo and push this folder as-is (the `.github/workflows/build.yml` file must be at the repo root under that exact path).
2. Push to `main` — the "Build SALEK HIGHTECH" workflow runs automatically.
3. Go to the repo's **Actions** tab → open the latest run → download the artifact zip for your platform from the bottom of the run page.
4. Windows: copy the `.vst3` folder into `C:\Program Files\Common Files\VST3\`.
   macOS: copy `.vst3` into `~/Library/Audio/Plug-Ins/VST3/` and `.component` into `~/Library/Audio/Plug-Ins/Components/`.
   Linux: copy `.vst3` into `~/.vst3/`.
5. Rescan plugins in your DAW.

I can't push to GitHub or run this workflow myself (no network/git in this sandbox) — you'll need to create the repo and push. If a run fails, paste me the Actions log and I'll fix it.

## Next steps (as originally planned)
1. Second/third oscillators, sub + noise, mixed into the voice.
2. Multimode filter (SVF: LP/HP/BP/notch) per voice + filter envelope.
3. LFOs (drawable, tempo-synced, S&H/random) + real modulation matrix.
4. FX chain (distortion/delay/reverb/phaser/chorus).
5. Arp + step sequencer.
6. The actual SALEK HIGHTECH UI — this is intentionally last, once there are enough real parameters to bind the visuals to.
