# RFL/Mii real-or-absent cleanup

Date: 2026-08-07 UTC

## Result

The PC RFL service no longer creates an official `Mario` Mii, a default-source
copy, an `SMGPC` creator identity, or create IDs derived from entry indices.
When `/shared2/menu/FaceLib/RFL_DB.dat` is missing, the database status is
`RFLErrcode_DBNodata`, all Mii collections and selector pages are empty, and
model/icon/info requests return no-data without writing synthetic output.

The private host `SRFL` parser, serializer, mutation API, and persistence path
were removed. Arbitrary bytes at the retail `RFL_DB.dat` path are now present
but unsupported (`RFLErrcode_NotAvailable`); they are never interpreted as a
host-only database and never rewritten in a private format.

Resource initialization is separate from the user database. The compatibility
holder reads the real `/RFL_Res.dat` entry from the disc's
`/ObjectData/MiiFaceDatabase.arc`, and the service verifies the retail
18-archive resource structure before accepting it. A missing or malformed
resource is an actual load/corruption error. A valid resource may initialize
successfully while the optional user database remains empty/`DBNodata`.

The decompiled RVLFaceLib sources under `src/RVLFaceLib` were used as the
reference for data-independent SDK behavior too. PC work-size results now
match the retail heap sizes plus the `0x1F24` PPC manager
(`0x4CF24`/`0x65F24`), model-buffer sizing uses the retail `0x8260` resource,
32-byte texture-object/alignment, and mask-level formula, and favorite colors
match the retail table exactly. The former host estimates and altered palette
are gone.

The host-generated RGB5A3 face rasterizer and its `ResTIMG` writer were
removed. Character-model initialization, expression changes, draw requests,
and icon generation do not mutate output or report success unless a real
implementation exists. At present the PC port has real resource validation
but no RFL model renderer or retail user-database parser, so those operations
explicitly report `NotAvailable` or `DBNodata`.

The RVLFaceLib bridge no longer constructs a fallback service. Without an
active `RuntimeContext`, initialization, availability, async wait,
character-model, and icon calls report `NotAvailable`; additional-info calls
return zeroed `DBNodata` output. Data-independent SDK size queries remain
available.

`MiiFacePartsHolder.cpp` and `.hpp` are byte-identical to the regular
decompiled source and the PC target excludes the retail source until its full
face-model dependency closure is present. `MiiFacePartsHolderCompat.cpp`
supplies the retail ownership/type surface, performs the genuine archive
lookup, and reports a terminal error when that resource or the runtime is
absent. Unsupported face-part creation returns `nullptr`; it does not create a
substitute actor or texture.

The port's rewritten `FileSelector::createFileItems()` had omitted the retail
`MR::createSceneObj(SceneObj_MiiFacePartsHolder)` call. That exact ownership
step is restored at the corresponding point, rather than globally creating the
holder in the host scene or branching on a route/stage name.

The hardcoded English fellow-name list and the explicit `Mario`/`Luigi` user
name fallback were removed. Both fellow labels and actual Mii labels now pass
through `FileSelectFunc::copyMiiName`, preserving localized message data and
UTF-16 RFL names. The retail `MiiSelectNrvDummySelected` state was not changed.

## Real BTP playback

The J3D compatibility path now parses the retail `TPT1` block used by `.btp`
files. It validates the material-name table, material IDs, texture-map slots,
frame/value ranges, and referenced model `TEX1` indices. Playback resolves the
action-to-BTP mapping from the real `ActorAnimCtrl.bcsv`; for example, the
Korean `FileSelectDataMario.arc` maps action `normal` to `wait`. There are no
hardcoded FileSelect animation names, frame counts, or texture indices in the
runtime implementation.

The renderer applies the evaluated texture index to the actual material slot
and exposes the effective binding in packet traces. A malformed/trackless BTP,
an absent action resource, a material mismatch, or an invalid texture index
leaves BTP unavailable instead of reporting playback success. The FileSelect
fixture proves that all four retail resources (`blink`, `close`, `open`, and
`wait`) target the real `EyeMat_v` material, slot 0, and valid model textures.

`MR::checkPassBckFrame` no longer uses nerve-step modulo arithmetic. The host
path derives the current real controller state and delegates interval testing
to the retail-shaped `J3DFrameCtrl::checkPass`. Focused tests cross-check all
five controller attributes over 40 updates, including loop/reverse boundary
frames.

## Layout and pointer boundary

The first strict-route exception came from `SaveIcon::calcAnim()` reading the
real `SaveIconPosition` marker while its `SysInfoWindowMini` host was dead.
The retail archive does contain that pane. Transform lookup had incorrectly
used `paneBounds`, which intentionally rejects dead, hidden, transparent, and
zero-sized panes. `copyPaneTrans` now reads the parsed pane matrix instead:
missing panes still fail, but a dead or hidden real pane retains its real
transform without becoming visible or pointable.

Focused tests cover the distinction directly with the retail
`SysInfoWindowMini.arc`: `SaveIconPosition` exists and its location-adjusted
matrix is available while the host is dead; dead and hidden states have no
pointer bounds; matrix reads never revive the host; and an absent pane neither
writes an identity matrix nor changes caller storage.

The host-only screen-coordinate rectangle in `MiiSelect.cpp` was removed.
Mii selection now consults only `isStarPointerPointingPane` for the real
`Mii01` through `Mii08` pane transforms. If that pane/target lifecycle is not
active, selection remains unavailable; there is no widened 128-pixel hitbox or
route-specific input branch.

## Honest route status

The original route predicate accepted a registered-but-dead `FileNumber`
layout and unrelated starfield/Mario packets. Thus the earlier
`route-real-btp3/file_confirm` and `route-real-btp4/file_select` manifests are
retained only as diagnostics. Their SQLite traces contain zero
`FileSelectData*` packets and zero active-BTP packets; they are not BTP or
FileSelect success evidence.

The route validator now rejects that false positive. FileSelect checkpoints
require a non-dead layout that actually emitted layout packets, a real
FileSelect planet/face model, and absence of the explicit title-activation
failure. The `file_confirm` checkpoint additionally requires an `EyeMat_v`
face packet with active BTP state, a nonzero real BTP frame/material range, and
a real slot-0 texture binding.

The strengthened run stops honestly at the current first dependency:
`sequence:title_activation_unavailable`, with detail `real MarioActor
auto-rush binder event is not installed`. Its trace has zero live
`FileNumber` layouts, zero `FileNumber` packets, and zero FileSelect model
packets. The picturebook attempt consequently has no `PrologueDemo` layout;
its screenshot is the starfield and is retained as failure evidence. No title
advance, BTP packet, picturebook, or Gateway success is claimed in this note.

## Source identity

- `pc-port/src/Game/NPC/MiiFacePartsHolder.cpp` matches
  `src/Game/NPC/MiiFacePartsHolder.cpp` byte-for-byte.
- `pc-port/src/Game/NPC/MiiFacePartsHolder.hpp` matches
  `include/Game/NPC/MiiFacePartsHolder.hpp` byte-for-byte.

## Verification

See `verification.log` for focused tests, the full build, exact SQLite counts,
and the strengthened route-validator result. Useful binary evidence is kept
in place:

- `route-strict-validator/file_select/`: strengthened failure manifest, app
  log, screenshot, and SQLite trace at frame 1900.
- `route-real-btp5/picturebook/`: honest missing-`PrologueDemo` failure,
  starfield screenshot, app log, and SQLite trace at frame 7600.
- `route-real-btp3/file_confirm/` and `route-real-btp4/file_select/`: old weak
  predicate captures retained to demonstrate why registration alone was not
  sufficient.

`fallback-audit.log` records the targeted lexical audit. Matches remaining in
the owned test sources are negative assertions, while the route script names
forbidden legacy placement statuses so a future trace cannot reintroduce
them.
