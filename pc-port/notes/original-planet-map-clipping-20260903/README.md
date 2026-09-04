# Original PlanetMap placement/clipping recovery

Frozen 2026-09-03. Root files are `src/Game/Map/PlanetMap.cpp` and the final
`PlanetMapClippingInfo` struct in `include/Game/Map/PlanetMap.hpp`. No production
native source, shared build, xmake file, or owner implementation was changed.

## Retail behavior and corrections

The original init method reads Arg0 and Arg2, initializes each local to -1,
and leaves their results unapplied. It retains one otherwise unused floating
comparison **against zero**. Contrary to the concern raised in the previous
creator audit, the unused argument results are original behavior, not evidence
of missing actor assignments. The old source compared against -1; the new
source preserves the exact retail comparison and reads. No argument effect or
actor field was invented.

Clipping uses a real one-record table: name `PhantomShipA`, radius 3000, center
offset `(800, 1300, 0)`. The table occupies 20 bytes on PPC: one pointer, one
float and one actual `Vec`. The former header had a fabricated trailing word
and the former code interpreted the radius as an offset coordinate. The new
root layout and full table match the disc, including every float bit and the
name pointer. Pointer width naturally changes the host struct size.

The record is selected only when the model name matches, using the original
case-insensitive comparison. This corrects the old inverted match. The named
case overrides Arg1 with the table radius and retains `mPosition + offset`
in the actor's existing `_90` vector, whose stable address is passed to the
clipping system. It does not rotate or scale this authored offset. The former
code added to an uninitialized temporary and supplied the wrong radius.

For other names, a positive Arg1 is the sphere radius. If it is zero or
negative, the original model-bounds helper supplies the radius and 100 is
added. For valid placement info, original group clipping receives capacity
128. No branch was removed or replaced with a special native path.

## Original compiler proof

`verify-original.py` compiles the complete original PlanetMap TU. Init is
**604 bytes, relocation-aware byte exact** (raw fuzzy 99.834435%). InitClipping
is **316 bytes, raw fuzzy 99.87342%**. Every instruction matches after verified
relocations and a checked exchange of two independent 12-byte stack temporary
locations. The proof checks the exact five affected instruction words and
both source/retail slots before remapping them; it changes no arithmetic,
register assignment, instruction order, branch or external call. This is
canonical equality, not a claim of raw byte equality for initClipping.

The proof also validates the complete 20-byte clipping table, its relocated
string, and the `cFollowJointName` pointer/string. DOL and compiled objects
remain in ignored build storage; notes contain text evidence only.

## Native compile and owner readiness

`stage-native.py` copies the complete root source and header literally into
`build/original-planet-map-clipping-20260903/staged`. The entire native TU
compiles using current native headers, **without root-header fallback**.
`native-dependencies.json` lists all 75 direct undefined symbols; all Game
symbols have definitions in the current game/common/render archives. The five
absent symbols are system exception/typeinfo/stack-check dependencies. This
is a symbol inventory and compilation result, not a linked runtime claim.

A truthful actual-owner runtime fixture is not yet available:

| Required original authority | Current native boundary |
| --- | --- |
| ClippingDirector → ClippingActorHolder sphere state | GameRuntimeCompat forwards to ActorRuntimeRegistry clipping metadata |
| ClippingDirector → ClippingGroupHolder grouping | GameRuntimeCompat throws for a valid authored clipping group |
| Actual CollisionParts identities and camera-code collector | PlanetMapRuntimeCompat / CollisionPartsCompat still represent registrations and return no actual parts for optional collision resources |
| Original low/middle, bloom, water and indirect model graph | PlanetMapRuntimeCompat rejects these optional model branches; complete original providers remain gated |
| Original LodCtrl animation/resource graph | Native createLodCtrlPlanet adds ownership but rejects present low/middle models and omits their original tryStartAllAnim calls |

The actual model accessor/bounds providers already available do not close
those clipping/collision/optional-model owners. Testing these recovered methods
against a replacement clipping recorder, null model or unsupported native
planet subset would not satisfy this task. Therefore no synthetic owner test
or claimed PlanetMap initialization runtime was added. Existing earlier raw
KCL, actual model-owner and real placement-data proofs remain separate evidence.

On eventual activation, retain the real model and scene allocation cohorts,
register actors with the original clipping holder, retain `_90` through those
borrowers, retire clipping/collision borrowers before actor/native model owners,
and reclaim the scene domain last. This source patch does not change that
ownership order. Full factory/class expansion remains outside this checkpoint.

## Artifacts and reproduction

`root.patch` is narrow against creator checkpoint d57d7d245. `native.patch`
contains the literal source update and header delta; its header also includes
the preceding creator checkpoint's two still-staged constructor declarations.
`native-manifest.json` records source hashes and the production baseline. Parent
owns applying that patch and any provider selection. `source-evidence.json`
verifies root/native source/header identity.

```
PYTHONDONTWRITEBYTECODE=1 python3 pc-port/notes/original-planet-map-clipping-20260903/verify-original.py
PYTHONDONTWRITEBYTECODE=1 python3 pc-port/notes/original-planet-map-clipping-20260903/stage-native.py
PYTHONDONTWRITEBYTECODE=1 python3 pc-port/notes/original-planet-map-clipping-20260903/freeze.py
```
