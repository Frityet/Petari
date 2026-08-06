# Original Dolphin castle route and JPA world-rendering oracle

## Outcome

The modified local Dolphin build deterministically replayed the original Korean
SMG disc from the main title through file select, the five picturebook stops,
the Star Festival, Bowser's attack, and the assault route to Peach's Castle.
The furthest reproducible gameplay checkpoint in this package is presented
frame 22150, on the castle side of the bridge beside the guiding Toad. The
Gateway scene itself was not reached in this run, so this package does **not**
claim an original Gateway `Steam` screenshot.

Presented frame 21250 does provide a useful original-renderer control: a bright
white particle plume at image right remains attached to its world source, while
Mario and the camera are far from screen origin. It does not collapse into a
pile at screen center. This is a general JPA world-placement observation, not a
claim that the visible castle plume is the HeavensDoor `Steam` resource.

The decompiled JPA source gives the stronger implementation oracle. The
original billboard path transforms each particle center by the camera matrix,
builds the camera-facing quad in view space, and then submits the canonical JPA
display-list quad. The captured pre-fix PC fallback placed world-space `x/y/z`
directly into a `TexturedQuad2D`, which explains the center-stacked particles.
That PC excerpt is preserved as a before-state: the shared worktree may contain
a newer compatibility/render-layer correction by the time this note is read.

## How to use this oracle

This package deliberately separates three kinds of evidence. The local
Dolphin checkout executes the unmodified `RMGK01` game image and supplies the
original runtime images/GX packets. The root decompiled `JSystem/JParticle`
source supplies the exact original matrix and shape-dispatch behavior. The PC
real-disc parser supplies resource names and fields that the GX packet trace
cannot expose. The original Dolphin capture is therefore the visual/runtime
oracle; it is not a second PC implementation and it does not validate a result
merely because a PC screenshot looks plausible.

## Representative original screenshots

- [Castle crest, frame 11450](screenshots/01-castle-crest-f11450.png)
- [Festival road, frame 12050](screenshots/02-festival-road-f12050.png)
- [Bowser attack trigger, frame 12500](screenshots/03-bowser-attack-trigger-f12500.png)
- [Airships over the festival, frame 13000](screenshots/04-airships-f13000.png)
- [Peach during the attack sequence, frame 15000](screenshots/05-peach-f15000.png)
- [Assault gameplay handoff, frame 19000](screenshots/06-assault-gameplay-f19000.png)
- [Castle approach, frame 20450](screenshots/07-castle-approach-f20450.png)
- [Bridge/parapet traversal, frame 21150](screenshots/08-castle-bridge-f21150.png)
- [World-anchored white plume, frame 21250](screenshots/09-world-plume-f21250.png)
- [Aligned bridge-mouth player state, frame 21900](screenshots/10-castle-bridge-mouth-f21900.png)
- [Castle-side guiding Toad, frame 22150](screenshots/11-castle-side-toad-f22150.png)

## Exact original JPA matrix path

The relevant source excerpts, with line numbers, are preserved in
[`data/jpa-billboard-source-oracle.txt`](data/jpa-billboard-source-oracle.txt).
The important flow is:

1. `JPAEmitterManager::draw` copies `JPADrawInfo::mCamMtx` into
   `JPAEmitterWorkData::mPosCamMtx` and copies the projection matrix.
2. `JPADrawBillboard` computes the particle center in camera space with
   `PSMTXMultVec(work->mPosCamMtx, &particle->mPosition, &center)`.
3. It constructs a position matrix whose X/Y basis contains global base-shape
   scale times per-particle scale, whose Z basis is `1`, and whose translation
   is that camera-space center.
4. It loads that matrix as `GX_PNMTX0` and draws the canonical four-vertex
   `jpa_dl`. `JPADrawRotBillboard` uses the same center and rotates the X/Y
   basis before drawing.

Shape dispatch matters for full fidelity. In `JPAResource.cpp`, shape type `2`
selects `JPADrawBillboard`/`JPADrawRotBillboard`; shape types `3` and `4`
select the directional particle path. A generalized PC implementation should
first restore correct world/view placement for all JPA packets, then preserve
the distinct basis construction selected by shape, direction, rotation, and
plane type.

## HeavensDoor Steam resource evidence

[`data/steam-resource-metadata.json`](data/steam-resource-metadata.json) is a
compact extraction from the real-disc PC trace's `effects` section. The parser
read the same `ParticleData/Effect.arc` / `particles.jpc` resource used by the
game. The relevant values are:

| resource | base shape | base size | child shape | texture |
| --- | --- | --- | --- | --- |
| `Steam00` (user 2279) | type 4, direction 1, plane 0 | 1.0 x 1.5 | none | `mr_steam00_ia`, 64x64 IA8 |
| `Steam01` (user 2280) | type 2, direction 0, plane 0 | 2.0 x 2.0 | type 2, scale 1x1, inherit scale 0.875 | `mr_steam00_ia`, 64x64 IA8 |

Thus `Steam01` and its children use the ordinary JPA billboard path, while
`Steam00` is a directional shape. Treating every non-child particle as the
same 2D quad loses this distinction even after world anchoring is repaired.

## Deterministic front-end script

All ranges use Dolphin's absolute presented-frame counter.

Wiimote buttons:

```text
120-1700:A+B;2100-2120:A;2700-2720:A;3150-3170:A;4600-4620:A;5500-5520:A;6800-6840:A;8000-8040:A;8400-8440:A;8800-8840:A;9200-9240:A;9600-9640:A;10400-10440:A
```

Pointer:

```text
0-1799:0,0,false;1800-2200:271,264,true;2201-2299:0,0,false;2300-2800:386,385,true;2801-2999:0,0,false;3000-3250:187,251,true;3251-4449:0,0,false;4450-4700:386,385,true;4701-5299:0,0,false;5300-5600:271,264,true;5601-5999:0,0,false;6000-7000:430,420,true;7001-11450:0,0,false
```

The five separated pulses at 8000 through 9640 advance the five picturebook
stops. The final pulse at 10400 starts Star Festival gameplay. Nunchuk movement
begins only after that front-end sequence.

## Chained original-game movement checkpoints

These are ordinary emulated Wiimote/Nunchuk inputs. No RAM writes, position
patches, stage warps, or save-data edits were used. Savestates make the search
reproducible; overlapping endpoints reflect loading the named checkpoint
rather than pretending that exploratory branches were one uninterrupted run.

| state in | absolute input range | result/state out |
| --- | --- | --- |
| front-end gameplay | stick `10800-11400:0,1` | castle crest; safe state at 11400 |
| state 11400 | stick `11400-12000:-0.35,0.937` | festival road state 12000 |
| state 12000 | stick `12000-12450:-0.45,0.893` | Bowser attack trigger state 12450 |
| state 12450 | neutral | attack cinematics; gameplay state 18950 |
| state 18950 | stick `18950-19250:0.25,0.968` | right-side guiding Toad |
| state 19250 | stick `19250-19450:1,0` | tower/road corner state 19400 |
| state 19400 | stick `19400-19750:-1,0` | reddish castle-road state 19700 |
| state 19700 | stick `19700-20150:0.5,0.866` | upper road state 20100 |
| state 20100 | stick `20100-20700:0.5,0.866` | crystal-lined castle bend state 20700 |
| state 20700 | stick `20700-21100:-1,0`; A at `20710-20724`, `20820-20834`, `20930-20944`, `21040-21054` | bridge parapet state 21100 |
| state 21100 | stick `21100-21450:0.7,0.714`; A at `21120-21134`, `21230-21244`, `21340-21354`, `21450-21464` | castle-right grass state 21450 |
| state 21450 | stick `21450-21850:0,-1`; A at `21470-21484`, `21580-21594`, `21690-21704`, `21800-21814` | aligned bridge state 21850 |
| state 21850 | stick `21850-22100:0.4,-0.916` | castle-side guiding-Toad state 22100; captured at 22150 |

The attempted cardinal continuations from state 22100 did not trigger the
doorway; they returned toward the waterfront/plaza or circled the nearby
building. A diagonal bridge-aligned branch reached frame 22250, but continuing
the same heading through frame 22900 also looped back into the plaza. Frame
22150 is therefore the last route state claimed here, and doorway/Gateway is a
known remaining route gap rather than an unexamined pending probe.

## Trace evidence

[`data/trace-validation.tsv`](data/trace-validation.tsv) records the validator
results for eight representative original frames. Every listed trace
contains a frame record, render packets, and a semantic anchor. Notable counts:

- frame 11450: 374 total records, 354 render packets, 2 semantic records;
- frame 19000: 470 total records, 451 render packets, 1 semantic record;
- frame 20450: 988 total records, 969 render packets, 1 semantic record;
- frame 21250: 765 total records, 746 render packets, 1 semantic record; and
- frame 22150: 886 total records, 867 render packets, 1 semantic record.

The bulky raw SQLite traces remain transient. The compact packet grouping in
[`data/dolphin-f21250-billboard-packets.tsv`](data/dolphin-f21250-billboard-packets.tsv)
preserves the original frame's four-vertex, blended JPA-like texture packet
signatures. It includes 64x64 IA8 groups alongside IA4/I8 groups. Dolphin's GX
trace does not expose the game's particle resource names or a game-level camera
pose, so the table intentionally does not assign the visible plume to a named
effect.

## Build and savestate provenance

Machine-readable hashes and exact commit ancestry are in
[`data/provenance.txt`](data/provenance.txt); the successful incremental build
output is in [`data/dolphin-build.log`](data/dolphin-build.log).

- Game image: `/workspaces/pcport/RMGK01.iso` (Korean disc ID `RMGK01`,
  SHA-256 `0c321eff29251c8c7a1eed87d03dcf9d10908b2c2fdb177256e7ed9b39851ea6`).
- Dolphin checkout: `pc-port/dolphin`, origin
  `https://github.com/Frityet/dolphin`.
- Dolphin commit: `ed8e44d4be114fc70258fbfaeb239f3e83b041fe`
  (`Core: add deterministic presented-frame savestates`).
- Scripted-Nunchuk ancestor: `5895bce8fddc05265d24df5f763a078a55c2d9ed`.
- Binary: `pc-port/dolphin/build-nogui-libcxx/Binaries/dolphin-emu-nogui`.
- Binary SHA-256 after the recorded build:
  `54c88ed12cb89ce871061fc890ccc3c24fb11cec44a69a75eea5ae0de753b486`.
- Build command: `cmake --build build-nogui-libcxx --target dolphin-emu-nogui -j4`.
- Backend: NoGUI, X11 platform, OpenGL video backend, emulated Wiimote 1.
- Initial route state: `/tmp/dolphin-gateway-safe-f11400/frame-11400.sav`.
- Initial route-state SHA-256:
  `8cf23e2686d025037e40bddba4a6a4c4c0edd856002da31c2f92403a2f1e7879`.
- Castle-side route state at frame 22100 SHA-256:
  `1dea84e5bf0e6b841820778756b6571a7c23776ce96b4085e1b1650dba1708c5`.
- The save hook was requested with
  `SMGPC_DOLPHIN_SAVE_STATE_FRAME=11400` and dispatches `State::SaveAs` through
  a CPU-thread host job. A separate save/load smoke reached frame 900 and
  validated 12 records, 5 render packets, and 1 semantic record.

The nested Dolphin branch is clean and its exact HEAD equals `origin/master`.
No alternate `RMGK02` image was needed for this oracle: the Dolphin build
passed and the recorded `RMGK01` image booted/replayed correctly. The parent
gitlink was intentionally not staged or committed by this oracle run.

## Implementation checklist

1. Keep decompiled `Game/` behavior source-close; place host-renderer adaptation
   in the compatibility/render layer.
2. Give JPC rendering a true world/view-space packet path. Do not feed world
   positions into `TexturedQuad2D`.
3. Transform the particle center exactly once by the active camera/view matrix,
   then construct the quad basis in view space (or equivalently use camera
   right/up in world space before the normal view/projection transform).
4. Apply base-shape size, emitter/global scale, and per-particle scale before
   projection. Preserve child inherit-scale behavior.
5. Dispatch shape type 2 as billboard and shape types 3/4 as directional
   geometry; retain rotation and base-plane choices.
6. Preserve the already-correct IA8 alpha/intensity decode, TEV, blend, alpha
   compare, depth test/write, texture animation, and draw ordering.
7. Extend debug traces with world center, camera-space center, basis vectors,
   and projected corners. Verify several emitters and two camera poses so a
   screen-center special case cannot pass.
8. Compare the corrected PC result against the original frame-21250 plume and,
   once the original Gateway is reached, add a same-scene `Steam00`/`Steam01`
   comparison.

## Limits

- This package reaches the castle-side Toad, not the abduction cutscene or
  Gateway. The exact Gateway timing and an original Gateway Steam screenshot
  remain open.
- Frame 21250 proves general original world anchoring but is not resource-name
  proof for HeavensDoor `Steam`.
- Steam base/child metadata comes from the real-disc resource parsed into the
  PC trace; the original Dolphin GX trace supplies independent visual and
  packet-state evidence but not JPC names.
- Savestates and raw traces are deliberately omitted from the compact bundle
  because each state is roughly 40 MiB and the useful traces are several MiB.
