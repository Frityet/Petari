# Complete original camera owner source activation

The normal native Game archive now compiles the actual CameraHolder and all 45 original controller/translator entries, plus the transitive CameraParamChunk/Holder, CameraManGame/Event/Subjective and CameraDirector ownership graph. The runtime still uses its existing scoped target and camera service binding; no reduced selector was added and full original camera runtime integration is not claimed.

`import-manifest.json` records 103 newly copied original C++ TUs and 106 required original headers. All 209 current pairs are byte-identical after the reference-first compile boundaries below. The overlapping CameraLocalUtil.cpp was deliberately retained in its existing native owner; `OriginalCameraOwnerUtil.cpp` adds exact original nonoverlapping helper bodies and HUD/subjective predicates. It also forwards camera interpolation and player braking to actual original owners. There is no Game/xmake change for this cohort; normal source globs include it. `source-manifest.json`, `root-source-paths.txt`, and `reference-source-paths.txt` are the exact checkpoint paths and SHA256 values. `tests/xmake.lua` is shared with other agents; our target is `smg-pc-original-camera-holder-tests`.

## Reference-first compile boundaries

- CameraManGame passes a CANM pointer through the existing pointer-width general parameter using intptr_t. Its Wii createStartAnimCamera stays 100%, 164 bytes; all 57 paired preexisting functions retain their scores.
- CameraParamChunk loads binary num1 through a signed 32-bit temporary, then assigns the native pointer-width slot only if the field exists. This preserves signed values, adjacent num2, and existing wide values when absent. Wii load changes 100%/1072 bytes to 96.99254%/1088 bytes; that is the sole score change among 31 paired functions and is the explicit architecture boundary.
- CamTranslatorSpiral uses the existing typed packed-halfword accessors instead of pointer-layout aliasing. Reference accessors retain original big-endian address semantics; the existing native header performs value-based host-endian decoding. setParam remains 100%/84 bytes, getCamera 100%/8 bytes, with no baseline changes.

`wii-compile-results.json`, `wii-baseline-comparison.json`, and `CamTranslatorSpiral.wii-proof.json` contain fresh compiler/objdiff commands and results. CameraHolder constructor and createCameras both match retail 100% (100 and 168 bytes).

## Native validation and exact current limit

All 103 imported TUs passed isolated Homebrew LLVM compilation. The normal target then compiled all sources and archived Game, but its first link failed: `holder-build.log` and compact `holder-initial-undefined.txt` record eight real retained shared provider gaps. Subsequent saved helper work closes interpolation, player braking, normalized pointer coordinates and input distance; the collision agent independently built/tested the exact original Collision strike-list provider. A fresh complete holder link has not yet run. The remaining audited frontier is the original Tripod accesser/joint queries and actual pointer depth ownership. No test or production fake providers were added. There is no camera holder runtime pass yet.

`retained-helper-compile-results.json` confirms current helper, GamePadUtilCompat and stage-owner fixture isolated compilation all succeed. The pad normalized query converts native framebuffer pixels back to original normalized coordinates and preserves invalid-pointer zero; input distance forwards actual Aurora state. Host mouse input currently supplies a virtual sensor distance of one meter while valid; no physical-distance measurement is claimed.

## Fixture coverage prepared

The fixture uses normal Game archive definitions: all 45 constructor/virtual translator identities and Game heap provenance, typed registration cleanup, full 1024 chunk capacity, original prefix subclass selection, duplicate rejection, sort/lookup, binary signed/wide field decoding, absent fields, packed Spiral halfwords, and actual Follow/Tower/WaterPlanet translator state. Its optional RVZ path registers all authored IDs then invokes original CameraParamChunkHolder::loadFile(0) through the new retained StageResourceBinding for HeavensDoorGalaxy and EggStarGalaxy. The stage archive and decoded JMap string registration outlive the original temporary reader. This path is compiled but awaits the genuine retained-provider closure before runtime execution.

The next pointer depth implementation is separate under `notes/original-star-pointer-depth-20260907/`; its draft source is not part of this checkpoint. `OriginalTripodBossQuery.cpp` here is a notes-only exact original provider candidate, also not in the checkpoint source list.
