# LodCtrl exact-source and real-or-absent evidence

Timestamp: 2026-08-07 05:00:22 UTC

## Result

`LodCtrl` now has one Game implementation. The missing retail `update()` routine was recovered into the root decomp source first, and the PC copy is byte-identical. The old partial `compat/LodCtrlCompat.cpp` implementation was removed. Host-only compilation and runtime support live outside `src/Game`.

The host implementation does not synthesize LOD models, view groups, shadow controllers, distances, or director state:

- Construction requires the active scene to own an explicitly created `SceneObj_ClippingDirector`.
- Middle and Low models are created only when the original `/ObjectData/<resource>Middle.arc` or `Low.arc` lookup succeeds.
- Model clipping bounds require parsed BDL/BMD joint bounds; missing model data rejects the operation.
- Joint and material animation synchronization shares the real source model's animation state.
- Unsupported view-group, clipping-group, clipping-list removal, and shadow ownership paths throw `std::logic_error`.
- The existing scene scheduler remains the generalized owner of camera-based actor clipping evaluation.

## Exact Game boundary

The following pairs were checked with both `cmp` and SHA-256:

| Root decomp | PC Game copy | SHA-256 |
| --- | --- | --- |
| `src/Game/LiveActor/LodCtrl.cpp` | `pc-port/src/Game/LiveActor/LodCtrl.cpp` | `3358532163a286f5e7a7809e7f00debbf0e2ad99441cc700e0247b85629c8457` |
| `include/Game/LiveActor/LodCtrl.hpp` | `pc-port/src/Game/LiveActor/LodCtrl.hpp` | `4c5c22bcc5c7962d597468875a6168a1a4eafc367ce300d7886e2dfd980af01c` |
| `include/Game/LiveActor/ClippingDirector.hpp` | `pc-port/src/Game/LiveActor/ClippingDirector.hpp` | `2e733da1ac3c2ded98ff4a9c888ce4ae87d6d6c1ab8a58beec518fd847a4074e` |

`pc-port/src/Game/xmake.lua` excludes the exact LodCtrl translation unit from direct GCC compilation. `pc-port/src/compat/LodCtrlSource.cpp` includes it and contains only host compiler/API adaptation: the Metrowerks trailing `NO_INLINE` spelling and the host model's real base-matrix accessor. It does not replace any LodCtrl behavior.

## Recovery evidence

- Retail symbol: `update__7LodCtrlFv`, address `0x80166f08`, size `0x1a4` in the RMGK01 symbol map.
- The routine was recovered from `build/RMGK02/main.elf` with `build/binutils/powerpc-eabi-objdump`.
- `ninja build/RMGK02/src/Game/LiveActor/LodCtrl.o` reports the RMGK02 decomp object up to date.
- `powerpc-eabi-objdump -t build/RMGK02/src/Game/LiveActor/LodCtrl.o` contains `update__7LodCtrlFv` as a compiled global function.

## Host support boundary

- `compat/ClippingDirectorCompat.cpp`: real scene-owned director lookup and explicit rejection for unavailable retail holder tables.
- `compat/SceneObjHolderCompat.cpp`: creates `ClippingDirector` only for explicit `SceneObj_ClippingDirector` creation.
- `compat/LodCtrlRuntimeCompat.cpp`: original utility semantics backed by real actor/model state, with explicit rejection when a real subsystem is absent.
- `render/J3dModelRenderer.*`: bounding radius from parsed joint min/max values and evaluated joint matrices.
- `render/live_actor/LiveActorModel.*`: shared real BCK/BTK/BRK/BTP source state for LOD models.

The retail `ClippingActorInfo` constructor values used by actor registration are the recovered values (`300.0f` sphere and far level `6`), not host-selected tuning values.

## Verification

Successful commands and results:

```text
xmake -vD smg-pc
  build/linux/x86_64/debug/smg-pc linked successfully

xmake run smg-pc-lod-ctrl-real-or-absent-tests
  4 LodCtrl real-or-absent test(s) passed

xmake build smg-pc-sceneobj-holder-real-or-absent-tests
build/linux/x86_64/debug/smg-pc-sceneobj-holder-real-or-absent-tests
  3 SceneObjHolder real-or-absent test(s) passed

xmake build smg-pc-live-actor-util-real-or-absent-tests
build/linux/x86_64/debug/smg-pc-live-actor-util-real-or-absent-tests
  LiveActorUtil real-or-absent tests passed: 3/3

xmake build smg-pc-btp-real-resource-tests
build/linux/x86_64/debug/smg-pc-btp-real-resource-tests
  BTP real-resource tests passed: 4/4

ninja build/RMGK02/src/Game/LiveActor/LodCtrl.o
  ninja: no work to do (object is newer than the changed source)

git diff --check -- <LodCtrl task files>
  no errors
```

The focused LodCtrl suite proves exact source identity, rejects missing scene/director ownership, executes the recovered high-only update path against actual actor flags, refuses a missing Low archive, and rejects unavailable shadow ownership instead of reporting success.
