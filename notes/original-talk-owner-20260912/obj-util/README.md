# Original ObjUtil and ActorMovementUtil consolidation, 2026-09-12

## Result

The native `Game/Util/ObjUtil.cpp` now contains the complete available reference TU instead of its 171-line host substitute. The only native source include correction is `<cstdarg>` for the native variadic ABI; the native header retains its previous SceneFunction include because native consumers use those category declarations transitively. Canonical ClippingJudge, BenefitItemObj/LifeUp, THexahedron3 and TPartition3 headers supply missing real types.

`Game/Util/ActorMovementUtil.cpp` was already byte-identical to reference and compiles as a whole. Its original actor-offset water/death, reset, direction, matrix and velocity methods now replace the extracted and substituted compatibility methods. Root owns removing its Xmake exclusion. The actor source itself was not changed.

Seven complete compatibility files were removed: OriginalCsvReader, OriginalPreDrawRegistration, OriginalArchiveResourceQueries, OriginalSceneConnections, OriginalActorMovement, OriginalSensorGeometry and MarioCameraAccessCompat. Fourteen surviving providers lose their exact duplicate methods. The full manifest is `removed-providers.json`; it includes the three actor matrix overloads removed from MtxCompat after coordinating with its owner, and the synthetic NPC faceToVector body replaced by the actual ActorMovement implementation. Unrelated providers in those files remain intact. Private helpers that became unused were removed too.

Original ObjUtil now obtains resource tables from ResourceHolderManager, pre-draw registration from GameSystem's scene controller, StageStateKeeper from EventFunction and shake/rumble from the original owners. The Game-local filename probing and the scene-phase substitute no longer answer these queries. Original joinToNameObjGroup retains its required-group precondition; no absent group is created automatically. Original `initStarPieceGetCSSound` null check and rumble pattern names were preserved from the current reference.

## Reference recovery

The four previously missing CSV methods were recovered first in `decomp/src/Game/Util/ObjUtil.cpp` and then mirrored. `getCsvDataF32` and Bool use the original JMap typed reader, which preserves the caller's output if the field does not exist. Vec formats X/Y/Z suffixes into the original 256-byte buffer. Color formats R/G/B/A and calls getCsvDataU8, preserving its zero default and eight-bit truncation. The suffix bytes were read directly from retail DOL address 0x806B2680; see `retail-csv-formats.txt`.

Retail calls getCsvDataU8 rather than inlining it in the Color method. The existing reference convention `#pragma dont_inline on/reset` around that existing helper preserves those calls. Its own code remains 100% matched. All new methods compile under the original compiler: F32 100%, Bool 100%, Vec 99.6875%, Color 99.66102%. Commands, original object diff and per-method summary are adjacent. No configure status was changed.

## Validation

All 16 affected native translation units compile with the actual Game target flags (including its shared Metrowerks header). `native-compile-results.json` records each result. Native probes without that target's force-include first exposed the expected undeclared Metrowerks math builtins; the final commands reproduce the real target and all pass.

An isolated notes-only harness includes and calls the existing `test_original_csv_reader` from OriginalResourceHolderTests. It links the newly restored full ObjUtil object before the current Game/Aurora archives and runs the actual retained ResourceArchiveOwner/JMap fixture. It passes (exit 0): variadic archive table lookup, optional absence, original string handling, fractional/negative float values, missing-field defaults, masked booleans, XYZ fields, byte truncation and retained source lifetime. See `csv-probe-compile-command.json`, `csv-probe-link-command.json` and `csv-probe-run.log`. This validates the restored original CSV behavior without constructing artificial GameSystem or ResourceHolderManager instances. It is not evidence of full Talk or Gateway gameplay.

Exact MR symbol comparison against the current Game archive, excluding retired/recompiled providers and including the fresh objects, finds no remaining duplicate MR definition for either TU. `ObjUtil-symbols.json` and `ActorMovementUtil-symbols.json` list remaining external references absent from that archive snapshot; SDK/runtime typeinfo entries are not Game defects.

## Remaining owner and provider requirements

Original archive and pre-draw entrypoints require actual ResourceHolderManager and GameSystem startup. RuntimeContext's host resource/scheduler services are not those singleton roots. The previous wrappers had bypassed this distinction; they are not reinstated.

Original water queries already flow through the compiled AreaObjUtil and WaterAreaHolder sources; an absent ocean holder returns false according to its original logic after normal Water-area lookup. Actual clipping-judge methods and BenefitItem owner methods are not yet present. Additional unused original StarPieceDirector/PowerStar model helpers also remain unresolved in the archive snapshot.

ActorMovement exposes the still-missing shared math operations, notably MathUtil::turnQuatZDirRad used by original faceToVector; also the matrix faceToVector overload, rotateVecRadian, near-angle and turn-vector helpers, plus ground-area/rail/shadow queries. These need their original owners/source cohorts, not a return to the removed actor-specific substitute. Root was notified before integrated activation. Native compilation and the passing CSV fixture do not claim closure of all these methods.

No shared Xmake changes or commits were made in this lane. Source hashes and deletions are in `source-manifest.json`.
