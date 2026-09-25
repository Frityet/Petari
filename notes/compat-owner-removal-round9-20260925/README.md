# Restore original actor, shadow, map and image-effect owners

Removed **31 more compat files**, reducing `src/compat/` from 129 to **98 files**. The full compat removal and gameplay goal remain active. Previous turn was progress: published 1191258ac with 31 removals, a passing build and short opening run.

- Restored complete LiveActorUtil, ActorShadowUtil, ActorShadowLocalUtil and MapUtil. Retained necessary native resource decoding, lifetime and bounded-array checks at the original owners. Removed duplicate shadow metadata and construction state from ActorRuntimeRegistry.
- Original LiveActor/ShadowControllerList/ShadowController now own and retire CSV storage, drawers and filters. Borrowed line endpoints are invalidated on controller retirement.
- Actual image-effect classes own their shared/private textures, states and arrays. Removed synthetic scene capture/retirement and JUT construction-capture services; SDK heap finalizers remain.
- Imported complete existing donor Flag, SwingRopePoint and four missing shadow shapes. Restored original OceanHomeMapCtrl, replacing the name-specific refusal. Restored original TriangleFilterDangerCode and JMapInfo::begin required by the full utility sources.
- Moved J3D loader/material-factory behavior into canonical SDK owners. Replaced NativeJkrDisposer with identity-safe copy/move behavior on actual JKRDisposer. Binder's native destructor now lives with Binder.
- Retired obsolete shadow metadata fixtures/cases and migrated necessary callers to original factories/fields. Updated 22 provider records to their canonical paths.

## Reduced verification

`xmake build smg-pc` passed. One fresh-save real-disc Metal opening run completed **120 frames and exited 0** in 2.9 seconds, with no surviving process. Binary SHA-256: `f35b90d484f63035177558d85a173b8948cb86e018b360cba09f7048577c19fa`. See `validation.json`, `build.log`, and `gateway-120.json`/`.log`.

Per the user's faster iteration request, no focused test suite, extended gameplay run, new fixture campaign or broad audit was run. This is bounded opening evidence, not proof of Rosalina/full Gateway completion. Unrelated dirty work remains, so it is not a clean-checkout attestation.

## Preservation

Staged through a separate temporary index. The existing six staged route-note changes remain byte-for-byte unchanged. Existing dirty source/test changes are preserved; only this batch's target removal is staged from tests/xmake.lua. Snapshots and detailed ownership notes exist per lane; published notes exclude before-copy trees and NAND data.
