# Original audio scene state under disabled output

The previous real-process run reached active GameScene frames and then stopped in
Mario's abyss-death nerve because unchanged `MR::setCubeBgmChangeInvalid()` writes
the original `AudSceneMgr::_1D` field. `AudWrap::getSceneMgr()` always threw even
though the process had already created its explicitly disabled audio backend.
The recorded stack is in
`../gateway-wakeup-demo-20260912/original-app-twenty-eighth-exception.log`.

The existing disabled object-audio service now owns an actual constructed
`AudSceneMgr`. Its constructor and Mario/Luigi selectors are exact copies of the
existing reference decompilation. The object preserves logical scene flags and
player identity, while its JAU wave heap remains absent. The audio wrapper
preserves its original player-selection argument when loading a stage.

`AudWrap::getSceneMgr()` borrows this actual service-owned object. Querying an
absent owner still throws. The scene-start adapter rejects a nonactive owner and
performs the original `_4 = 0` and `_1D = false` assignments. Retail output-only
operations on AudSystem voice flags, volume, effect and Wii-speaker owners are
omitted under the existing explicitly disabled-output policy. This does not
create an AudSystem, loaded wave bank, sound voice, rhythm graph or output device.

The existing native audio bypass was removed from `GameScene::start()`, restoring
that complete method to exact reference source. Scene transitions therefore reset
their own audio flags instead of retaining a prior stage's invalidation flag.
No Mario/death/Gateway-specific fallback was added.

## Evidence

`source-equivalence.json` records exact source equality and hashes for the
constructor, both player selectors and the restored complete GameScene start
method. This is source identity evidence, not a new retail object-match claim.
No reference decompilation was modified.

`OriginalJaiSoundOwnershipTests --scene-only` is a self-contained regression for
the actual owner, original initial fields, unchanged SoundUtil reads/writes,
scene reset, preserved player identity, nested owner publication, absent/nonactive
owner rejection and three heap retirement cycles. `--backend-only` also checks
scene-owner publication from the original OS initialization worker, real wrapper
player selection, flag reset and retirement alongside existing name-resource,
bank-request and reset checks. Root coordinates builds to avoid concurrent Xmake
mutations. Build/runtime results will be recorded after execution.

The native production build and the focused test target passed. Both
`--scene-only` and `--backend-only` passed (the latter uses the retained local
retail audio fixture). The integrated real-disc run passed ordinary
`GameScene::start()` and entered the authored opening demo. Its next exception
was the independent gravity query restriction, not scene audio state.

## Remaining audio boundary

Direct `AudWrap::getSystem()` requests remain explicit unsupported operations;
no substitute AudSystem is introduced. In particular, unchanged SoundUtil
submit/permit-SE functions write AudSystem `_82B`/`_82C` directly, while volume,
limited-sound and chord queries require other original audio owners. Those
interfaces are separate from this implemented logical scene-state contract.
Full wave-bank loading, effects, rhythm and audible output remain disabled.
