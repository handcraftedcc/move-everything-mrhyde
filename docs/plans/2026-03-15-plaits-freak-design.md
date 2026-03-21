# Plaits Freak Design

## Goal

Build a Schwung sound generator module inspired by MicroFreak that uses Mutable Instruments Plaits source code as its synthesis core, while keeping existing Move module packaging/layout conventions.

## Architecture

The module is split into:

1. `plaits_plugin.cpp` (Move plugin ABI adapter, parameter parsing, MIDI ingest, audio render loop).
2. `plaits_move_engine.{h,cpp}` (Move-specific voice manager, modulation routing, UI-facing parameters).
3. Vendored upstream core in `src/third_party/eurorack/plaits` and `src/third_party/eurorack/stmlib`.

The wrapper keeps upstream Plaits DSP mostly untouched and applies Move behavior in a thin adapter layer so future upstream sync can be done by replacing the vendored tree.

## Components

### DSP Core

- Use `plaits::Voice` for oscillator model rendering and LPG behavior.
- Support full engine selection range exposed by upstream `kMaxEngines`.
- Maintain per-voice state for note lifecycle and modulation values.

### Modulation System

Implement all requested sources:

- LFO
- Envelope (ADSR)
- Cycling Envelope
- Random (S&H / Smooth / Drift)
- Velocity
- Poly Aftertouch

Destination pages:

- Harmonics Mod
- Timbre Mod
- Morph Mod
- FM Mod
- LPG Mod

Modulation amount `0` is a strict no-op for each source/destination path. A global troubleshooting switch disables all destination modulation contributions at once without disabling core synthesis.

### Voice and Performance

- Voice modes: mono, poly, mono legato.
- Polyphony / unison budget clamped to keep rendered voices <= 8.
- Per-voice LPG and modulator state.
- Glide and spread handled in Move wrapper.

### UI/Metadata

- `module.json` hierarchy follows provided spec sections with minimal nesting.
- `help.json` and `README.md` mirror existing project style.
- `ui.js` remains a thin wrapper using shared Move sound generator UI.

### Build/Release

- Keep `scripts/build.sh`, `scripts/install.sh`, `scripts/Dockerfile` structure.
- Add local upstream sync helper script for vendored Plaits sources.
- Add `.github/workflows/release.yml` tag-based packaging workflow and `release.json` update behavior.

## Data Flow

1. Host sends MIDI and parameter updates.
2. Plugin adapter parses/clamps and updates engine state.
3. Engine computes per-voice modulation sources each block.
4. Destination modulation matrix applies source contributions (or bypasses entirely if global disable is set).
5. Engine renders audio from Plaits core and writes interleaved stereo frames.

## Error Handling

- Unknown params are ignored with non-fatal error text via `get_error`.
- File/source sync failures do not affect DSP runtime.
- Out-of-range values are clamped in normal mode.

## Testing Strategy

1. Unit tests for modulation math, zero-amount bypass, and global mod-disable behavior.
2. Engine tests for voice budgeting and render stability under poly/unison stress.
3. Plugin-level smoke tests for parameter set/get and note rendering non-silence.
4. Build verification from Docker cross-compile path.

## Upstream Sync Strategy

- Track upstream eurorack commit hash in docs/script output.
- Sync script refreshes vendored trees (`plaits`, `stmlib`) and preserves wrapper files.
- Keep wrapper independent from vendored file modifications whenever possible.
