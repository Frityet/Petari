# Round 9 OceanHomeMapCtrl

Restored the entire current donor OceanHomeMapCtrl source and retained its matching header. Native differences are only the compile-time CP932 wrappers on four original Japanese narrow literals and an explicit cstring include. The original planet name tests, underwater low-model creation, original movement/clipping/LOD visibility decisions and empty destructor are preserved. This removes the former specific unsupported-ocean exception.

Enabled the exact original `SceneObj_OceanHomeMapCtrl -> new OceanHomeMapCtrl()` case in the currently compiled SceneObjHolderCompat factory. The archival Game/Scene factory already has that case but is not the selected provider. The edit is just an include and one case, captured separately in `scene-factory-scoped.patch` after snapshotting its complete pre-edit state.

Deleted all of PlanetMapRuntimeCompat. Its remaining MR submodel/model/LOD functions are restored by the parallel full LiveActorUtil owner change; this batch does not duplicate them. The complete donor source only needs those existing original interfaces.

Native retirement uses the existing generic NameObj ownership: SceneObjHolder's transaction owns the controller; NameObj construction registration and original placement child ownership/GameSceneBinding retire the low-water ModelObj. Adding a controller-specific delete would risk double ownership, so the donor destructor remains empty.

No builds, tests, Git operations or xmake edits performed. Canonical Game sources use the existing recursive build glob. Snapshot/hashes and exact owned paths are in `owned-manifest.json`.
