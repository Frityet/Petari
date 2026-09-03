# Original scene draw-buffer ownership

The staged service owns the actual `DrawBufferHolder`, original category table,
groups, executers and packet drawers. Registration, `allocateActorListBuffer`
and activation are separate phases. A model registration after allocation
throws; the owner never grows or rebuilds an active original list.

Each executer retains the first actor's actual `ModelManagerOwner` as its
material-packet prototype. Removing that actor removes its shape packets but
does not replace the prototype. The shared prototype remains until holder
retirement. Unregister and holder teardown drain queued GX work before the
corresponding owners may be released. Host metadata keeps the original
allocation domain alive through original holder/model destruction.

## Verified service fixture

`build/original-scene-draw-buffers-20260903/rebuild-probe.py` rebuilt the actual
ModelManager/ModelX/DrawBuffer overlays against the latest native JMap layout
and archives, then linked `live`. With the real RMGK01 RVZ, the five-frame probe
passed:

- Two independent actual models/players/cores share one executer and produce
  22 settled draw calls.
- After destroying the first actor, the executer retains its material prototype
  and contains only the survivor's shape packets. Three frames each produce
  11 draws.
- Original BCK frames advance 6, 12, 18, 24, 30. Entry does not advance them.
- Retiring the holder releases its prototype. A survivor whose player/core were
  borrowed by the retired manager still updates/calculates at frame 31.
- Runtime and texture retirement restores the complete initial MEM1 capacity.

The initial frame again reports 36 bootstrap draws, recorded separately from
the settled 22. This fixture is an actual scene draw service test, not proof
that the subsequently staged scheduler/route integration already passes.

## Staged scheduler and lifecycle boundary

`SceneScheduler` owns the service, with explicit begin/allocate/retire methods.
Registration retains the registry's actual ModelManager owner and respects the
original draw category. All scheduler removal paths disconnect the actual
holder before erasing raw actor pointers. Death, clipping and temporary draw
connection determine activation; movement suspension does not hide models.

View entry follows `SceneFunction::executeCalcViewAndEntryList` literally:
identity J3D view, actual holder entry for camera category 2D, `MR::loadViewMtx`,
then actual holder entry for 3D. The holder invokes actor view callbacks;
scheduler iteration no longer invokes them again. Opaque/translucent rendering
uses the original holder/group packet drawing, including original per-executer
light grouping. `MR::findActorLightInfo` forwards to the actual holder.

The old native animation synchronization step and model renderer facades are
removed from the staged scheduler. Debug state reads the actual model matrix,
Xanime BCK name and material-animation resource table identities.

`SceneJ3dScope` retains GD/OS execution context and restores the complete
J3DSys object, static current matrix/scales, active joint/calculator/buffer and
texture-scale table across complete movement, animation, view and draw phases,
including exception unwinding. It does not advance clocks or replace any
original calculation. Original `MR::drawInit` and default viewport/scissor
bodies are supplied directly from root DrawUtil for native 3D submission.

Stage construction allocates lists after all placement callbacks and before
the final appear pass. Stage teardown first removes registrations, destroys
actors, then retires the holder. Runtime teardown clears remaining scheduler
registrations and prototype leases before capture textures disappear.

## Title and file-select construction evidence

Root `FileSelector::init` creates the camera, sky and file items before creating
the title. `FileSelectItem::createNew` constructs the PartsModel planet and
immediately marks it dead. `FileSelector::exeTitleEnd` requests `goToFarPoint`
and appears existing items; it does not create their models.

The staged native route follows that construction boundary. Its title visual
defers list finalization to the enclosing route; the route constructs the Far
composition while registering the initial scene, then allocates once. Planets
remain dead until `begin_far` transfers the same sky and requests the original
camera transition. The actual camera now runs throughout the title, so the old
two manually injected camera movement calls are removed. Standalone title
visuals finalize their complete sky-only composition themselves.

Direct tests formerly constructing a Far composition *after* an already
finalized standalone title need to use the explicit early construction phase.
The obsolete late-construction convenience constructor is removed. The
existing bounded native far selection/scale/number flow is otherwise retained;
this change does not claim full original FileSelector/RFL activation.

## Activation status

All nine staged translation units compile with the current original actor
retirement package and native ABI flags. The 17-file `activation.patch` passes
`git apply --check`; its RuntimeContext delta preserves the parent's independent
actual-model effect matrix change and its DrawUtil declarations preserve the
new original shadow accessors. The parent owns production application, shared
linking and the next actual showcase/route tests. No production files were
modified by this staging task.
