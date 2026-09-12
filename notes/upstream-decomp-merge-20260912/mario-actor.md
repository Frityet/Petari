# Mario actor merge resolution

Merged the assigned cohort from ours `ad4f2b1132248520f4b3c00c03a39c966328bcfe`
and upstream `cae223c32` against base `d1ae0a05c`. The files in
`resolved-files.json` are staged in `decomp`; no commit was made by this agent.

The resolution preserves every method present in either parent across all eleven
conflicted actor translation units. `method-preservation-check.json` records the
inventory; `method-resolution.json` records the individual earlier recoveries.
The upstream addition is eighteen methods: `updateGravityVec` and seventeen
SpecialDraw/MarioState methods. Local copies of the three Git stages are review
scratch files, not necessary checkpoint artifacts.

| File | Resolution |
| --- | --- |
| MarioActor.hpp | Accept upstream named/typed members and PlayerMode values while retaining our declarations. Use one `mMaskTextures[2]` array. Remove the automatic merge's duplicate `updateTeresaAnimation` declaration. |
| MarioActorDraw.cpp | Keep the automatic merge's upstream typed DLchanger, DisplayListMaker and J3D access, plus our existing methods. Remove the obsolete local mask-array alias. Retain our pre-merge ten-instance NrvMarioActor block, which reconstructs Draw's retail static initializer; it is not the sole source definition of those instances. |
| MarioActorGravity.cpp | Add upstream `updateGravityVec`; retain our recovered `updateBeeStickMode` body. |
| MarioActorMorph.cpp | Accept the correctly named sensor type; retain our `changeMorphString` recovery and adopt the named normal player mode. |
| MarioActorDefensiveMsg.cpp | Retain our four recovered receiving/cylinder methods and required includes, while incorporating upstream changes to preexisting methods and named player modes. |
| MarioActorOffensiveMsg.cpp | Retain our attack/push, trample, cylinder-push and item recovery bodies; incorporate the upstream preexisting-method changes and named modes. |
| MarioActorParts.cpp | Retain our six recovered methods and their required math includes; keep upstream changes to other methods. Preserve the valid `_468 == 0` integer comparison rather than upstream's `nullptr`. |
| MarioActorRush.cpp | Retain our begin/end recovery, including the explicit animation overload; incorporate upstream other-method changes and named modes. |
| MarioActorRushMsg.cpp | Retain our three recovered selection/start methods; keep upstream's correctly named sensor constants in the other methods. |
| MarioActorSensor.cpp | Keep upstream's corrections confirmed against retail below, retaining all eleven existing methods. Adapt its offset-0x68 Rabbit access to the retained named `mJumpAnimationIndex`. |
| MarioActorSpecialDraw.cpp | Add all seventeen upstream-only methods; retain our five recovered dark-mask/spin methods and their original operation order. Use the canonical mask array. |
| MarioActorTakeMsg.cpp | Retain our pull-vector and tornado-pull recoveries plus their includes; incorporate upstream preexisting-method changes. |
| MarioActorInit.cpp | Rename the two mask-array initialization accesses to `mMaskTextures`, with no behavioral change. |
| FurMulti.hpp | Remove the automatic merge's second, incompatible `FurMulti : LiveActor` declaration; retain the complete original helper definition and methods. |

## Verified semantic corrections

`retail-semantic-proof.json` records bytes from the actual RMGK01 DOL, SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`.

- Draw texture formats are RGB565 (4) and IA4 (2), loaded into r6 at
  `802B5BC8` and `802B5BFC`. The previous source used RGBA8 and IA8.
- Sensor's eye-radius condition uses MovementStates bit 15 from offset 8
  (`802BECFC..802BED00`), so upstream `_F` is correct; the previous `_2F`
  queried another word.
- Trample inhibition reads offset `0x1C` bit 6 (`802BF07C..802BF084`), and its
  timer comes from MarioConst offset `0x3A4` (`802BF0C0`), matching upstream's
  `_1C._6` and `mAirWalkTimeTornado`.
- The DOL strings at `805B8CF4..805B8D51` confirm upstream's three stomp jump
  animations, two Hopper stomp animations, voice and effect. The prior source
  used unrelated spin/jump strings.

No additional unverified behavioral alteration was introduced to resolve a
conflict. The full upstream-only recoveries remain upstream work, not a claim
of new runtime validation by this merge pass.

## Validation

All twelve affected actor source files (eleven conflicts plus Init) compile with
the project's actual Metrowerks compiler and current merged headers. Each
command and exit code is in `compile-results.json`; individual logs are beside
it. Outputs went only to this note directory, leaving the root Ninja lane free.
The compiler reports existing nontrivial-union warnings in the merged actor
header; all twelve final exit codes are zero. The final SpecialDraw probe was
rerun after the FurMulti declaration fix.

The staged cohort passes `git diff --check`. This establishes source merge and
original-compiler closure, not a linked game or gameplay result. Root owns the
aggregate Player build and merge publication.

## Nerve initializer retention clarification

The earlier uniqueness claim was incorrect. Twelve Player translation units
currently contain the same ten-instance block, and every one already contained
it in `ad4f2b113`. `nerve-instance-source-audit.json` records those files.
`INIT_NERVE(name)` expands to `name name::sInstance;` in `NrvMarioActor`, so it
defines an external static data member, not a private instance per translation
unit. Inspection of the actual Metrowerks Draw and Wall objects confirms both
emit strong global `B` symbols for those instances. These are not weak symbols.

The retail Draw and Wall assembly each contains a local static initializer
(`802B6FE8` and `802F7B5C`) that initializes the same retail Wait instance at
`806B39F8`, alongside the other nine instances. Retaining Draw's block preserves
our existing reconstruction of that per-file initialization. It does not prove
that the duplicate raw-object definitions are suitable for a final source link;
that preexisting linkage issue was not introduced or resolved by this merge.
`nerve-instance-linkage-audit.json` records the macro, object symbol output and
retail addresses. This clarification changes notes only, without expanding the
merge into nerve ownership cleanup.
