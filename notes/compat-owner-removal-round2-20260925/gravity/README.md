# Gravity owner restoration and unused camera deletion

Restored all 17 public GravityUtil functions from the current decomp source, including its parameter constants and PlanetGravity::getDistant getter. Required native failure checks from GameGravityCompat now live at that actual Game utility owner: missing scene/manager, null LiveActor or gravity, and duplicate gravity registration remain explicit errors. Position-only queries still permit a null requesting object and use the real manager. Existing field selection, strength, priority, type-mask and zero-vector behavior remain original.

The gravity requester identity is now `uintptr_t` end to end, from all eleven public query signatures through the shared query helper and PlanetGravityManager::calcTotalGravityVector comparison. Previously the utility and manager each truncated pointers to `u32`, allowing unrelated native objects with the same low address bits to exclude each other's fields. No object identities or geometry are synthesized. Native-only constrained null overloads in GravityUtil.hpp preserve original `nullptr` host call spelling without competing with integer zero.

GameGravityCompat also held four original LiveActorUtil methods. Since the complete LiveActorUtil.cpp remains excluded, the exact contiguous donor methods now compile in Game/Util/LiveActorUtilGravity.cpp. This focused owner split is approved by root; remove the split only when the complete owner is enabled. No duplicate compiled provider remains.

Deleted the GameGravityCompat cpp/hpp pair. Deleted MarioCameraTarget cpp/hpp (both exported helpers had no consumers) and CameraUtilCompat cpp/hpp (the namespace free helper had no callers; similarly named calls are unrelated CameraSystemService member methods). Removed their exact includes from MetrowerksStdCompat and four tests; all dirty test baselines were saved before editing.

## Regression and validation

The existing GravityRealOrAbsent real-manager case now verifies exact full-width explicit identity, two tokens sharing low 32 bits, the implicit requesting object's full pointer, and explicit host override. This case is included by the target's `--queries-only` selection. The alternate token is used only for opaque equality and never dereferenced. Existing tests continue covering absent-manager errors, null registration/actor, duplicate registration, authored JMap settings, field blending and shadow selection.

Static verification retains all 17 donor public functions and proves the four LiveActorUtil methods are byte-identical to the donor block. Source/test searches find no remaining deleted helper references. Diff whitespace check passes. No build or test was run by this agent; root owns integrated validation.

## Build wiring handed to root

- Remove `remove_files("Util/GravityUtil.cpp")` in src/Game/xmake.lua.
- Keep the full LiveActorUtil.cpp exclusion; the Game wildcard picks up LiveActorUtilGravity.cpp.
- Remove the PlanetGravityManager.cpp special `-fms-extensions`/`-fpermissive` block, because it no longer narrows a pointer to u32.
- The compat wildcard naturally drops the six deleted files.

The before directory, manifest and patch preserve the exact dirty baseline. No GlobalGravityOwnership, registry, scene, ActorSensor, build, Git index or commit modifications were made by this agent.
