# Next J3D closure assessment (read only)

Recommendation for the next checkpoint, after round-four commit: restore complete `JSystem/J3DGraphAnimator/J3DAnimation.cpp`, `JSystem/J3DGraphBase/J3DDrawBuffer.cpp`, `JSystem/J3DGraphBase/J3DStruct.cpp` and `JSystem/J3DGraphBase/J3DGD.cpp`. This is an initial dependency assessment, not a completed implementation/function audit. No source changes made for this proposal.

Potential eight compat-file deletions:

- `J3DFrameCtrlCompat.cpp`
- `J3DTransformAnimationCompat.cpp`
- `J3DMaterialAnimationCompat.cpp`
- `J3DAdditionalAnimationCompat.cpp`
- `J3DAnimationInterpolation.hpp`
- `J3DDrawBufferCompat.cpp`
- `J3DStructCompat.cpp`
- `J3DGDCompat.cpp`

## Animation complete owner

The four sampler/frame providers all belong to the single complete 1,378-line donor `decomp/src/JSystem/J3DGraphAnimator/J3DAnimation.cpp`. Restore its full algorithms/function set rather than concatenating partial fragments. The interpolation header currently has only the three sampler providers as consumers, so its original template and overloads belong inside that complete TU. Native PPC integer conversion, low-halfword narrowing and shift behavior must remain calls to existing `aurora::ppc` primitives. Preserve the signed-16 PSQ Hermite scalar translation's exact explicit FMA sequence and disabled implicit contraction; retain BCA's unfused interpolation and original rotation behavior. Current native out-of-line destructor declarations must remain satisfied even where the donor relies on inline emission. Inspect donor `J3DAnimation_FORCE_EMIT` and native material-attach/header dependencies during implementation.

Relevant existing coverage: `OriginalJ3DTransformAnimationTests`, `OriginalMaterialAnimationTests`, `OriginalJ3DAnimationResourceTests`, `J3DFrameCtrlTests`, `BtpRealResourceTests`, `OriginalXanimeCoreTests` and `OriginalXanimePlayerTests`. The independent `FixedStepClockTests` target directly compiles `../src/compat/J3DFrameCtrlCompat.cpp`; root must replace that direct shard wiring with an appropriate canonical link/source arrangement when the whole TU is imported.

## Draw buffer and structures

The current DrawBuffer provider differs from its complete donor only in null/cast spelling and obsolete Metrowerks pragmas. The current donor already performs pointer hashing through `uintptr_t`, so no pointer truncation needs reintroduction. Preserve the original six sort strategies and both draw orders.

The Struct donor needs explicit native branches for paired-single copy operations and the 3x4-to-4x4 affine final row. Existing native scalar assignment implementations preserve the semantic fields and should be carried through as architecture adaptation. Do not copy the donor's empty non-Metrowerks `setEffectMtx` path. Existing material/texture matrix tests exercise these records; inspect whether overlapping/in-place copy coverage is sufficient before adding tests.

## GD

The complete donor is already very close to current native behavior. Differences are Aurora Dolphin GD include spelling, an equivalent signed scale-exponent cast and `GXBool`/`u8` signature spelling. The current compat TU additionally emits `J3DGDWriteXFCmdHdr` and `J3DGDWriteXFCmd`; review the canonical header's intended inline ownership and preserve its big-endian command-byte writer semantics when restoring those. Do not duplicate definitions. Existing display-list/material draw coverage should run after import.

## Defer animation loader and model loader

`J3DAnmLoaderCompat.cpp` looks close to original loader algorithms, but its public load body is exported as `smgpc::resource::detail::load_native_animation` and block access goes through `first_animation_block`. `resource/J3dAnimationResource` owns native decoding, pointer width, retained storage and the public wrapper. Removing that provider coherently requires auditing both halves and moving the original complete loader TU to its actual owner with an explicit native decoding boundary; merely renaming the current partial provider is insufficient.

The analogous model loader/factory resource boundary and remaining `OriginalJ3dJointTree` preview renderer ownership are larger independent closures. Keep them out of the sampler/draw-buffer batch unless a required symbol demonstrates a real dependency.
