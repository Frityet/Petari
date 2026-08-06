# Deterministic Dolphin oracle checkpoint

Captured 2026-08-06 UTC. This checkpoint establishes a repeatable original-game oracle for the
title, file-select, and picturebook sequence without modifying `Game/` or `pc-port/src/Game/`.

## Outcome

- Dolphin NoGUI builds and boots the Korean disc under Xvfb with the software renderer.
- Environment-driven Wii Remote 1 button and pointer spans use Dolphin's normal emulated
  controller and IR-camera path. There are no game-memory patches, addresses, scene names, or
  route-specific coordinates in the Dolphin implementation.
- A fresh title-to-file-select capture at frame 1900 is byte-identical to the existing cached
  oracle: SHA-256 `ca700cfbbd36852c5f95d78fbb4abf5b3d3bef6f8f3c373dbb22ee09361301de`.
- With the hash-pinned Dolphin user seed in this directory, the title-to-picturebook capture at
  frame 7600 is byte-identical to the existing cached oracle: SHA-256
  `89895d039e768e0dcb061d264ff3277ecda55fb7aae43505fb408c048ef7162d`.
- Both new SQLite traces pass `smg-pc-trace-validate-sqlite` with required `frame`,
  `render_packet`, and `semantic_event` records.
- Configured comparison crops are now extracted independently from both PNGs before dimensions
  are compared. This handles different source viewport heights without resizing or
  checkpoint-specific dimension rules.
- A fresh `file_select_far` paired run completed end to end with Dolphin at 640x456 and the PC
  port at 640x480. The configured 640x240 crop compared at normalized RMS `0.284755`, below the
  scenario threshold `0.35`.

The seed matters. A completely fresh Dolphin user has no game save, so the same route enters the
file-icon creation screen and is still there at frame 7600. The pre-existing cached picturebook
user already contained `GameData.bin` before its historical screenshot was made. Reusing a copy of
that state reproduces the picturebook frame exactly. The fresh-user screenshots here preserve that
diagnostic; it is not evidence of an IR-conversion failure.

## Implementation boundary

The Dolphin changes are intentionally separate from the PC port:

- `dolphin/Source/Core/Core/HW/WiimoteEmu/ScriptedInput.cpp` and `.h`: environment parsing,
  inclusive span selection, ordinary controller overrides, and the atomic presented-frame clock
- `dolphin/Source/Core/Core/HW/WiimoteEmu/WiimoteEmu.cpp`: installs the optional override on Wii
  Remote 1
- `dolphin/Source/Core/VideoCommon/Present.cpp`: publishes the same unique frame index used by
  capture
- `dolphin/Source/Core/Core/CMakeLists.txt`: registers the two new source files

The runner changes in `scripts/render_parity.lua` create each work directory explicitly and wait
for a journal-free SQLite database whose size remains stable for five seconds before terminating
the emulator. A process-exit path also rechecks the files after the process wait rather than using
stale pre-wait state. Whenever a scenario configures a crop, the runner requests crop-first visual
comparison.

`src/debug/VisualDiff.cpp` implements that general crop-first mode. It validates the same rectangle
against each source image, extracts both regions with their respective source strides, and only
then requires matching dimensions. Existing invocations retain their original behavior unless
`--crop-first` is supplied.

## Source and binary identity

- Dolphin source HEAD: `ccaee2f458bf60da63991a3731ff8546035793b4`
- Rebuilt binary version: `Dolphin ccaee2f458`
- Disc image: `/workspaces/pcport/RMGK01.iso`
  - container SHA-256: `0c321eff29251c8c7a1eed87d03dcf9d10908b2c2fdb177256e7ed9b39851ea6`
  - Dolphin raw-disc SHA-1: `2cf2139ad9d199cd1572463cca3dd4941f855d56`
- Extracted disc `main.dol` is byte-identical to both the original and rebuilt RMGK02 DOL:
  - `/workspaces/pcport/orig/RMGK02/sys/main.dol`
  - `/workspaces/pcport/build/RMGK02/main.dol`
  - SHA-256: `8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`
  - SHA-1: `54b71431af0d509097bfdef4ec28617afc487e89`

## Script contract

The Dolphin hook reads these optional variables when emulated Wii Remote 1 is constructed:

- `SMGPC_DOLPHIN_WPAD_BUTTON_SCRIPT`
  - semicolon-separated inclusive spans: `120-1700:A+B;2100-2120:A`
  - a single frame may be written as `2100:A`; `2100-` is open-ended
  - supported tokens: `A`, `B`, `ONE`/`1`, `TWO`/`2`, `PLUS`, `MINUS`/`-`, `HOME`,
    `UP`, `DOWN`, `LEFT`, and `RIGHT`
  - overlapping active spans are ORed; ordinary configured input remains available
- `SMGPC_DOLPHIN_WPAD_POINTER_SCRIPT`
  - semicolon-separated inclusive spans: `1800-2200:271,264,true`
  - the last matching span wins; `false` hides the IR cursor
  - coordinates use a 640x456 logical viewport by default
- `SMGPC_DOLPHIN_WPAD_POINTER_WIDTH` and `SMGPC_DOLPHIN_WPAD_POINTER_HEIGHT`
  - optional positive viewport overrides

Malformed entries are ignored with a Wii Remote warning. Script frames are driven by the same
unique-presented-frame counter used by the screenshot and render-trace hooks, not Dolphin's movie
counter (which advances once per VI field for this interlaced title).

## Reproduction

Build the isolated NoGUI target:

```sh
cd /workspaces/pcport/pc-port/dolphin
cmake --build build-nogui-libcxx --target dolphin-emu-nogui -j4
```

Fresh title-to-file-select capture:

```sh
cd /workspaces/pcport/pc-port
SMGPC_DOLPHIN_GAME='/workspaces/pcport/Super Mario Wii - Galaxy Adventure (Korea).rvz' \
SMGPC_DISC_IMAGE=/workspaces/pcport/RMGK01.iso \
SMGPC_PARITY_BUILD=0 \
SMGPC_PARITY_REFRESH_DOLPHIN=1 \
SMGPC_PARITY_WORK_DIR=/tmp/dolphin-oracle-file-select \
xmake render-parity-compare file_select_far
```

Seeded title-to-picturebook capture:

```sh
cd /workspaces/pcport/pc-port
mkdir -p /tmp/dolphin-oracle-picturebook
cp -a notes/dolphin-oracle-20260806T201750Z/seed-dolphin-user \
  /tmp/dolphin-oracle-picturebook/dolphin-user
SMGPC_DOLPHIN_GAME='/workspaces/pcport/Super Mario Wii - Galaxy Adventure (Korea).rvz' \
SMGPC_DISC_IMAGE=/workspaces/pcport/RMGK01.iso \
SMGPC_PARITY_BUILD=0 \
SMGPC_PARITY_REFRESH_DOLPHIN=1 \
SMGPC_PARITY_WORK_DIR=/tmp/dolphin-oracle-picturebook \
SMGPC_PARITY_DOLPHIN_USER=/tmp/dolphin-oracle-picturebook/dolphin-user \
xmake render-parity-compare picturebook_page1
```

That command now completes the trace and visual comparisons even though Dolphin produces 640x456
and the PC port produces 640x480. The runner invokes:

```text
smg-pc-visual-diff --crop 0,150,640,240 --crop-first \
  --max-full-normalized-rms 0.35 dolphin-frame-1900.png pcport-frame-1900.png
```

The extracted comparison is 640x240 and the observed normalized RMS is `0.284755`. The synthetic
640x480-versus-640x456 control and complete command output are preserved under
`visual-crop-proof/`.

Validate a captured trace:

```sh
build/linux/x86_64/debug/smg-pc-trace-validate-sqlite \
  --require-record-type frame \
  --require-record-type render_packet \
  --require-record-type semantic_event \
  /tmp/dolphin-oracle-picturebook/dolphin-frame-7600.trace.sqlite
```

## Preserved evidence

- `screenshots/title-frame-1900.png`: stable original title screen
- `screenshots/file-select-frame-1900.png`: fresh scripted file-select result
- `screenshots/fresh-icon-select-frame-3100.png`: fresh-user pointer diagnostic
- `screenshots/fresh-user-frame-7600.png`: fresh user remains in icon creation
- `screenshots/seeded-picturebook-frame-7600.png`: successful seeded picturebook result
- `seed-dolphin-user/`: 152 KiB hash-pinned user-state prerequisite copied from the historical
  cached picturebook oracle
- `trace-summary.tsv`: trace counts and artifact hashes
- `manifests/`: runner manifests; they also record exact scripts and paths used during capture
- `visual-crop-proof/`: synthetic unequal-height fixtures, before/after command output, and the
  unequal-height Dolphin/PC screenshots from the passing end-to-end runner invocation

The seeded `GameData.bin` SHA-256 is
`5294589258e44368989627ed8e965887da349d3b9e4e0dfa96b0c9394739b895`.
