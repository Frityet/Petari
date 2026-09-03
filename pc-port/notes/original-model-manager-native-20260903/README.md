# Original ModelManager and draw-buffer ownership probe

This is an isolated execution of the actual original ModelManager, XanimePlayer,
XanimeCore, J3DModelX and Game DrawBuffer implementations on complete authored
resources. It prepares the production owner boundary; it does **not** activate
the actor pipeline, replace MarioAnimator, or establish jumping. The production
runtime/texture checkpoint used here is `18c80730d`.

## Source package and reproducible build

`verify-staged.py` checks 14 complete source/header files against root, byte for
byte: ModelManager, DisplayListMaker, MaterialCtrl, DrawBuffer,
DrawBufferExecuter, DrawBufferGroup and DrawBufferHolder. It also checks 17
extracted original utility bodies. `getModelResName` selects the original u32
index overload explicitly on LP64; no other helper body needs normalization.
The shadow publication globals and five accessors are the actual DrawUtil
definitions, not a fabricated texture provider.

`source-evidence.json` records source and archive hashes, the actual runtime
header identity, and the native owner/probe snapshots. Existing root compiler
evidence for DisplayListMaker/MaterialCtrl and the ModelX architecture fixes
remains in the preceding `original-model-manager-owner`, `original-material-controllers`,
`original-material-fog-texture` and `original-modelx-native` notes. This source comparison does not assert a new
retail instruction-match percentage.

The durable files in this directory are:

- `ModelManagerOwner.cpp/.hpp`: the reviewed native ownership boundary, still
  staged outside production.
- `ModelManagerDrawLifetimeProbe.cpp`: the real-resource, two-actor removal test.
- `Helpers.cpp` and `ShadowPublish.cpp`: literal source dependency extracts.
- `probe-only-overlays.patch`: the limited registry/LOD/LiveActor compatibility
  changes required to link this isolated probe. **Do not publish this patch as
  the production actor migration.** It retains the old native animation/model
  path for unrelated actors, removes conflicting old MaterialCtrl providers,
  and supplies the original view call for the two explicitly attached models.
- `rebuild-probe.py` and `draw-link-template.json`: rebuild all 17 owner/ModelX/
  draw objects plus the fixture with the current Game compiler flags. Conflicting
  providers are removed only from a private archive copy under `build/`.

The rebuild requires the preceding verified ModelX staged preparation and the
listed owner inputs in `build/original-model-manager-native-20260903`. It does
not invoke xmake or change production sources. Exact executed commands and
full compiler/linker logs stay in that ignored build directory.

```sh
python3 pc-port/notes/original-model-manager-native-20260903/verify-staged.py
python3 pc-port/notes/original-model-manager-native-20260903/rebuild-probe.py
SMGPC_REAL_DISC="/Users/frityet/Projects/petari/Super Mario Wii - Galaxy Adventure (Korea).rvz" \
  build/original-model-manager-native-20260903/draw-live \
  build/original-model-manager-native-20260903/survivor.png
```

The normal executable links successfully with dead stripping. An additional
all-symbol link, which retains unreachable functions from the complete walk
archive objects, fails at the existing full Mario effect/jump/water/drawing and
Triangle matrix frontier. `whole-symbol-frontier.txt` lists those unresolved
symbols. No dummy methods were added to make that diagnostic link succeed.

## What the live fixture establishes

The fixture mounts the real RMGK01 image and creates the real RuntimeContext,
JUTVideo, CaptureScreenDirector and mapped screen textures. It loads Mario.arc
and MarioAnime.arc through the actual ResourceHolder service. Two actual
ModelManager instances use their original constructor/init and MR model
specialization, giving two real J3DModelX instances with independent actual
XanimePlayer/Core instances. Each model uses the same retained complete model
data: 30 joints, 9 materials and 9 shapes. All 16 original ModelX constructor
display lists are present, and construction restores the caller's GD pointer.

The original DrawBufferGroup registration groups both actors by the original
model resource name into one executer. Registration completes before its fixed
actor-list storage is allocated. The test uses actual LiveActor identities and
their actual view virtual, with no test-only virtual replacement. Original
ModelManager update/calc runs six times per rendered frame. Entry calls original
view calculation and does not advance the animation again.

After two rendered frames the first actor is deactivated and destroyed. Its
actor-runtime and NameObj identities disappear. The original executer's
swap-removal leaves only the second actor. Every active shape packet is checked
against that surviving model; no retired actor shape packet remains. Coalesced
drawer packet counts are checked against the original multiplicity.

The draw buffer deliberately retains the first model as its material prototype.
Every drawer's material packet must still point into that model, while all its
active shape packets come from the survivor. The first ModelManagerOwner lease
therefore remains alive until the final buffer use. The survivor continues
through original BCK frames 18, 24 and 30 with finite original joint matrices.
Settled draw counts change from 22 to 11, and remain 11 for all three survivor
frames. The initial recording reports 36 calls; that startup difference is logged
separately and is not the steady-frame comparison. Its cause is not isolated by
this ownership test.

The test uses existing display-copy readback synchronization before reading
Aurora's render-worker statistics. There are no sleeps, guessed frame delays,
or production instrumentation hooks. Screenshots show the two actual animated
models before removal and only the survivor afterward. Fixture lighting is
deliberately simple and overbright; these images do not establish retail scene
lighting or visual parity.

`build-result.txt`, `live-result.txt` and `live-evidence.json` record the final
successful rebuild and run, including source/binary hashes. Both models pass
finite-matrix checks.

The final teardown also exercises mutable borrowed pointers: it points the
retiring manager/player at the actual survivor's player/core, retires the first
owner, then calls original update/calc on the survivor. This checks that native
retirement uses the captured identities it constructed, instead of deleting
objects found through mutable Game fields. It does not pretend that the fixture
performs MarioAnimator's complete lower/upper initialization.

Both actors and active executer entries retire before their packet owners. After
RuntimeContext destruction, the process's full mapped MEM1 capacity is restored
before video/GX and the process heap are destroyed. This is a texture-capacity
and object-lifetime check; it does not claim per-model reclamation from the
original solid scene heap.

## Production activation prerequisites

1. **One real actor model.** Install this owner as the authoritative actor
   ModelManager and retire the separate native LiveActorModel/frame-controller/
   renderer matrix path atomically. Original movement update, calc-animation,
   joint/material/visibility queries and rendering must see the same model.
   The staged coexistence overlays are insufficient for that migration.
2. **Retain the actual scene allocation cohort.** ModelManager and its model and
   animation holders use the same ResourceHolderService domain. Original
   `initMaterialAnm` can allocate holder-owned MaterialAnmBuffer storage on the
   current heap; an independent per-model heap would leave that holder dangling.
3. **Retain construction identities.** Original MarioAnimator can replace the
   manager's player on the same model and share transform ownership. The native
   owner stores the model/player/core it actually created, separately from the
   mutable Game fields. Additional authored players need their own explicit
   ownership, without each independently deleting the shared model.
4. **Preserve prototype lifetime.** DrawBuffer stores the first model and its
   material packets. Removing an actor does not rebind those pointers. The
   scene draw owner must retain that prototype owner through the executer's
   final use, and remove shape packets before actor/light teardown. Keeping
   allocation bytes after C++ object destruction is not a lifetime substitute.
5. **Own real phase/global state.** Construction has the original allocation
   scope, J3dCommandScope and enclosing J3DSys restoration. Normal update/calc/view
   also uses shared joint calculators, material state and J3DSys globals. The
   scene phase must serialize this state and restore or retire references before
   a model dies. The probe's enclosing restoration and sequential phases do not
   constitute a completed production scheduler integration.
6. **Respect scene texture lifetime.** DisplayListMaker can copy the actual
   screen TIMG into shared model texture data. Resource/cohort leases alone do
   not extend RuntimeContext's capture texture lifetime. Draw/model owners must
   retire before capture/video/MEM1 teardown. The original ShadowProjDummy path
   additionally requires the real CollisionShadow publication/texture owner;
   the five source accessors do not construct a shadow texture.
7. **Complete original draw registration/light ordering.** Use the actual scene
   category table, registration/allocation/activation phases and holder camera
   ordering. Retire the old scheduler's parallel actor view iteration. The
   existing light-sort reset and light-info linkage must be connected to the
   real buffer. This test's null actor-light pointers exercise material/packet
   grouping and lifetime, not full source light-area transitions.

The known nonnull `J3DModelX::viewCalc3` retail stack-address behavior remains
unchanged and outside this ordinary inherited-view probe. Full MarioAnimator,
camera-dependent extra model passes, collision/grounding and jumping remain
separate original-system activation work.
