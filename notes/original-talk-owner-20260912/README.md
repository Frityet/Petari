# Original Talk owner migration (in progress)

The original owner cohort is imported and directly compiled, but the active
synthetic Talk providers have deliberately not been deleted yet. Original
TextBox/CustomTagProcessor and the GX completion callback route are being
recovered concurrently. No linked original talk startup or conversation has
been validated at this point.

## Sources and reference evidence

Imported whole native sources: TalkDirector, TalkBalloon, TalkState,
TalkMessageCtrl, TalkMessageInfo, TalkSupportPlayerWatcher, and TalkUtil. The
existing whole TalkNodeCtrl source remains unchanged; its existing native tag
reader adjustment is retained. The process-services agent owns TalkTextFormer
and recursive text processing, so that source was not overwritten here.

Recovered the three missing TalkUtil creation wrappers in decomp first. The
originals allocate TalkMessageCtrl, create its message, and return it; the
automatic-root variant additionally sets its original flag. All three match
the retail object exactly (124, 124, and 132 bytes). All 53 comparable TalkUtil
functions are 100%; its reference configure entry is now Matching.

Reference/native portability corrections are restricted to invalid bool or
integer comparisons against nullptr in TalkMessageCtrl/TalkState and local
min/max helper names in TalkBalloon which otherwise collide with host standard
library overloads. The TalkMessageCtrl event helper now has the original
anonymous namespace linkage and NO_INLINE declaration; that helper also
matches 100%.

Native TalkDirector::getBranchResult keeps the original 12/13/18 live queries
and obtains the bool region at its native member offset instead of retail's
literal 0x70. The Game Util/ObjUtil header gains its missing original
isStageStatePowerStarAppeared declaration; no replacement behavior was added.
The new revolution/gx/GXGet.h is a regular SDK include shim.

The reference comparison covers all original TU functions:

| Source | Functions | Weighted instruction match |
| --- | ---: | ---: |
| TalkUtil | 53 | 100% |
| TalkMessageInfo | 14 | 100% |
| TalkDirector | 62 | 99.97% |
| TalkBalloon | 69 | 99.90% |
| TalkMessageCtrl | 43 | 99.82% |
| TalkSupportPlayerWatcher | 5 | 99.87% |
| TalkState | 31 | 98.76% |

These figures exclude data symbols. match-summary.json and compressed objdiff
reports preserve the exact per-function results. retail-dol-proof.json records
5,743 verified instructions across those TUs against the actual RMGK01 DOL,
SHA1 25c5959534b3c21246c6c7e42021b916b41fb578.

Native direct compilation now passes for every imported source, including
TalkDirector against the real GXSetDrawSync SDK declaration added by the root
agent. An earlier compile recorded a pending declaration only; that evidence
has been superseded by owner-followup-native-commands.json. DemoUtil and
TalkDirectorLifetime also compile directly. No Xmake build was run concurrently
with the root agent's work.

## Native lifetime boundary

TalkDirectorLifetime stores only the completed original director identity,
its NameObj generation, and whether scene retirement has begun. It does not
replicate controller ownership, graph traversal, presentation, clocks, or
conversation state. Original raw holders, states, watchers, text formers,
message nodes, generated names, cameras and callback functors allocate in the
existing scene Game arena. Their members contain no host-owned resources.
Original controller and balloon NameObjs belong to the existing object graph;
LayoutActor's established boundary releases its native layout resources.

The helper removes borrowed controller or host identities from the actual
director vector/selection slots, actual states, and actual balloons during
NameObj retirement. Retail actors use kill() while retaining their storage;
native deletion of a participant in an active talk outside scene retirement
fails loudly instead of inventing camera/demo/nerve cancellation behavior.

Required integration points, owned by the root agent:

1. SceneObj_TalkDirector creates actual TalkDirector under its scene Game scope.
2. capture_after_init runs after successful init, while that scope is active.
3. begin_retirement runs before destroying scene NameObjs.
4. release_name_obj joins the existing NameObj registry release boundary.
5. Generic GX callback registration lifetime must cover the entire original
   factory/init and unregister callbacks on rollback/retirement before their
   scene arena disappears.

The last point must cover partial init: TalkDirector's constructor leaves
mPeekZ/mBalloonHolder/mStateHolder indeterminate until init assignments, and
its draw-sync callback is registered before initBranchResult can throw. A
rollback must not read those fields as though init completed. The generic
callback registration scope can retire a registration without those reads.

## Activation/removal plan

Once original text and GX dependencies link, remove TalkRuntime.cpp/.hpp and
TalkCompat.cpp; remove SceneNameObjUtilCompat's duplicate pauseOffTalkDirector.
The original TalkDirector/TalkUtil methods then supply all game behavior.
Remove old owned_talk_ctrl/has_owned_talk_ctrl test inspection and the actor
release_talk_runtime_state hook; actual ownership is already scene/actor graph
ownership and the NameObj borrowed-reference hook replaces that teardown role.

Remove the entire OriginalDemoUtil.cpp and enable existing unchanged
Game/Util/DemoUtil.cpp. Their only remaining difference is the original talk
owner include/getter and three queries. The original queries explicitly allow
an absent SceneObj_TalkDirector and return false/null in that case.

Tests must inspect actual TalkDirector/TalkNodeCtrl state and owner lifetimes.
The old TalkRealOrAbsent and GatewayDemoScene fixtures currently depend on the
synthetic presentation API and cannot establish original conversation success.

## Remaining link frontiers

native-unresolved-vs-current-archives.json compares the directly compiled
objects with current native archives, not with future source changes. Besides
TextFormer and GX, most missing symbols are the original talk branch queries
from EventUtil. A full EventUtil migration can replace EventUtilCompat instead
of adding dozens of wrappers. Native EventUtil currently has several old
recoveries absent from decomp; those must be audited reference-first before
activating the whole original source.

Other boundaries are actual hide/showScreen and text-width adjustment,
YesNoSelector helpers, endNPCTalkCamera, the original actor water-offset query,
isStageStatePowerStarAppeared through StageStateKeeper, and two audio helpers.
These are generalized owners/API frontiers, not reasons for talk-specific
fallback state.

## EventUtil reference completion and duplicate removal preparation

The complete EventUtil source was audited against the retail assembly and
recovered in decomp, then copied to native. Its five previously missing
methods and commented-out coin accessor are now present. The Luigi
absent-or-hiding query had its branch reversed in the reference; the retail
returns the absent result when true and asks about hiding only when false.
That correction matches 100%. The supply/comet short-circuit chains retain
the exact original call order and now match 100%/99.375% respectively.

All 179 EventUtil methods are compared, with 99.30% weighted instruction
match. The reference configure entry is Equivalent. Combined DOL proof now
covers 7,315 instructions across eight TUs. Exact source hashes and the five
reference files ready for checkpoint are in reference-checkpoint-manifest.json.

Full source activation remains held. EventUtil-current-provider-overlap.json
records the current strong-symbol duplicates to remove at activation: all
of EventUtilCompat, three MR counters from OriginalSceneCounterQueries,
decPlayerLeft from OriginalPlayerEventUtil, isStarCompleteAllGalaxy from
StorySequencePlatformCompat, and seven event/comet/BGM queries from
StageSessionGameCompat. The actual EventDirector family and the original
GameSystem/SceneController selected-scenario query remain real dependencies.
EventUtil-unresolved-vs-current-archives.json records the remaining provider
symbols; no substitutes for those owners were added here.

## AlreadyDone owner consolidation

Removed StageSessionState's manual setup_already_done_flag and
update_already_done_flag table algorithms. The original
GameDataTemporaryInGalaxy and AlreadyDoneFlagInGalaxy methods already exist;
the missing GameDataFunction setup/update now forward to that exact owner via
the established temporary-data boundary in OriginalSceneCounterQueries.
The temporarily active EventUtilCompat methods also use those forwards,
pending wholesale EventUtil activation.

OriginalSceneCounterOwnerTests now constructs real one-field BCSV placements
and exercises the public MR setup/update path through both original owners.
It checks the original masked message-name hash, zone/link keys, duplicate
identity after a packed bit update, distinct keys, and all 64 entries. Removed
the duplicate direct-native-registry block from TalkRealOrAbsentTests. Tests
no longer demand invented exceptions after overrunning the original table.
All changed source and fixture translation units compile directly; the target
to run is smg-pc-original-scene-counter-owner-tests. Linked execution has not
yet been recorded for this new fixture.
