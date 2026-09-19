# GX destination-alpha and texture-copy boundary — 2026-09-19

## Source contract

- `decomp/src/RVL_SDK/gx/GXFrameBuf.c:339–402`: `GXCopyTex` emits the copy command and its explicit clear flag. It does not first overwrite EFB alpha.
- `dolphin/Source/Core/VideoBackends/Software/SWEfbInterface.cpp:420–449`: blending uses the fragment color/alpha, then enabled destination alpha replaces the stored fragment alpha; color and alpha write masks remain independent.
- `dolphin/Source/Core/VideoCommon/PixelShaderGen.cpp:170–174`: destination-alpha override is active only with enabled destination alpha, alpha writes, and RGBA6 EFB format.

The inherited Aurora copy path queued an alpha-only EFB clear before every destination-alpha-enabled texture copy, including no-clear copies. Its draw path also used `source_alpha * blend_constant_alpha` instead of replacing stored alpha. Copy-time repair both concealed that draw bug and changed previously drawn pixels globally.

## Bounded correction

Removed the copy-time repair; preserved copy-clear color/alpha and write-mask handling. Uses dual-source fragment outputs for effective destination-alpha draws: RGB blending consumes original TEV alpha from the secondary source; primary alpha is one, multiplied by the existing per-draw blend constant. Alpha testing still consumes the original TEV alpha before output. Ordinary shaders do not request dual-source output. Requests the optional GPU feature when available and fails explicitly if a draw requires it on an unsupported device. No actor, scene, layout or stage condition is involved.

## Pixel proof

New `aurora/tests/gx_copy_alpha_render_test.cpp` draws into a real EFB and reads actual `GXCopyTex` GPU textures. It covers two regions with different alpha, state changes before no-clear copies, retained EFB content, independent true-clear alpha, partial/zero TEV alpha, both RGB source-alpha factors, alpha-test rejection and disabled alpha writes. Optional `--copy-only`/`--draw-only` modes isolate the two pre-fix failures. This is renderer validation, not a claim that a Gateway screenshot symptom has been reproduced or fixed.

## Validation

Real Metal pixel red baseline, before any production change:

- `--copy-only` failed: expected copied left `rgba(252,0,0,84)`, observed `rgba(252,0,0,40)`. A later destination-alpha state change rewrote prior content during a no-clear copy.
- `--draw-only` failed: expected `rgba(84,0,171,168)`, observed `rgba(84,0,171,55)`. RGB already used TEV alpha correctly; stored alpha was incorrectly multiplied by it.

After the correction:

- New pixel regression **PASS**, including no-clear preservation of two distinct alpha regions, true-clear alpha, partial/zero TEV alpha, both source-alpha blend factors, unblended overrides, alpha-test rejection, disabled alpha writes, and alpha-only writes preserving RGB.
- Existing Z-texture GPU proof **PASS**, with an added case combining dual-source output and actual fragment depth.
- Existing copy-filter and clipping GPU proofs **PASS**. Four renderer tests total, 5.60 seconds, executed sequentially on Metal.
- Complete GX FIFO suite **333/333 PASS**, 1.619 seconds.
- `git diff --check` **PASS**.

The cached CMake build initially mixed its detected CommandLineTools SDK with later Xcode SDK include paths. The first two build logs retain that setup failure. Only this untracked cache was reconfigured to use `/Library/Developer/CommandLineTools/SDKs/MacOSX26.sdk` consistently for the sysroot and SQLite/BZip2/Zlib includes. No production header-order workaround was added. The test's readback explicitly synchronizes Aurora's render worker before queue submission; this avoids sampling a texture before its recorded copy has been submitted.

Commands (from repository root):

```sh
cmake --build build/aurora-fifo-tests-20260919 --target gx_copy_alpha_render_test gx_fifo_tests gx_copy_filter_render_test gx_z_texture_render_test gx_clip_mode_render_test -j 8
build/aurora-fifo-tests-20260919/tests/gx_fifo_tests
ctest --test-dir build/aurora-fifo-tests-20260919 -R '^gx_(copy_alpha_render|copy_filter_render|z_texture_render|clip_mode_render)$' --output-on-failure -j 1
```

Limits: Metal was exercised on this host; Vulkan and unsupported adapters were not. A GPU without optional `DualSourceBlending` still supports ordinary single-source shaders, but an effective RGBA6 destination-alpha draw fails explicitly rather than silently substituting pixels. The guard remains active in release builds. Pipeline cache version 16 invalidates previous GX pipeline configurations. Native main integration and any Gateway visual comparison are owned by the parent task, and are not claimed by this renderer proof.

## Publication

Aurora commit `8c19ab45d233eb43d0c817256e8a348cc98746cf` was pushed to `origin/codex/macos-compat`; `git ls-remote` independently returned the same SHA. Aurora working tree is clean. An initially created unused `pcp-aurora` branch was immediately removed; the existing tracked branch is the published destination. Parent gitlink update remains with the parent task.
