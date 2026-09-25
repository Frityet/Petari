# Compat removal: process, save, J3D, and archive owners

This batch deletes another 34 files from `src/compat/`: 132 of the original 330 are removed, with 198 remaining. Complete removal remains the priority. This checkpoint does not complete that objective or establish the full Gateway encounter.

## Changes

- Restore the complete original `Game/Util/SystemUtil.cpp`. Async entry points retain the necessary Aurora guest-thread scopes. Particle and restart queries now use the actual original GameSystem fields. `FileLoader` and `FunctionAsyncExecutor` own their native shutdown in their destructors; FileLoader's static retirement entry preserves the existing self-worker preflight before deletion. The existing placement-zone scope implementation lives beside its scene header. Eight compat files are deleted.
- Restore the complete original `GameDataFunction`, the original dynamic-story record insertion, and the complete StaffRoll source. Remove the alternate GameDataSession, current/backup overrides, host construction helpers, and remaining replacement query providers: nine compat files. The actual SaveDataHandleSequence owns current/backup files. StaffRoll's factory integration remains pending; restoring its implementation does not prove endgame playback.
- Consolidate fourteen split J3D/JMath providers into nine canonical SDK translation units. Restore missing original owner methods, retain required native pointer/endian/lifetime adaptations and explicit PPC arithmetic, and restore original inline vector interpolation. No game-specific behavior is added to the SDK.
- Consolidate JSU stream and JKR archive implementations into seven canonical SDK owners, deleting three compat files. Raw stream operations retain native-byte behavior; explicit packed-scalar helpers preserve Wii save bytes. The SDK volume lock replaces the compat lock. Both archive-name parsing overloads reject a 256-byte component before writing its terminator; 255-byte components remain valid. Existing native RARC machinery is retained; this does not claim every unimplemented SDK archive operation is complete.

## Validation and limits

The integrated application builds. `validation.json` records 19 passing focused targets and the exact bounded runtime evidence. The real-disc run uses a fresh save and Metal, completes 600 original GameSystem frames, and retires normally. The inspected frame-480 image shows Mario and the Luma during the opening scene. It does not establish the later Rosalina encounter or galaxy completion.

Save, story, counter, talk, particle and auto-effect fixtures use the real process rather than publishing substitute Game owners. Save probes restore current/backup state in place through original serialization, preserving the complete PLAY value. The particle checks cover 3,327 particles, 225 textures, 2,591 metadata rows and all case-folded groups, plus backing retirement. Initial failed fixture checks and debugger evidence are retained alongside corrections. The animation parser test now checks the original final-block cursor contract; no parser change was required. The native session test only asserts its remaining services. The obsolete standalone RuntimeContext construction fixture reaches an original MessageHolder load without a FileLoader in both the committed and dirty implementations. Its replacement tests constructor failure cleanup only; successful preview bootstrap is not claimed. It avoids assumptions about unrelated pending scene-service removal.

The standalone FileLoader target exercises real queues, callbacks and repeated retirement, but explicitly compiles original FileUtil in its fixture. Production FileUtilCompat and its ArchiveMountService/ResourceHolderService closure still require a separate migration. Passing this target must not be read as removal of those services.

The configured archive-provider audit finds zero duplicate strong providers, zero stale source providers and zero unavailable owner inputs. Its broad approval gate remains red: 7,781 providers lack reviewed provenance, and existing anchored source checks include 11 unresolved and two differing entries. A full-suite pass is not claimed. Agent notes distinguish source comparisons from executable tests.

## Publication

Unrelated staged notes, dirty source/tests, scene deletions and submodule state were present at the start. A separate temporary index stages only this batch; the shared test build file receives only the seven required process-fixture dependencies and the obsolete RuntimeContext target rename. The original index patch is checked before and after committing. Baseline snapshots, private temporary save data and bulk generated provider tables remain local. Publication records include the verified remote commit.
