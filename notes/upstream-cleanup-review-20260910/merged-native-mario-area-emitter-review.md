# Merged native Mario, AreaObjUtil, and ParticleEmitterHolder review

Read-only source review, 2026-09-10. No concrete semantic merge regression or lost native ABI requirement found in this cohort. Build and runtime validation belong to the parent task; this review does not claim final linked-provider proof.

## Reference identity

| Native source | SHA-256 | Result |
| --- | --- | --- |
| src/Game/Player/Mario.cpp | 58aae4f574829798e8d63e336c16ac8b462aa4b0a1ee5718494951fb3a75e704 | Identical to resolved decomp source |
| src/Game/Util/AreaObjUtil.cpp | 317909026601baff7ed63a4d161c64e8c64a0eb6e174ae4dc61382b2b1e26971 | Identical to resolved decomp source |
| src/Game/Effect/ParticleEmitterHolder.cpp | 0936947a99975fe94ae3677a92b45b26594ccf67a777ceac7e60d7adf07704aa | Identical to resolved decomp source; unchanged from native HEAD |
| src/Game/Effect/ParticleEmitterHolder.hpp | 68344025dab4fdcbc31c931905066a16e6271a8233c234499ed146b2d03d8c39 | Identical to resolved decomp header |
| src/Game/Player/Mario.hpp | 7390b3483c4031ff69774cb5380dce20dd0148036b54696c0cba30ad859f6d20 | Preserves native Aurora bitfield declarations |

## Mario

The native MovementStates through DrawStates declaration block remains byte-identical to native HEAD, including Aurora reversal of bitfield declarations for little-endian raw-word correspondence. The merged writeBackPhyisicalVector restores the retail _1C._D gate (802ACE20–802ACF20); the two branches are not conditioned on f28 alone. Earlier original recoveries remain present, including inputStick 0.01, checkForceGrounding 0.99, negated slip-floor predicate, gravity/front-vector gates, and updateGroundInfo assigning checkGround's return.

Header changes _5FC to const HitSensor* and _60D from u8 to bool retain pointer width and byte storage. Their matching users are adjusted; no pointer truncation or bitfield reorder was introduced. Source scans find no duplicate Mario.cpp function definitions. Removed MarioAccess and MarioActor gravity compat definitions have actual original providers in compiled native Game sources; the still-excluded MarioState TU continues using its separate existing compat providers.

## AreaObjUtil

All original helpers now reside in this one Game TU. Removed OriginalAreaMovement.cpp and CameraRepulsiveAreaUtilCompat.cpp providers are absent, and definition scans found no remaining duplicates. AreaForm sphere/cylinder radius/height changes are field renames with unchanged types/order. TRot3f is a 3x4 TRotation3<TMtx34f>; calcCubeAxisZ retains the same third-column extraction. Newly restored cube helpers use typed virtual calls and typed field access, without hardcoded 32-bit offsets.

MR's thin original wrappers still reach native AreaObjRuntimeCompat owner and manager validation, so restoration does not suppress missing-owner or unsupported-manager errors. Header and form declarations can retain separately documented native compile-only differences; identity of this utility source does not by itself prove those headers should be byte-identical.

## ParticleEmitterHolder

The source is unchanged from native HEAD, so this merge does not introduce new emitter lifecycle behavior. MR::AssignableArray stores a native pointer and count, with typed array iteration and delete[]; no packed 32-bit stride was introduced. Existing EffectSystemOwnership retains the resource domain and owns the actual emitter holder. Retirement deletes actor owners and active emitters before the emitter holder and manager. ActorEffectOwner uses pointer-width tokens. No new shadow emitter provider or lifetime transfer was found.

Supporting detailed Mario retail decisions: mario-overlap-audit.md. The separate incoming SwitchWatcher capacity issue is documented in switch-watcher-count-audit.md and is owned by initialization integration.
