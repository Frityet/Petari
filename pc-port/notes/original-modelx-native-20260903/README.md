# Original ModelX and owned JUTTexture preparation

The complete original Mario J3DModelX now compiles and links in an isolated native probe, and renders the real Mario.bdl through original calc/material/view/entry and J3D packets. ModelX activation remains staged while the actual ModelManager/actor/render owner is assembled. This does not activate MarioAnimator or establish jumping/gameplay completion.

The production change in this tranche is the general owned-JUTTexture allocation and runtime-lifetime boundary. No Game algorithm is changed for this allocation work. The preceding real ResourceHolder/model/material/animation ownership checkpoint is `1f254b773`.

## Applied native texture ownership

`JutTextureAllocationService` installs the process's actual `Mem1ResourceHeap`. An owned JUTTexture receives one aligned mapped allocation containing its original ResTIMG header followed by its pixels. Native metadata associates that allocation with the actual JUTTexture identity outside the original class. A texture retains an independent heap lease after the allocation service is removed. New owned construction without a provider fails explicitly.

The owned constructor copies every original root assignment and call from `src/JSystem/JUtility/JUTTexture.cpp`, replacing only allocation with the mapped owner and adding an exception guard. The prior native width/height clamp is removed; zero width remains zero. Native `JUTTexture.hpp` already initializes `mFlag` to zero, so the original `mFlag & 2 | 1` assignment is defined. Borrowed TIMG construction retains the caller's storage; it never acquires or frees an owned allocation.

Destruction first removes GX texture-object state, then removes the owned image's copy-texture state. The mapped allocation drains queued GX CPU reads before returning its storage for reuse. Registry locks are not held during the drain. The original allocation identity is used even if the JUTTexture's currently bound image changes.

`GameResourceRuntime` installs the allocator after its actual MEM1 heap exists. RuntimeContext separately owns the actual texture constructed by unchanged CaptureScreenDirector, recovered through the director's public ResTIMG identity. The native screen-alpha provider's previous process-static texture array is now owned by a RuntimeContext ScreenAlphaCaptureService. That provider retains its existing MR surface and behavior. The **original Game ScreenAlphaCapture class remains inactive**; this change does not claim its source lifecycle has been imported.

The initial composed probe exposed why this ownership is necessary: the former static screen-alpha array destroyed a mapped JUTTexture after graphics/process shutdown and after the allocation registry's mutex lifetime. LLDB identified the exact static-array destructor. The explicit runtime owner removes that delayed destruction; the successful probe verifies full MEM1 capacity is restored before process/GX retirement.

Constructor-failure recovery is applied and tested. A first-declared runtime registration guard outlives every dependent owner and clears JUTVideo/WPAD/the published RuntimeContext only after member destruction. The scheduler is declared immediately afterward, so camera/scene NameObj destruction can still disconnect from a live scheduler. A common early-retirement helper runs in both the destructor and constructor body catch while trace/scheduler members are alive, retiring scene callbacks and actual capture textures in their existing order.

The focused actual RuntimeContext fixture passes real mapped OOM before the first frame, an injected configured-ILogger failure after both original CaptureScreenActor callbacks register, and two successful reconstructions. Each verifies null runtime/video/archive/screen-alpha ownership, retired NameObj identities, and restored MEM1 capacity. It passes without a mounted disc and with the real RMGK01 disc.

That before-first-frame test exposed a second concrete boundary: flushing initial GX register writes could access a render-pass array while no recording existed. Aurora recording now uses its existing window-size fallback when inactive and caches viewport/scissor changes without emitting pass commands. The next recording remaps and emits the retained logical state. No FIFO command is removed. The independent recording suite passes all 10 tests, including real queued viewport/scissor writes and drains before and between recordings followed by mapping into a doubled target. Its CPU-only fixture uses native viewport policy outside a frame; the actual RuntimeContext fixture covers fitted-policy fallback with a real SDL window.

## Staged ModelX source adaptation

`modelx-activation.patch` is activation-ready source preparation, not an applied build change. `modelx-manifest.json` records its exact baseline and source hashes. It contains:

- Root and identical PC J3DModelX source/header, with typed SDK GD includes and explicit enum conversions replacing obsolete `extern C GD(int, ...)` declarations.
- Four address fields typed as actual pointers: callback data `_128`, replacement model `_12C`, dynamic display list `_1B4`, and display-list pointer table `_1C4`. Corresponding pointer/integer casts and allocation types are corrected.
- Native little-endian bitfield order preserving all 29 retail raw flag masks. No flag meaning is changed.
- A TARGET_PC guard around the SDK base J3DModel destructor, whose native original provider already exists separately.
- Literal original ModelX draw helpers from MarioActorDraw in a separate compat TU, five original MR model/Xanime construction helpers, and three literal fog helpers. There are no fabricated owner or virtual methods.
- Typed root GD header declarations plus native revolution-to-dolphin header forwards.

Run `python3 pc-port/notes/original-modelx-native-20260903/verify-source.py`. It reconstructs the patch only under ignored `build/original-modelx-native-proof-20260903`, compiles the baseline and proposed source with GC3.0a3, and verifies all 20 functions plus the vtable: **7,236 identical bytes and 300 identical references**. Eleven extracted helper bodies also match root tokens exactly. This is before/after compiler equivalence, not a claim that the pre-existing ModelX constructor is a 100% retail decompilation match. Its existing reconstructed constructor is 3,608 compiled bytes versus 3,580 retail bytes.

The verified retail `viewCalc3` nonnull path remains unchanged. At `0x802A6B88` it stores incoming r5 at stack+8; at `0x802A6BCC` it passes that stack address to calcDrawMtx. Four current root call sites pass null. The isolated normal model path uses inherited viewCalc; nonnull native viewCalc3 semantics are not claimed supported by this probe.

## Real owner and call requirements

The original MR specialization is preserved; real Mario uses J3DModelX. The caller must retain its actual ResourceArchiveOwner lease and the original JKR allocation cohort for all model, buffer and display-list users. The original empty destructors do not independently free every allocation; the retained cohort supplies their lifetime.

Model construction needs actual initialized JUTVideo/CaptureScreenDirector and mapped screen texture, actual model resource data, original shape-packet userdata and SDK globals. `J3dCommandScope` encloses the unchanged constructor/helper calls because the original ModelX constructor leaves GDCurrentDL pointing into its stack frame. The native enclosing owner restores it. Retain the original scheduler/interrupt semantics and serialize model/global-state access; do not replace specialized construction with plain J3DModel.

Original extra view buffers are populated by later original Mario drawing initialization. They cannot be used immediately just because the constructor linked. The remaining actor integration must share one actual model through ModelManager, MarioAnimator and rendering; the staged probe does not reuse the independent parsed renderer's animation state.

## Validation artifacts

- `ModelXLayoutTests.cpp`: all 29 raw retail flag masks and four native pointer types; isolated native executable passes.
- `ModelXLiveProbe.cpp`: real-disc actual MR specialization, 30 joints / 9 materials / 9 shapes, all 16 original constructor display lists, holder identity, restored GD pointer, original calc/material/view/entry and teardown order.
- `live-result.txt`: final isolated probe exits 0; rendered frames report 11 original draws, and full MEM1 capacity is restored after runtime retirement. The first asynchronous statistics samples can include startup submissions; they are not used as frame correctness assertions.
- `mario-modelx.png`: inspected actual Mario bind pose from original packets. Fixture lighting is deliberately minimal; this is not a claim of retail visual parity or live animation.
- `JutTextureOwnershipTests.cpp` is installed in `pc-port/tests`; target `smg-pc-jut-texture-ownership-tests`. The isolated equivalent passes mapped/borrowed ownership, exact GD physical address encoding, zero dimensions, three capture/destruction/reuse cycles, enclosing-constructor unwinding, real allocation failure, and lease survival after provider retirement. `jut-texture-tests-result.txt` records its compact output.

Commands used for the isolated real-disc draw probe:

```sh
python3 build/original-modelx-native-20260903/live-build.py
SMGPC_REAL_DISC="/Users/frityet/Projects/petari/Super Mario Wii - Galaxy Adventure (Korea).rvz" \
  build/original-modelx-native-20260903/live \
  build/original-modelx-native-20260903/mario-modelx.png
```

The ignored scratch directory also retains exact compile/link command JSON, original-compiler objects, full logs and the LLDB shutdown diagnosis. Shared production builds/smokes are run serially by the parent after source freeze; their results are recorded in `../jut-texture-ownership-20260903/`. The initial shared mapped-texture target plus Title2/Gateway5 smokes passed before the final constructor guard; final guard results are recorded there after the parent's rebuild.

A separate pre-existing observation remains: `begin_recording` remaps command-cache viewport/scissor for a changed target but does not itself update `gx::g_gxState.renderViewport/renderScissor`, which shader code also reads. Draws that write viewport again receive updated state. A retained-viewport draw after resize without a new setter deserves a separate uniform-state regression; it is not changed by this lifecycle fix.
