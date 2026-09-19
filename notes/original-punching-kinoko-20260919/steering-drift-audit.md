# Steering investigation and verified original-source restoration

A live original-process trace recorded by the root task shows Mario front/velocity opposing `world_pad_direction` during part of the live chase. This is not sufficient to attribute a gameplay bug to compatibility. Neutral input and restarting from rest subsequently restored near-alignment.

Native `src/Game/Player/MarioMove.cpp` is not byte-identical to current decomp after encoding normalization. In `retainMoveDir`:

- Native line916 checks animation `ターン`; decomp line862 checks `その場足踏み`.
- Native line932 uses default threshold0.99; decomp line875 uses0.06. Both use0.1 when `_3CE < 2`.
- Native line945 sets `mDrawStates._9`; decomp line887 sets `_16`. The native and donor DrawStates name those as different bit positions.

Both implementations return false; their role here is modifying turn flags/timers (including `_D`), not directly returning a retained output vector. The native `mainMove` applies0.1/0.3 multipliers to turn speed when `_D` is set (`src/Game/Player/MarioMove.cpp:485`). No source change is justified until retail assembly/constants are checked. Native `vecBlendSphere` and vector `diffAngleAbs` are structurally consistent with donor bodies. `MarioWalk` and `MarioActorPad` match current donor after literal-only normalization.

The performance request temporarily takes priority. This drift is recorded, not fixed; no claimed causal relationship or gameplay validation.

## Retail constants and instruction check

The three drift points are now independently confirmed directly from `decomp/orig/RMGK01/sys/main.dol` (SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`), not inferred only from donor source:

- `retainMoveDir` loads its default threshold at0x802EC284 from0x806BFDE8; big-endian bytes `3d75c28f` are float0.05999999865889549 (0.06f). Its initial landing threshold from0x806BFDB0 is0.1f.
- The first animation query at0x802EC208 uses the string at0x805C8CE9, whose CP932 bytes decode as `その場足踏み`.
- Instruction0x802EC2D0 is `60630200` (`ori r3,r3,0x200`) followed by a store to Mario+0x18, the DrawStates word. MSB-first bit index0x16 has mask0x200. Native `_9` instead addresses mask0x00400000.

See `retain-move-dir-retail-evidence.json`. Restoring the complete canonical donor function is proposed; no gameplay mutation has yet been made. Full body MWCC comparison and a meaningful original-owner regression should precede claiming the restoration validated.

## Whole donor compilation

`verify-retain-move-dir.py` compiles the unchanged canonical MarioMove with the configured MWCC toolchain, preserving unrelated decomp dependency output. `MarioMove-match-summary.json` reports retainMoveDir736bytes at99.78261% fuzzy match and the full translation unit text at98.27502%. Independently, all10,792bytes of the reference ELF text match the retail DOL after masking only461 actual ELF relocation fields. `MarioMove-verified-objdiff.json.gz` preserves the detailed comparison; source SHA256 is recorded. The review-only `proposed-retain-move-dir.patch` replaces only the native function with the canonical body and explicit CP932 wrappers.

## Exact native restoration and actual-owner red/green proof

The parent authorized restoring the complete canonical function. Native `MarioMove::retainMoveDir` now equals the validated donor body after removing only the explicit CP932 literal wrappers (`retention-native-body-audit.json`). No adjacent movement functions changed.

The actual-process test uses the initialized original Mario owner and animator. It snapshots/restores every touched state field, then verifies the retail DrawStates mask, countdown and expired-retention flags, the 0.06 threshold and initial-landing 0.1 threshold, preserved output/return behavior, and copied real ground/gravity vectors. It does not create replacement owners or persist test state into subsequent game frames.

- **Red:** the previous native implementation failed at frame38 with `Retail retention marks DrawStates mask0x200, not the unrelated mask0x00400000`; exit1, PID66898 reaped. Evidence: `retention-red-process.json/log`.
- **Green:** the restored canonical implementation passed all cases, original sensor ownership/message/retirement checks twice, and independent actual-camera pointer-plane checks. The original process completed120frames, exit0, PID67791 reaped,3.310s. Pointer coverage:30moving,54stationary,30second-ray-only parallel cases. Evidence: `retention-green-process.json/log`; binary SHA256 `07eeb68b29fd4a7a3b3f556cf670e220ccd021dd58cd283ac707358649359f26`.

The corrected build is `retention-green-build.log` (PASS4.169s). Earlier test compilation and behavioral failures remain preserved. This proves the bounded original-source/state restoration. It does not establish that the previous chase trajectory was caused by these differences or demonstrate rabbit/story progression.
