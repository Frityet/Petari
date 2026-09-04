# Original MultiEmitter callback native fixture

Frozen native proposal only; no production Game changes, shared xmake build, or GPU run. Root callback recovery is already committed as `48e8d2ce8`; this package executes that recovered code on actual native particle objects.

## What ran

`python3 pc-port/notes/original-multi-emitter-callback-native-20260903/verify-native.py`

The script stages byte-identical root `MultiEmitter`, `MultiEmitterCallBack`, `SingleEmitter`, `MultiEmitterParticleCallBack`, and `AutoEffectInfo` source/header pairs. It compiles **complete translation units**, then links with dead stripping so uncalled methods requiring the not-yet-owned `EffectSystem` do not require substitute providers. The only new compatibility extracts are complete original `MR::Effect::isEffect2D`, `MR::hasStringSpace`, and `MR::isDigitStringTail` bodies. The two string methods exist in the excluded native StringUtil source but have no provider in the current linked Game archive.

The resource owner/query package is staged identically to the now-active original particle owner. `ParticleResourceHolder` is compiled from current production. All five Game TUs, the owner/query code, holder, fixture, and actual SDK `JPAEmitterManager`, `JPAEmitter`, and `JPAMath` sources are instrumented with `-fsanitize=address,undefined -fno-omit-frame-pointer`; remaining linked libraries are existing ordinary debug builds. Both sanitizers run with `halt_on_error=1`. Production JGeometry headers and current Aurora PPC conversion helpers are used directly: no older TVec overlay.

The actual supplied RMGK01 RVZ is mandatory. The fixture loads `/ParticleData/Effect.arc` through the real DVD/archive owner, reads the original particle-name table, and selects an actual name ending in two decimal digits (`2PGlowActiveLoop00`, resource ID 0). The full original MultiEmitter constructor creates one genuine SingleEmitter using that resource ID. A real JPAEmitterManager creates its JPABaseEmitter from the owned JPA resource and retains the actual MultiEmitter callback pointers. Actual AutoEffectInfo construction and `init` parse the first authored metadata record; focused checks then change that test-owned metadata's public fields. No fake emitter, resource holder, EffectSystem, or GameSystem is constructed.

## Results

Two complete resource-owner/scene-cohort cycles passed. Per cycle:

- 524,288 flag cases cover every low-seven authored metadata and callback-force bit combination, all eight individual host-component availability combinations, matrix presence, initialization versus subsequent follow, and off/reset priority. Explicit force API calls and all draw-order values -2 through 11 are checked too.
- 256 channel-distinct color compositions verify integer truncation, alpha preservation, direct colors when light influence is zero, and the original reuse of **old primary RGB** for environment modulation. Prior environment RGB intentionally differs so using the wrong source fails.
- Five signed-angle cases compare the actual callback/emitter rotation with an independent double-precision multiplication of elementary axis matrices, after independently deriving the original signed16 truncation and 14-bit table index. This tests fractional negative angles and rotations past a full turn without substituting native Euler math into the callback.
- SRT checks cover rotation-before-componentwise-scale of the offset, host translation, retained live host changes, dynamics/particle scale, base scale during initialization, and no update when authored follow is disabled.
- Matrix checks cover nonuniform scale decomposition, offset transformation, a distinct explicit scale pointer, live matrix translation changes, the extra Z rotation for both 2D draw orders, and force-off pose preservation.
- Original JPA force deletion retires live emitters before callback destruction and scene-cohort disposal. The particle resource owner remains alive throughout. Both scene removal and resource removal restore the original root heap free size; publication/mount counts return to zero and a fresh owner succeeds.

`probe-asan.log` records the complete output, with no sanitizer report. `runtime-proof.json` records environment and exit status; `native-compiles.json` records exact isolated compile/link commands. No claim is made about uninstrumented library internals, GPU particle rendering, or full EffectSystem/MarioEffect activation.

## Source and ownership boundaries

- Complete staged source/header pairs are byte-identical to root. `source-manifest.json` records original/staged paths and hashes; `native-proposal.json` checks equality and lists the exact native destinations. `native.patch` contains only the five source/header pairs and three literal query bodies in two compatibility files. It does not duplicate the already-active particle owner/query files.
- `MultiEmitterCallBack` names are historical: member `mScale` at Wii +8 actually points to position, `mRotation` to degrees, and `mTranslation` at +0x10 to scale. Tests follow the verified operands without renaming Game fields.
- Retail `followSRT` leaves its local FlagSRT uninitialized if metadata is absent. The fixture supplies actual initialized metadata for init/execute. The null-metadata query is checked only through `isFollowSRT` with explicit initialized input flags, preserving rather than masking that source precondition.
- Integer Color8 construction in the current native header still needs the previously staged generic endian correction (`../original-auto-effect-registration-20260903/color-native.patch`). These callback checks use distinct channel construction and do not claim validation of authored packed-color parsing. The unrelated known boundary is not silently patched by this package.
- Actual original EffectSystem ownership, createSingleEmitter/scan lifecycle, SyncBck scheduling, scene draw registration, and callback GPU rendering remain separate activation work. Direct callback execution here verifies original callback behavior on real objects; it is not an end-to-end particle-system claim.

The staged files are under `build/original-multi-emitter-callback-native-20260903/staged/`. The fixture source is `probe.cpp`; it is suitable for a dedicated test target once the complete original effect owner is activated. The frozen patch is intentionally not applied here.
