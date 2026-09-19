# WarpPod closure and authored rabbit-route audit

The exact factory activation and general group helper passed an actual
360-frame original-process probe. Player warp traversal remains untested.

## Native closure

`src/Game/MapObj/WarpPod.cpp` and its header already contain the recovered
original actor and manager. The native source is compiled, but the native
factory table has no WarpPod descriptor. Dead stripping means the existing
main binary retains manager/player-camera helpers, not the actor's full
initialization/vtable path. The archive table already records WarpPod.arc,
and the original SceneObjHolder provider already constructs WarpPodMgr.

The direct compiled actor object's undefined imports, compared against all
project archives, identify exactly one missing non-host symbol:
`MR::joinToGroup(LiveActor*, const char*)`. See `direct-undefined.json` and
`warp-object-undefined.txt`. This is direct symbol closure, not recursive
linking or runtime proof.

The complete donor helper at `decomp/src/Game/Util/LiveActorUtil.cpp:1194`
resolves the existing named group with `NameObjFinder::find`, calls
`LiveActorGroup::registerActor`, then returns that group. Its native source
file is excluded because utility providers are implemented separately.
`src/compat/OriginalLiveActorGroupUtil.cpp` already owns adjacent group APIs
and is the appropriate exact donor provider location. Native NameObjFinder
uses the actual scene NameObjHolder, whose find/cache implementation compares
the original names. WarpPodMgr constructs its named group before WarpPod init
joins it. The group stores borrowed actor identities; it does not own or
delete the actors.

The original actor uses GroupId plus zone identity for pairing; Obj_arg0 is
part of the camera name, not its pairing key. Pair initialization can allocate
the original 60-point path and its TestColor.bti/TestMask.bti JUTTexture owners
even for an invisible placement. Actual initialization therefore needs the
real WarpPod resource owner, scene manager, camera, sensor and heap services.
EventUtil and GameDataFunction persistent-bit/count providers already exist.

The preexisting native initPair initialization from the paired zone's low
byte is a documented architecture correction representing retail register
residue. It is not an arbitrary warp workaround and should remain; see
`../original-warp-pod-draw-20260907/README.md` and its DOL evidence. No donor
recovery is currently necessary for the missing wrapper or factory row.

## Actual authored activation metadata

`stage-metadata.json` was read directly from the actual RVZ's
StageData/HeavensDoorMysteriousZone.arc using the existing nod extraction
utility and the local read-only Yaz0/RARC/BCSV inventory script. The extracted
archive remains under ignored build storage. `extract.log` records extraction.
This corroborates the earlier placement inventory while adding GroupId and
the child object records that the earlier JSON omitted.

- WarpPod l_id52 and53 share GroupId0 in zone5. Their args1/2 are0/0 and
  they have no stage switches. They are invisible sensor warp endpoints,
  distinct from the visible EarthenPipe pair.
- EarthenPipe l_id56/57 share original pipe arg0=10. Their SW_B values are
 1112/1118. Original pipe exit turns on that switch after the player exits,
  provided the rabbit minigame has released its initial WAIT gate.
- Collector l_id4 creates its child actors directly. Two alternative
  RunawayRabbit children in group1 use SW_APPEAR1112/1118, so either authored
  pipe exit reveals that same collectible rabbit group.
- The bush group2 rabbit uses SW_APPEAR1113; the hole group0 rabbit uses
 1114. Original SwitchCube l_id8/11 write these switches and are gated by
 1111. The CastId0 RunawayTico child writes1111 from original startRunaway.
  Scenery bush interactions are not the reveal trigger in these records.
- Collector activate changes hidden rabbits from NoActive to Hide. Their
  registered SW_APPEAR listener invokes original startRunnaway, shows the
  model and validates sensors. Nearby sound checks do not reveal them.
- The final catch selects RunawayTico::appearMamaComment. Its original
  WhiteOut/WhiteIn route starts the authored Tico guide tower demo. WarpPod
  has no direct role in the three rabbit reveal switches above.

Thus missing WarpPod remains a legitimate general actor-availability gap,
but it should not be described as the missing pipe rabbit reveal trigger.
No direct teleport, fabricated switch, or stage-specific position is needed.

## Next bounded validation

The exact general named-group helper is now copied into its existing native
provider; `group-provider-source-equivalence.json` verifies identical donor
body bytes. The exact original WarpPod factory row is provisionally enabled.
This sequencing was authorized after the separate PunchingKinoko probe showed
that temporary post-frame model construction violates the original requirement
to register models before actor-list allocation. No late-registration bypass
or special constructor hook was added.

`smg-pc-original-process-warp-pod-tests` observes the naturally constructed two
authored actors. It checks mutual pairing, real group/model/camera/sensors,
the original 60-point path and its two texture owners, exact movement-only
scheduler registrations, and normal scene retirement. The observer performs
no gameplay mutation. Such a probe establishes actor initialization/ownership;
post-intro movement and player traversal still need separate observations.

Production changes are confined to the exact helper in
OriginalLiveActorGroupUtil.cpp and the original factory row in
scene/nameobj/NameObjFactory.cpp. Game sources remain unchanged.

## Validation

The first run completed 360 ordinary frames but failed the diagnostic's
expectation that both WarpPod timers would advance during the opening. Its
evidence is retained as `initial-probe-timer-assumption.{log,json}`. The actual
pair/group/resources/sensors/path checks had already passed at frame37. No
compatibility failure was hidden or bypassed: the revised observer records
original flags and checks the exact scheduler registrations instead of
assuming that authored opening actors must move.

The revised target linked successfully and `run_probe.py` completed 360 frames
with exit0, no timeout, PID68014 gone, no debugger, in8.0114 seconds. Binary
SHA256: `66c8345f04d7e44314e91450f9493f798f7bfac7660cbc96d591c82d6d1d50e5`.
See `original-process-run.{log,json}`. All construction checks passed at
frame36, and both actual actor, group and manager identities were absent after
normal scene retirement.

At frame359 both actors were alive, clipped, and had original NameObj flag1
(movement off), with their timers still0. This accounts for the initial test
assumption failure and does not justify forcing their updates. Include
`WarpPod` in the next general `SMGPC_DEBUG_ACTOR_TRACE_TYPES` selection alongside
MarioActor/RunawayRabbit/RunawayTico for post-intro movement observations.
The actual console/gameplay script, not this probe, must establish traversal.
