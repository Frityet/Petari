# FileSelectSky / MaterialCtrl exact-source boundary

Recorded 2026-08-08 UTC against the RMGK02 configuration.

## Outcome

- `FileSelectSky.hpp/.cpp` remain byte-identical between the retail decomp tree and `pc-port/src/Game`.
- The recovered `ProjmapEffectMtxSetter` declaration and implementation in `MaterialCtrl.hpp/.cpp` are byte-identical between the two trees.
- `LiveActorUtil.hpp`, `Animation/AnmPlayer.hpp`, and `ModelUtil.hpp` are also byte-identical root-to-PC. Host-only matrix and projection ownership live under `pc-port/src/compat`.
- Exact `FileSelectSky.cpp` is compiled by the Clang exact-source boundary target.
- `FileSelectSky.cpp` and `MaterialCtrl.cpp` remain excluded from the production `smg-pc-game` source list. No factory registration or runtime route was activated.
- The host projection provider requires an actual registered `LiveActorModel`, rejects invalid transforms, owns controllers per actor, and applies the real projection-effect matrix through the existing J3D material renderer. Raw SDK `J3DModel` construction is explicitly unavailable rather than emulated.

## Verification

```text
ninja build/RMGK02/src/Game/LiveActor/MaterialCtrl.o build/RMGK02/src/Game/Map/FileSelectSky.o
PASS

ninja
PASS; build/RMGK02/main.dol: OK
SHA-1 54b71431af0d509097bfdef4ec28617afc487e89

cd pc-port
xmake build -j 1 smg-pc-game
PASS

xmake build -j 1 smg-pc-file-select-exact-source-compile
PASS (includes exact FileSelectSky.cpp)

xmake run smg-pc-file-select-exact-source-compile-tests
PASS: exact File Select source compile boundary passed

xmake run smg-pc-live-actor-util-real-or-absent-tests
PASS: LiveActorUtil real-or-absent tests passed: 5/5, including real-model
projection-controller binding, inverse transform/local-offset composition, and
explicit absence without a model renderer.
```

`objdiff-summary.json` records the focused RMGK02 metrics from the regenerated full report. `mirror-sha256.txt` records the exact-pair digests.

## Honest remaining boundary

The full retail `MaterialCtrl` unit also contains unrecovered Fog, view-projmap, mirror-reflection, Mario-shadow, and texture-matrix controller implementations. Consequently the exact `MaterialCtrl.cpp` is source-present but deliberately not compiled into the production PC library. The recovered `ProjmapEffectMtxSetter` slice needed by `FileSelectSky` is high-confidence and its principal methods range from 92.833336% to 100% fuzzy match.
