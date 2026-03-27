#include <cstdio>
#include <cstdlib>

#include "plaits_move_engine.h"

static void fail(const char *msg) {
    std::fprintf(stderr, "FAIL: %s\n", msg);
    std::exit(1);
}

static void render_some(ppf_engine_t &engine, int frames, int block) {
    float l[PPF_MAX_RENDER];
    float r[PPF_MAX_RENDER];
    int remaining = frames;
    while (remaining > 0) {
        int n = remaining > block ? block : remaining;
        engine.render(l, r, n);
        remaining -= n;
    }
}

static float render_peak(ppf_engine_t &engine, int frames, int block) {
    float l[PPF_MAX_RENDER];
    float r[PPF_MAX_RENDER];
    float peak = 0.0f;
    int remaining = frames;
    while (remaining > 0) {
        int n = remaining > block ? block : remaining;
        engine.render(l, r, n);
        for (int i = 0; i < n; ++i) {
            float al = l[i] < 0.0f ? -l[i] : l[i];
            float ar = r[i] < 0.0f ? -r[i] : r[i];
            if (al > peak) peak = al;
            if (ar > peak) peak = ar;
        }
        remaining -= n;
    }
    return peak;
}

static float render_peak_after_skip(ppf_engine_t &engine, int skip_frames, int measure_frames, int block) {
    render_some(engine, skip_frames, block);
    return render_peak(engine, measure_frames, block);
}

int main() {
    ppf_engine_t engine;
    float tuning_compensation = engine.debug_pitch_compensation_semitones();
    if (tuning_compensation < 1.30f || tuning_compensation > 1.55f) {
        fail("pitch compensation should account for Plaits corrected sample-rate offset");
    }

    ppf_params_t params;
    ppf_default_params(&params);
    params.voice_mode = 1;  // poly
    params.polyphony = 4;
    params.unison = 1;
    params.env_attack_ms = 0;
    params.env_decay_ms = 0;
    params.env_sustain = 0.0f;
    params.env_release_ms = 5;
    engine.set_params(params);

    engine.note_on(60, 1.0f);
    render_some(engine, 256, 64);
    if (engine.debug_active_voice_count() != 1) {
        fail("first note_on should allocate one voice");
    }
    if (engine.debug_active_note_count(60) != 1) {
        fail("first note_on should have one active C4 voice");
    }

    engine.note_on(60, 0.8f);
    render_some(engine, 256, 64);
    if (engine.debug_active_voice_count() != 1) {
        fail("retriggering same note in poly should steal/reuse, not duplicate");
    }
    if (engine.debug_active_note_count(60) != 1) {
        fail("same note should still have a single active voice");
    }

    ppf_engine_t mono_engine;
    ppf_params_t mono_params;
    ppf_default_params(&mono_params);
    mono_params.voice_mode = 0;  // mono
    mono_params.unison = 3;
    mono_engine.set_params(mono_params);
    mono_engine.note_on(60, 1.0f);
    render_some(mono_engine, 256, 64);
    if (mono_engine.debug_active_voice_count() != 3) {
        fail("mono with unison should allocate a full unison stack");
    }
    if (mono_engine.debug_active_note_count(60) != 3) {
        fail("mono unison should stack voices on the played note");
    }
    mono_engine.note_on(64, 1.0f);
    render_some(mono_engine, 256, 64);
    if (mono_engine.debug_active_voice_count() != 3) {
        fail("mono should keep unison stack size when changing notes");
    }
    if (mono_engine.debug_active_note_count(60) != 0) {
        fail("mono should not allow chords when changing notes");
    }
    if (mono_engine.debug_active_note_count(64) != 3) {
        fail("mono note changes should retrigger the full unison stack");
    }

    ppf_engine_t legato_engine;
    ppf_params_t legato_params;
    ppf_default_params(&legato_params);
    legato_params.voice_mode = 2;  // mono-legato
    legato_params.unison = 2;
    legato_engine.set_params(legato_params);
    legato_engine.note_on(67, 1.0f);
    render_some(legato_engine, 256, 64);
    if (legato_engine.debug_active_voice_count() != 2) {
        fail("mono-legato with unison should allocate all unison voices");
    }
    if (legato_engine.debug_active_note_count(67) != 2) {
        fail("mono-legato unison should stack voices on the played note");
    }
    legato_engine.note_on(71, 1.0f);
    render_some(legato_engine, 256, 64);
    if (legato_engine.debug_active_voice_count() != 2) {
        fail("mono-legato should keep unison stack size when changing notes");
    }
    if (legato_engine.debug_active_note_count(67) != 0) {
        fail("mono-legato should not allow chords");
    }
    if (legato_engine.debug_active_note_count(71) != 2) {
        fail("mono-legato note changes should move the full unison stack");
    }

    ppf_engine_t live_detune_engine;
    ppf_params_t live_detune_params;
    ppf_default_params(&live_detune_params);
    live_detune_params.voice_mode = 0;  // mono
    live_detune_params.unison = 2;
    live_detune_params.detune = 0.0f;
    live_detune_params.spread = 0.0f;
    live_detune_engine.set_params(live_detune_params);
    live_detune_engine.note_on(60, 1.0f);
    render_some(live_detune_engine, 256, 64);

    float initial_target_0 = live_detune_engine.debug_voice_note_target(0);
    float initial_target_1 = live_detune_engine.debug_voice_note_target(1);
    float initial_pan_0 = live_detune_engine.debug_voice_pan(0);
    float initial_pan_1 = live_detune_engine.debug_voice_pan(1);

    if (initial_target_0 != 60.0f || initial_target_1 != 60.0f) {
        fail("detune=0 should keep unison voice targets centered on note pitch");
    }
    if (initial_pan_0 != 0.0f || initial_pan_1 != 0.0f) {
        fail("spread=0 should keep unison voice pan centered");
    }

    live_detune_params.detune = 1.0f;
    live_detune_params.spread = 1.0f;
    live_detune_engine.set_params(live_detune_params);
    render_some(live_detune_engine, 256, 64);

    float live_target_0 = live_detune_engine.debug_voice_note_target(0);
    float live_target_1 = live_detune_engine.debug_voice_note_target(1);
    float live_pan_0 = live_detune_engine.debug_voice_pan(0);
    float live_pan_1 = live_detune_engine.debug_voice_pan(1);

    if (live_target_0 >= 59.95f || live_target_1 <= 60.05f) {
        fail("detune should update live for held unison notes");
    }
    if (live_pan_0 >= -0.95f || live_pan_1 <= 0.95f) {
        fail("spread should update live for held unison notes");
    }

    engine.note_off(60);
    render_some(engine, PPF_SAMPLE_RATE * 8, 128);
    if (engine.debug_active_voice_count() != 0) {
        fail("note_off should fully release voice");
    }

    ppf_engine_t held_engine;
    ppf_params_t held_params;
    ppf_default_params(&held_params);
    held_params.voice_mode = 1;  // poly
    held_params.polyphony = 4;
    held_params.unison = 1;
    held_params.lpg_decay = 1.0f;
    held_engine.set_params(held_params);
    held_engine.note_on(64, 1.0f);
    render_some(held_engine, PPF_SAMPLE_RATE * 2, 128);
    float held_peak_before_release = render_peak(held_engine, 2048, 128);
    if (held_peak_before_release < 1e-5f) {
        fail("held note should remain audible before note_off");
    }
    held_engine.note_off(64);
    float early_release_peak = render_peak(held_engine, 2048, 128);
    if (early_release_peak < 1e-5f) {
        fail("held note release should keep audible tail until release completes");
    }
    render_some(held_engine, 128, 128);
    if (held_engine.debug_active_voice_count() == 0) {
        fail("held note release should not hard-cut immediately");
    }
    render_some(held_engine, PPF_SAMPLE_RATE * 2, 128);
    if (held_engine.debug_active_voice_count() == 0) {
        fail("max LPG decay should keep release active beyond 2 seconds");
    }
    render_some(held_engine, PPF_SAMPLE_RATE * 24, 128);
    if (held_engine.debug_active_voice_count() != 0) {
        fail("held note release should eventually finish");
    }

    ppf_engine_t short_lpg_audio_engine;
    ppf_params_t short_lpg_audio_params;
    ppf_default_params(&short_lpg_audio_params);
    short_lpg_audio_params.voice_mode = 1;  // poly
    short_lpg_audio_params.polyphony = 1;
    short_lpg_audio_params.unison = 1;
    short_lpg_audio_params.lpg_decay = 0.0f;
    short_lpg_audio_engine.set_params(short_lpg_audio_params);
    short_lpg_audio_engine.note_on(65, 1.0f);
    render_some(short_lpg_audio_engine, PPF_SAMPLE_RATE, 128);
    short_lpg_audio_engine.note_off(65);
    float short_lpg_tail = render_peak_after_skip(short_lpg_audio_engine, PPF_SAMPLE_RATE / 2, 4096, 128);
    if (short_lpg_tail > 0.02f) {
        fail("minimum LPG decay should fade quickly after release");
    }

    ppf_engine_t long_lpg_audio_engine;
    ppf_params_t long_lpg_audio_params;
    ppf_default_params(&long_lpg_audio_params);
    long_lpg_audio_params.voice_mode = 1;  // poly
    long_lpg_audio_params.polyphony = 1;
    long_lpg_audio_params.unison = 1;
    long_lpg_audio_params.lpg_decay = 1.0f;
    long_lpg_audio_engine.set_params(long_lpg_audio_params);
    long_lpg_audio_engine.note_on(65, 1.0f);
    render_some(long_lpg_audio_engine, PPF_SAMPLE_RATE, 128);
    long_lpg_audio_engine.note_off(65);
    float long_lpg_tail = render_peak_after_skip(long_lpg_audio_engine, PPF_SAMPLE_RATE / 2, 4096, 128);
    if (long_lpg_tail < 0.02f) {
        fail("maximum LPG decay should keep a clearly audible tail");
    }
    if (long_lpg_tail < short_lpg_tail * 2.0f) {
        fail("LPG decay control should audibly change tail length");
    }

    ppf_engine_t release_timing_engine;
    ppf_params_t release_timing_params;
    ppf_default_params(&release_timing_params);
    release_timing_params.voice_mode = 1;  // poly
    release_timing_params.polyphony = 1;
    release_timing_params.unison = 1;
    release_timing_params.lpg_decay = 0.0f;
    release_timing_params.env_release_ms = 5000;
    release_timing_engine.set_params(release_timing_params);
    release_timing_engine.note_on(60, 1.0f);
    render_some(release_timing_engine, 512, 64);
    release_timing_engine.note_off(60);
    int release_samples = release_timing_engine.debug_release_samples_total_for_note(60);
    if (release_samples <= 0) {
        fail("note_off should initialize release timing for active note");
    }
    if (release_samples >= PPF_SAMPLE_RATE) {
        fail("release timing should follow LPG decay, not ADSR release time");
    }

    ppf_engine_t modulated_release_engine;
    ppf_params_t modulated_release_params;
    ppf_default_params(&modulated_release_params);
    modulated_release_params.voice_mode = 1;  // poly
    modulated_release_params.polyphony = 1;
    modulated_release_params.unison = 1;
    modulated_release_params.lpg_decay = 0.0f;
    modulated_release_params.assign1_target = 3;  // PPF_ASSIGN_LPG_DECAY
    modulated_release_params.assign1_mod.velocity = 1.0f;
    modulated_release_engine.set_params(modulated_release_params);
    modulated_release_engine.note_on(62, 1.0f);
    render_some(modulated_release_engine, 512, 64);
    modulated_release_engine.note_off(62);
    int modulated_release_samples = modulated_release_engine.debug_release_samples_total_for_note(62);
    if (modulated_release_samples < PPF_SAMPLE_RATE * 8) {
        fail("release timing should track effective LPG decay, including assign modulation");
    }

    params.model = 0;
    engine.set_params(params);
    engine.note_on(48, 1.0f);
    render_some(engine, 512, 64);
    int engine_a = engine.debug_voice_active_engine(0);
    engine.note_off(48);
    render_some(engine, 4096, 128);

    params.model = 23;
    engine.set_params(params);
    engine.note_on(48, 1.0f);
    render_some(engine, 512, 64);
    int engine_b = engine.debug_voice_active_engine(0);

    if (engine_a == engine_b) {
        fail("model changes should select a different plaits engine");
    }

    ppf_engine_t mix_engine;
    ppf_params_t mix_params;
    ppf_default_params(&mix_params);
    mix_params.voice_mode = 1;  // poly
    mix_params.polyphony = 8;
    mix_params.unison = 1;
    mix_engine.set_params(mix_params);

    for (int n = 0; n < 8; ++n) {
        mix_engine.note_on(48 + n, 1.0f);
    }
    float chord_peak = render_peak(mix_engine, 4096, 128);
    if (chord_peak > 1.0f) {
        fail("poly voice mix peak should stay in [-1, 1]");
    }

    std::printf("PASS: plaits engine voice behavior\n");
    return 0;
}
