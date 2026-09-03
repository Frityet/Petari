# Regular native build integration

Applied the reviewed JPC loader, original SDK draw/field/emitter routines and
shared Gekko arithmetic helper to the production source tree. Nineteen missing
SDK headers are literal root imports; their hashes are recorded separately.
The current geometry header is preserved. The original JPA source files and
extracted Game/System/Overwrite methods are compiled once, with floating-point
contraction disabled to preserve their separate scalar arithmetic steps.

Regular macOS ARM64 LLVM23 targets now build and pass against SMGPC_REAL_DISC:

* smg-pc-original-jpa-manager-tests: 3,327 resources, 225 textures, 45,386
  original function entries, 52,571 calculation frames and 352,896 particle
  observations; original pools and heap retirement pass.
* smg-pc-original-scenario-catalog-tests: all 48 archives and 234 authored zone
  rows, two construction/retirement cycles and allocation-failure unwind.
* smg-pc-scenario-publication-tests: the actual parser identity is published,
  original GalaxyStatusAccessor reaches it, shared retention preserves it, and
  missing/duplicate owners plus constructor failure retire correctly.

The parser's one original GameSystem lookup is selected from a native service
accessor; its remaining algorithms are original. Archive ownership now belongs
to RuntimeContext and retains raw JMap resource identities. The former native
FileUtil implementation is moved out of Game to compat and its process-static
mount map is removed. The real scenario owner is available to the forthcoming
camera/scene transaction; this checkpoint does not yet start it in RuntimeContext.

The separate full-manager sanitizer fixture passes cleanly; its scope and
prebuilt dependency limitation remain documented in README.md. Regular target
logs here are unsanitized. Draw callbacks are linked but not GPU-executed.

The showcase was also rebuilt and still stops at the existing MarioEffect
frontier: emitEffectWithEmitterCallBack/setEffectHostSRT are undeclared and the
old native emitEffect API returns void instead of the original MultiEmitter*.
That API must be replaced with the actual EffectSystem ownership graph. No
placeholder or return-value workaround is added. Jumping and complete gameplay
are not active yet.

Aurora arithmetic commit: 3918c972a4f8f67e0ab78ce6f6a25e8402258ee9,
pushed and independently verified on origin/codex/macos-compat.
