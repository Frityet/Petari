# RunawayRabbitCollect reconstruction and PC import boundary

Timestamp: 2026-08-06T19:05:20Z

## Scope

This slice reconstructed the original `RunawayRabbitCollect` game actor in the root decompilation tree and then audited, read-only, what the PC port would need before that source can be imported safely.

Root files reconstructed or corrected:

- `include/Game/NPC/RunawayRabbitCollect.hpp`
- `include/Game/NPC/RunawayTico.hpp`
- `src/Game/NPC/RunawayRabbitCollect.cpp`

No `pc-port/src/Game` source was changed for this reconstruction. The PC-side findings below are an integration plan, not an actor-specific compatibility implementation.

## Recovered behavior

The collector now implements every function present in `build/RMGK02/asm/Game/NPC/RunawayRabbitCollect.s`:

- It counts child `RunawayRabbit` and `RunawayTico` placements, allocates the two child arrays, constructs the actors, and initializes them through the original child-object path.
- It groups rabbits by `Obj_arg0`, counts one completion target per non-negative group, and counts every negative group ID independently.
- It links a grouped rabbit's message controller to the `RunawayTico` whose demo-cast ID matches the rabbit group.
- It prevents simultaneously active rabbits in the same group, records catches, advances every rabbit's runaway level, and selects the normal or final catch message.
- Its wait nerve activates the rabbits after any child Tico reports the runaway start, then broadcasts the HeavensDoor start message.
- Its active nerve tracks caught and chasing rabbits, moves matching Ticos to a caught rabbit's `Spine` joint, chooses the hole/pipe/bush/mama comment order, drives the four BGM states, and plays the original `SE_SY_RUNAWAY_RABBIT_GET_1`, `_2`, and `_3` system sounds.
- `control()` is intentionally empty; the target function is exactly one `blr`.
- The two nerves, their singleton construction, destructor, and static initialization are present.

The restored `RunawayRabbitCollect` layout is `0xB4` bytes in the 32-bit target ABI. The restored `RunawayTico` declarations expose the assembly-defined class layout and methods without inventing PC behavior.

## Assembly and objdiff evidence

The comparison used the repository's existing RMGK02 target object and `build/tools/objdiff-cli`. A fresh collector object was compiled directly with the project's GC/3.0a3 Metrowerks toolchain and compared in `/tmp/rabbitcollect-final-report.json`.

- Overall text fuzzy match: **99.64343%**.
- Exact functions: **14/17**.
- Exact functions include construction, initialization, child placement initialization, message-controller linking, appear notification, the empty control hook, wait behavior, Tico appearance selection, destruction, static initialization, both nerve constructors, and both nerve execute thunks.
- `.data`, `.ctors`, and `.sbss` match at **100%**.

The only non-exact functions are behaviorally source-close register-allocation differences:

| Function | Fuzzy match | Observed difference |
| --- | ---: | --- |
| `calcCompleteRabbitCount()` | 97.05882% | Equivalent loop values assigned to different registers |
| `noticeCaughtRabbit()` | 99.791664% | Equivalent temporary/register assignment around the joint-position path |
| `exeActive()` | 99.26057% | Equivalent local/register assignment; calls and branches match |

No inline assembly or PC-only shortcut was used to obtain the result.

## Build verification

The reconstructed collector compiles with the same GC/3.0a3 flags represented by the existing RMGK02 build target. `src/Game/NPC/RunawayTico.cpp` also compiles against the restored header.

At the time of the initial reconstruction, full root regeneration recognized only the RMGK01 executable SHA-1 while the available extracted executable and target object were RMGK02:

```text
configured RMGK01 main.dol: 25c5959534b3c21246c6c7e42021b916b41fb578
available RMGK02 main.dol:  54b71431af0d509097bfdef4ec28617afc487e89
```

The primary integration subsequently restored the repository's former RMGK02 configuration using the known Rev 1 hash. Fresh `configure.py --version RMGK02 --non-matching --verbose` generation succeeds, and the native Ninja targets now compile directly:

```text
ninja build/RMGK02/src/Game/NPC/RunawayRabbitCollect.o build/RMGK02/src/Game/NPC/RunawayTico.o
[1/2] MWCC build/RMGK02/src/Game/NPC/RunawayTico.o
[2/2] MWCC build/RMGK02/src/Game/NPC/RunawayRabbitCollect.o
```

The initial direct Metrowerks/objdiff comparison remains the recorded match evidence. A full all-object `ninja` build now advances past this actor and stops in an unrelated pre-existing camera-header gap (`CameraGeneralParam::getNum1High`), which is being repaired separately from the collector.

## Remaining root decompilation dependencies

`src/Game/NPC/RunawayTico.cpp` currently contains only the constructor, destructor, and `makeArchiveList`. The collector directly depends on these six still-assembly-defined methods:

- `isStartRunaway()`
- `setPosAfterCaught()`
- `appearHoleComment()`
- `appearPipeComment()`
- `appearBushComment()`
- `appearMamaComment()`

The rest of `RunawayTico` also remains to be reconstructed before a source-complete import: `init`, `initAfterPlacement`, `setPosAllCaught`, `startRunaway`, `setDemoTrans`, and the guide, white-out, white-in, appear, and talk nerve executors.

`RunawayRabbit` is already source-close, but it calls `TrickRabbitUtil::createRabbitFootPrint`, whose implementation is still assembly-only. Its target assembly is one small function (about `0x94` bytes), so that utility should be reconstructed directly instead of importing the unrelated full `TrickRabbit` actor merely to satisfy the call.

## Read-only PC dependency audit

The PC game build globs `src/Game/**.cpp`, so imported sources require no per-file build-list entry. The blockers are API and actor dependencies, not xmake discovery.

The PC port currently has none of the collector actor stack:

- `RunawayRabbitCollect`
- `RunawayRabbit`
- `RunawayTico`
- `Tico`
- `NPCActor`

The placement factory also has no collector entry. Archive metadata already names `RunawayRabbitCollect` and the relevant TrickRabbit, Tico, SpotMarkLight, TicoBaby, and TrickRabbitBaby archives, so factory activation—not archive-list discovery—is the final placement gate once the actor stack works.

### Collector utility surface

The direct collector calls divide as follows:

| Surface | PC status |
| --- | --- |
| NPC movement connection | Present |
| Child count/name/initialization | Present, currently under `JMapUtil` |
| Switch-A write binding and validity | Present, currently under `ObjUtil` |
| Clipping invalidation | Present |
| Broadcast to live actors | Present and scheduler-backed |
| First-nerve-step test | Present |
| BGM-state and system-SE calls | Present |
| `MR::isEqualString` | Missing |
| `MR::createActorCameraInfo`, `MR::initActorCamera` | Missing |
| `MR::tryRegisterDemoCast` | Missing |
| `MR::copyJointPos` | Missing |
| HeavensDoor rabbit wait/start messages (`0xF0`, `0xF1`) | Missing from the PC message enum |

The original collector includes the broad Wii-side `Game/Util.hpp`. A host syntax-only experiment with the PC include tree first and root headers as fallback failed in two useful ways:

1. It exposed the missing utility calls above immediately.
2. The umbrella pulled Wii RVL/JSystem declarations into Aurora and caused extensive duplicate and incompatible SDK/math declarations.

The correct source-close preparation is to replace the collector's umbrella include in the root tree with the exact utility headers it uses, verify objdiff again, and only then copy it. Its narrow set is `ActorCameraUtil`, `ActorSensorUtil`, `ActorSwitchUtil`, `DemoUtil`, `JointUtil`, `LiveActorUtil`, `ObjUtil`, `SceneUtil`, `SoundUtil`, and `StringUtil`.

Where the PC port has already collapsed original APIs into another utility, the compatibility work should restore the general source-facing boundary rather than edit this actor. In particular, child-object helpers belong behind `SceneUtil`, while stage-switch helpers belong behind `ActorSwitchUtil`.

### Actor-family boundary

Importing `RunawayRabbit` introduces these immediate modules:

- `ActorStateBase`, `WalkerStateRunaway`, and `WalkerStateBlowDamage`
- `SpotMarkLight`
- `FootPrint` and the small `TrickRabbitUtil::createRabbitFootPrint` helper
- binder, gravity, velocity, collision, shadow, base-matrix, joint, player-distance, sensor, sound, and talk utility surfaces

Importing `RunawayTico` introduces `Tico`, and `Tico` introduces the broader NPC base chain:

- `NPCActor`, `NPCActorItem`, and `TalkMessageFunc`
- `AnimScaleController`, `LodCtrl`, `PartsModel`, and `TicoDemoGetPower`
- general demo, event, talk, joint-controller, rail, light, actor-movement, actor-sensor, actor-switch, matrix, NPC, sound, and archive-collection utilities

Many individual PC utilities exist, but that complete source-facing API and the NPC classes do not.

### General math and LiveActor prerequisite

This is the deepest compile boundary. The PC `LiveActor` currently defines a local `TVec3f`, while original actors use the JGeometry `TVec`, `TQuat`, and `TMatrix` family. `RunawayRabbit` stores `TQuat4f` and `TPos3f` and uses gravity, velocity, binder state, collision state, and base-matrix operations that the current PC `LiveActor` does not expose.

Falling back to the root JSystem headers is not viable: those headers assume the Wii SDK and PowerPC intrinsics and collide with Aurora's host types. The reusable solution is a host-safe JGeometry compatibility layer, followed by migrating the PC `LiveActor` away from its duplicate vector definition and expanding general actor physics/binder interfaces. Actor-local substitute types or fake rabbit-only movement would make later NPC imports harder.

Audio implementation may remain behaviorally minimal under the port policy, but the original call signatures still need to be available.

## Smallest general integration sequence

The shortest sequence that avoids route-specific shims is:

1. **Finish root-only source preparation.** Narrow the collector and rabbit-family utility includes and re-run objdiff. Reconstruct `TrickRabbitUtil::createRabbitFootPrint` and the full `RunawayTico` implementation against their target assemblies. This lane can proceed independently of PC compatibility work.
2. **Establish the host math ABI.** Add PC-safe JGeometry vector, quaternion, and matrix compatibility in the general Aurora/runtime layer, then migrate `LiveActor` to those shared types.
3. **Complete the general LiveActor substrate.** Add the original-facing gravity, velocity, binder/collision, joint/base-matrix, sensor, shadow, and movement interfaces needed by ordinary mobile NPC actors.
4. **Restore narrow utility boundaries.** Add general `ActorSwitchUtil`, child-object `SceneUtil`, string comparison, actor-camera creation/initialization, demo-cast registration, joint copying, and the two message constants. Back these with existing runtime services rather than collector conditions.
5. **Import the small leaf dependencies.** Bring in `ActorStateBase`, the two walker states, `SpotMarkLight`, `FootPrint`, `AnimScaleController`, `LodCtrl`, and their required general utility pieces.
6. **Import the NPC/talk chain.** Bring in `NPCActor`/`NPCActorItem`, talk-controller support, `TalkMessageFunc`, `Tico`, and `TicoDemoGetPower`, validating each layer before adding the runaway specializations.
7. **Import the route actors in dependency order.** Add the reconstructed `RunawayTico`, then `RunawayRabbit`, then `RunawayRabbitCollect`. Only after construction, child initialization, nerves, messages, and teardown work should `RunawayRabbitCollect` be registered in `NameObjFactory`.
8. **Verify the placement path.** Exercise HeavensDoor child creation and `initAfterPlacement`, confirm the wait/start broadcast, catch grouping, Tico comment order, BGM transitions, and scene teardown with no collector-specific runtime branch.

Steps 2–6 are reusable engine/NPC work. Step 7 is the first point at which copying the collector into `pc-port/src/Game` is likely to produce a useful, linkable actor rather than a growing set of local stubs.
