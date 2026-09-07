# Wipe animation owner boundary audit

The native-only declaration `MR::isAnimStopped(LayoutActor*,u32)` is incorrect. The original public API is `MR::isAnimStopped(const LayoutActor*,u32)` (`Util/LayoutUtil.s`, 0x803D8D68, 60 bytes). It obtains the manager, obtains the root pane controller with a null pane name, then calls that controller's `isAnimStopped`.

`LayoutPaneCtrl::isAnimStopped` at 0x8036A604 (16 bytes) selects the already-owned player by layer and tail-calls `LayoutAnmPlayer::isStop`. The latter is independently restored/proved in the Group cohort: a null animation transform is stopped. A real initialized controller with no animation yet is therefore stopped; that is distinct from missing layout resources or an invalid layer.

The native wrapper currently calls `LayoutRuntime::isAnimStopped`, whose standalone contract rejects absence of an active BRLAN. The compatibility boundary knows the original initialized manager/controller/layer and should retain those validation checks, then return true for that real owner's unstarted layer. Pane controls similarly must distinguish the root's actor animation from a named pane animation and preserve the real default stopped state. No placeholder animation transform is needed.

Scene wipe construction also exposes a general rollback requirement: children join `IgnorePauseNameObj` before later child resources may fail. The captured NameObjs are rolled back, but `release_name_obj_runtime_state` currently only erases the registry entry, leaving borrowed group slots. A generic membership retirement boundary is needed so a surviving group cannot later dispatch a deleted member. Normal scene teardown currently destroys members before the original group array, whose destructor does not dereference them.

This audit is notes-only while the Group/pointer runtime fixtures are being fixed; no native wipe production files have been saved.
