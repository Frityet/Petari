# Original authored demo-group creation

The real original-process run before this correction reached `RunawayRabbitCollect::init` once and `RunawayTico::initAfterPlacement` three times. All three Ticos remained alive with `mObjArg1=0` and `mDemoCastID=0`; `RunawayTico::exeGuide0` was never entered over 300 completed process frames. Evidence and reproducible LLDB commands: `opening-state.log`, `opening-state.lldb`. The final debugger exception is the intentional worker cancellation during bounded teardown, not an opening exception.

The original native placement loop requests a creator for every `DemoObjInfo` row. The native factory lacked the reference's generic `DemoGroup -> DemoExecutor` and `DemoSubGroup -> DemoCastSubGroup` entries (`decomp/src/Game/NameObj/NameObjFactory.cpp:5987`). Consequently those original rows were silently skipped and actors could not join any demo cast group. In particular RunawayTico's existing init only selects the authored Guide0 nerve after successful cast registration; without the group it never starts Mario's wake-up demo.

Added the exact two generic factory constructors and their headers in `src/scene/nameobj/NameObjFactory.cpp`. The already present original DemoExecutor, DemoCastSubGroup, DemoDirector and sheet keepers own all initialization and behavior. No Game source, stage name, story flag, timing, or actor-specific workaround was changed. The change applies to every original stage with these placement types.

Validation of the updated executable is pending the parent integrated build. Do not infer successful wake-up or rendering from this source correction.

## Tower actor import

Imported `HeavensDoorDemoObj.cpp` and `.hpp` byte-for-byte from the existing reference source/header, and registered all three exact reference factory rows: HeavensDoorAppearStepA, HeavensDoorInsideCage, HeavensDoorInsidePlanetPartsA. The first is an authored cast for the tower part that precedes Rosalina's appearance. Its existing Game actor chooses animation, demo actions, collision, sound and stage effects through the ordinary MapObjActor and DemoDirector systems. The actor's own original object-name decisions are preserved in Game rather than recreated at a compatibility boundary. No decomp changes were needed. The generic StageEffect methods used by this class already exist natively.

Baseline executable SHA256 was `4bf438a3e7fd11a33c390c79defec4e235ee244012a4d1d6ea3dfaf20169c191`. A follow-up attempt to inspect initial planet transforms while the parent rebuild was running (`placement-state.log`) failed because LLDB's referenced object archive was being replaced. That attempt is not used as actor-state evidence; process was killed after the unresolved-variable error. Retests must wait for a completed relink.

## Updated live opening frontier

The parent fourth build passed (binary SHA256 `b7b125b3360e22f0f26cd6f7129f450746c20dd80a03541b08d0d7cc713c165d`). `opening-activated.log` verifies all three authored demo executors initialize, the three RunawayTicos acquire their correct cast IDs and guide roles, and the opening guide enters `exeGuide0`. The original DemoDirector then dispatches DemoPlayerKeeper's first `MarioDemoPos` / `demomeettico` row.

Its actual position operation next throws from `GameGravityCompat.cpp:43`: the compatibility query rejected a null NameObj. The original `MR::findNamePosOnGround` deliberately calls `calcGravityVector(nullptr, position, ...)` for this generic named position query. This new observed frontier is a compatibility boundary error; the parent owns its correction. No successful visible wake-up is yet claimed.
