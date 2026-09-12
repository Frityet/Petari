# Complete original SoundUtil activation, 2026-09-12

## Changes

`Game/Util/SoundUtil.cpp` now supplies the complete original utility TU, including three missing methods recovered first in the reference source and then mirrored byte-for-byte. The original AudTalkSoundData table and lookup source were imported without modifications. Thirteen canonical Game audio headers provide the types required by the complete TU. No audio singleton or successful voice is invented. The general AudWrap system-SE accessor now rejects an absent disabled-system owner rather than allowing null-member dispatch into a method that ignores its this pointer.

Five duplicate providers were deleted (555 lines): SoundUtilCompat, OriginalActorSound, OriginalObjectSoundRequests, DisabledObjectSoundRequests and OriginalSubBgmQueries. Their Game decisions now come from SoundUtil. The original object-level request methods obtain a handle, check it for null, and only then update its JAISound lifetime. The generic disabled JAU/Aud backend returns null, so the dedicated fake lifetime helper is unnecessary and was deleted with its provider.

The restored methods observe actual AudSystem, AudSceneMgr, AudBgm and sound-name table ownership. Logical event telemetry no longer substitutes for these Game fields or claims voice readiness. The existing generic disabled object sound backend remains disabled; BGM retains its separate existing backend. Root owns the SoundUtil source-list activation.

## Reference recovery and validation

Retail source evidence: `decomp/build/original-scenario-catalog-20260903/retail/asm/Game/Util/SoundUtil.s`, plus the current split original object. Commands and object diffs are adjacent.

- startTalkSound: 99.875%. Uses the original AudTalkSoundData ID lookup, skips anonymous 0xFFFFFFFF, then routes to the actor original startSound overload or the system sound object's startSoundParam with both parameters -1.
- getMapSoundCodeFoot: 100%. Preserves the retail Binder subobject-address tests and ground/roof/wall result priority exactly, including the original null-Binder -1 result.
- isStopOrFadeoutStageBgmID: 100%. Missing BGM, another ID, missing handle, a nonzero actual JAISound fade transition, or stopping return true. The removed host substitute only inverted isPlaying and therefore missed a live fade.
- Existing moveVolumeStageBGM and NoteFairy variant: 100%. Forward to the actual current BGM virtual method when present.
- Imported AudTalkSoundData::getSoundIDFromTalkSoundNo: 100%.

Both full Game TUs compile under the original compiler and native Clang. The final AudioFacade owner guard and test sources also compile (see owner-guard-compile-results.json). The existing OriginalJaiSoundOwnershipTests TU compiles with new coverage. Exact MR symbol audit finds no duplicates after the five providers are retired; see `symbol-audit.json`.

## Runtime evidence

A notes-only harness calls the new `test_original_talk_sound_dispatch` function from the existing OriginalJaiSoundOwnershipTests. It links the complete new SoundUtil and AudTalkSoundData objects against current Game/Aurora archives and passes (exit 0). It checks anonymous and out-of-range IDs produce no request, retail rabbit/final-table IDs, valid system and actor dispatch, original map-code update, null object/level handles and two disabled owner generations. The owner-guard test rejects a valid request before owner creation and after every retirement without increasing the declined-request count; the final runtime rerun also passes. The actor fixture is an actual LiveActor borrowing an actual locally owned AudAnmSoundObject, with the actual JKR process heap retained for both audio object lifetimes. No MR provider is mocked and no fake sound voice is attached.

Evidence: `talk-probe-compile-command.json`, `talk-probe-link-command.json`, `talk-probe-run.log`. Normal integrated target: `smg-pc-original-jai-sound-ownership-tests`.

After the parent finished its shared archive rebuild, both the final talk dispatch probe and the complete existing JAI ownership fixture linked and ran successfully (exit 0). The complete fixture exercises actual native PCM parameters/lifetime and reciprocal handles; the original stream wrapper and three-slot pool; canonical handle transfer across three owner generations; and actual retail Stage/Sub preparation, playback, category control and retirement. `final-runtime-results.json` records both runs; `full-test-run.log` contains the complete successful output. An earlier transient link attempt saw the archive temporarily absent during the parent's rebuild; that attempt is superseded by this final passing run.

## Remaining lower owners

The archive snapshot lacks 17 lower method definitions: AudSoundNameConverter lookup; AudMeNameConverter lookup and AudMeObject dispatch; AudSystem chord/volume/limited-sound methods; CSSoundNameConverter and AudSpeakerWrap; AudSeKeeper; AudEffectDirector; AudMicWrap; and AudRemixMgr/Sequencer. Full names are in `symbol-audit.json`. The two reported live Talk calls (startTalkSound and moveVolumeStageBGM) have their original implementations now.

Named sound paths still require real AudSoundNameConverter/JAUSoundNameTable startup. Current AudioFacade explicitly rejects unavailable AudSystem/AudSceneMgr access. Existing logical-only SoundUtil fixture expectations require migration when those owners are integrated; they are not a reason to reinstate the deleted substitutes. The new bounded proof claims original dispatch to disabled objects, not audible playback or full Game audio startup.

Reference publish list: `reference-publish-list.txt` (only decomp src/Game/Util/SoundUtil.cpp changed). Native sources, canonical headers, test and deletions are recorded with hashes in `source-manifest.json`. No shared build files or commits were changed by this lane.

## Bounded native publication audit

The root shared full JAI fixture has a final exit-0 receipt with binary SHA-256 `d93fd4483545431cb46bf49074857f62bcd2f2adb327bc02e29ee2a676366602` at `notes/original-game-system-startup-20260912/original-jai-sound-ownership-tests-final-run.json`. The owned-source manifest remains unchanged and verified. All four owned translation units also compile against a clean exported committed HEAD plus only this Sound cohort and the single intended SoundUtil activation; no unrelated WIP source/header is needed for that compilation. See `publication/README.md`, the precise native publication list and independent compile receipts. The shared runtime evidence is distinguished from this isolated source-compilation proof.
