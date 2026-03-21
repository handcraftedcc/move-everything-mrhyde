# Plaits Freak Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Deliver a first working Schwung module that uses vendored Mutable Instruments Plaits as the synthesis core, with all requested modulation sources and release/build parity.

**Architecture:** Keep upstream Plaits/stmlib sources vendored and mostly unmodified, then implement a Move adapter engine/plugin layer that provides parameter mapping, modulation routing, MIDI handling, and output rendering. Preserve existing module packaging/build/release structure and add targeted tests around the wrapper behavior.

**Tech Stack:** C++14 DSP plugin, vendored C++ Plaits/stmlib, shell build scripts, Node test runner for JS helpers, GitHub Actions release workflow.

---

### Task 1: Establish vendored upstream source tree and module rename baseline

**Files:**
- Create: `src/third_party/eurorack/plaits/**`
- Create: `src/third_party/eurorack/stmlib/**`
- Modify: `scripts/build.sh`
- Modify: `scripts/install.sh`

**Step 1: Write the failing test**

Create `tests/plaits_vendor_layout_test.sh` expecting vendored trees and specific anchor files.

**Step 2: Run test to verify it fails**

Run: `bash tests/plaits_vendor_layout_test.sh`  
Expected: FAIL with missing `src/third_party/eurorack/plaits/dsp/voice.h`

**Step 3: Write minimal implementation**

Vendor upstream `plaits` and `stmlib` into `src/third_party/eurorack`, and update build/install scripts to target the new module directory naming.

**Step 4: Run test to verify it passes**

Run: `bash tests/plaits_vendor_layout_test.sh`  
Expected: PASS

**Step 5: Commit**

```bash
git add src/third_party/eurorack scripts/build.sh scripts/install.sh tests/plaits_vendor_layout_test.sh
git commit -m "chore: vendor plaits and stmlib sources"
```

### Task 2: Add failing modulation behavior tests (zero-amount no-op and global disable)

**Files:**
- Create: `tests/plaits_mod_matrix_test.cpp`
- Create: `src/dsp/plaits_move_engine.h`
- Create: `src/dsp/plaits_move_engine.cpp`

**Step 1: Write the failing test**

Add tests that assert:
- Destination modulation output equals base value when all amounts are zero.
- Destination modulation output equals base value when global disable flag is enabled.

**Step 2: Run test to verify it fails**

Run: `g++ -std=c++14 -Isrc/dsp tests/plaits_mod_matrix_test.cpp -o /tmp/plaits_mod_matrix_test && /tmp/plaits_mod_matrix_test`  
Expected: FAIL due to missing engine APIs/types.

**Step 3: Write minimal implementation**

Implement a modulation evaluator API in `plaits_move_engine` with explicit source terms and destination amounts, including global disable behavior.

**Step 4: Run test to verify it passes**

Run: `g++ -std=c++14 -Isrc/dsp tests/plaits_mod_matrix_test.cpp src/dsp/plaits_move_engine.cpp -o /tmp/plaits_mod_matrix_test && /tmp/plaits_mod_matrix_test`  
Expected: PASS

**Step 5: Commit**

```bash
git add tests/plaits_mod_matrix_test.cpp src/dsp/plaits_move_engine.h src/dsp/plaits_move_engine.cpp
git commit -m "test: define modulation matrix behavior for plaits wrapper"
```

### Task 3: Build plugin wrapper around Plaits and satisfy plugin smoke tests

**Files:**
- Create: `src/dsp/plaits_plugin.cpp`
- Modify: `scripts/build.sh`
- Create: `tests/plaits_plugin_smoke_test.cpp`

**Step 1: Write the failing test**

Create plugin smoke test to:
- load API v2
- create instance
- set/get key params
- send note on and render non-zero audio

**Step 2: Run test to verify it fails**

Run: `bash tests/run_plaits_plugin_smoke.sh`  
Expected: FAIL while plugin symbols/files are absent.

**Step 3: Write minimal implementation**

Implement Move plugin ABI adapter that routes MIDI/params into `plaits_move_engine` and returns stereo int16 output.

**Step 4: Run test to verify it passes**

Run: `bash tests/run_plaits_plugin_smoke.sh`  
Expected: PASS

**Step 5: Commit**

```bash
git add src/dsp/plaits_plugin.cpp scripts/build.sh tests/plaits_plugin_smoke_test.cpp tests/run_plaits_plugin_smoke.sh
git commit -m "feat: add plaits-based move plugin wrapper"
```

### Task 4: Add module metadata/UI/docs parity

**Files:**
- Modify: `src/module.json`
- Modify: `src/help.json`
- Modify: `src/ui.js`
- Modify: `README.md`
- Modify: `release.json`

**Step 1: Write the failing test**

Add `tests/module_metadata_test.mjs` validating:
- required chain params exist for all modulation sources/destinations
- hierarchy levels match spec sections
- defaults exist for key params

**Step 2: Run test to verify it fails**

Run: `node tests/module_metadata_test.mjs`  
Expected: FAIL due to missing/old Granny fields.

**Step 3: Write minimal implementation**

Update metadata, UI wrapper naming, help text, and README to the new module.

**Step 4: Run test to verify it passes**

Run: `node tests/module_metadata_test.mjs`  
Expected: PASS

**Step 5: Commit**

```bash
git add src/module.json src/help.json src/ui.js README.md release.json tests/module_metadata_test.mjs
git commit -m "docs: align module metadata and docs for plaits freak"
```

### Task 5: Add release workflow and run full verification

**Files:**
- Create: `.github/workflows/release.yml`
- Modify: `scripts/build.sh`
- Create: `tests/run_all.sh`

**Step 1: Write the failing test**

Add `tests/workflow_presence_test.sh` ensuring release workflow exists and references tag pushes and packaged tarball.

**Step 2: Run test to verify it fails**

Run: `bash tests/workflow_presence_test.sh`  
Expected: FAIL due to missing workflow.

**Step 3: Write minimal implementation**

Add release workflow and wire final verification script that runs all test suites before build.

**Step 4: Run test to verify it passes**

Run: `bash tests/workflow_presence_test.sh && bash tests/run_all.sh && ./scripts/build.sh`  
Expected: PASS and `dist/*` artifacts created.

**Step 5: Commit**

```bash
git add .github/workflows/release.yml tests/workflow_presence_test.sh tests/run_all.sh scripts/build.sh
git commit -m "ci: add tag-based release workflow and verification script"
```
