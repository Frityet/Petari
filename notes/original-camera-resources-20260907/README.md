# Original camera resource leaves — 2026-09-07

Activated the complete original `CameraParamString`, `CameraParamChunkID`, and `DotCamParams` translation units in the normal native Game archive, plus the missing original ID/reader headers. All three native sources and headers are byte-identical to their reference counterparts. The existing CameraParamString header was already identical. No reduced CameraHolder or new camera selector was introduced, and the parent retains the Game build-list lane.

The one reference recovery is `DotCamReaderInBin::hasMoreChunk`: use the actual `JMapInfoIter` validity and `JMapInfo::end()` comparison. This recovers **100% of the retail 184-byte function**, removes the old hand-expanded raw `JMapData*` assumption, and allows both Wii storage and the native retained JMap resource representation to use their existing iterator contract. The code was saved in decomp first and mirrored without native divergence.

## Evidence

- `native-syntax-results.json`: all three production TUs and the focused test compile with Homebrew LLVM 23/macOS arm64 production flags.
- `wii-validation-commands.json` / `wii-function-proof.json`: fresh baseline/current Wii compiles and per-symbol comparison with the verified retail split. All 16 CameraParamString/CameraParamChunkID methods are exact. DotCam has eight exact methods, including the recovered iterator query, and its vector reader is 99.7%. No paired symbol regressed.
- Two inherited DotCam differences remain: `getValueInt` inlines the correct `JMapInfoIter::getValue<s32>` implementation instead of the retail tail call (100 compiled PPC bytes vs 8 retail; objdiff 0%), and `init` has fewer temporary-iterator stack stores (108 vs 144 bytes, 65.06%). Retail inspection confirms the same signed getter delegation and attach/version-row-zero/iterator-row-zero operations. These are not claimed as exact PPC matches.
- Retail inputs are under `decomp/build/original-player-state-recovery-20260907/retail/obj/Game/Camera/`; the split uses DOL SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`. Complete commands retain the exact compiler/flags/object paths.

## Focused native regression

`smg-pc-original-camera-resource-tests` links the actual normal Game archive; no duplicate resource implementation or fake camera callback is supplied. The fixture covers:

- Borrowed parameter pointer identity, assignment, null/empty normalization, and source changes.
- Original cube/start/group/other/event ID formatting; signed s8 zone narrowing; unnamed sentinel ordering; zone-first then case-sensitive lexical comparison; bounded temporary buffers; deep persistent copies; and allocations into the actual selected JKR scene heap.
- Synthetic bounded big-endian BCSV with an unaligned source identity, signed 32/16/8-bit values, packed mask/shift, floats, inline and offset strings, exact row order, first-row version, end/empty iteration, missing-field preservation, and atomic three-component output when a vector component is absent.
- An actual heap-allocated DotCam reader with an embedded native JMap disposer, retained raw-resource/string lifetime after unpublication, host cache allocation outside Game routing, and complete typed destruction before the scene arena retires.
- Optional real RVZ CameraParam.bcam catalogs from HeavensDoorGalaxy and EggStarGalaxy: every row is traversed using the original reader, with present camera scalar/string/vector fields compared to the production bounded binary decoder. The synthetic cases independently fix the raw expected field values.

Run `xmake build smg-pc-original-camera-resource-tests`, then `xmake run smg-pc-original-camera-resource-tests`. Set `SMGPC_REAL_DISC` to the real game image to enable the authored-catalog case. `initial-player-blocked-build.log` preserves an initial unrelated failure on the parent-owned Teresa pointer-width stores, corrected by the parent before the retry.

## Runtime result

The normal archive build, synthetic-only run, and real-RVZ run all exit **0**. `build.log`, `synthetic-run.log`, `real-run.log`, and `native-tests.json` retain the results and exact commands/environment. The five deterministic cases pass in both runs; the optional archived-catalog case is explicitly skipped without the image.

| Actual CameraParam.bcam catalog | Rows | Scalar/string reads | Complete vector reads | Version |
| --- | ---: | ---: | ---: | ---: |
| HeavensDoorGalaxy | 42 | 798 | 210 | 196630 |
| EggStarGalaxy | 111 | 2109 | 555 | 196630 |

`source-manifest.json` records final native/reference equality and hashes. The root build lane was explicitly handed to the backend owner after both runs completed; no commit or staging was performed by this child.

This cohort enables original resource parsing/identity ownership; full original camera manager/controller activation remains a separate integration task.
