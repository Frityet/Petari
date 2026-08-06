# RMGK02 RunawayTico reconstruction

## Outcome

`src/Game/NPC/RunawayTico.cpp` now contains the complete source-level actor
behavior represented by `build/RMGK02/asm/Game/NPC/RunawayTico.s`. The prior
file stopped after construction, destruction, and archive selection. The
reconstruction remains in the regular decomp tree first; it has not been
copied into `pc-port/src/Game` ahead of its general NPC compatibility
dependencies.

The actor now restores:

- placement-driven Tico color/mode selection and demo-cast identity;
- guide, transformation, white-fade, appearance, talk, and wait nerves;
- named demo actions for the first Tico meeting, rabbit chase, and tower
  appearance sequences;
- the original animated camera, start camera, BGM, level sound, and wipe
  timing;
- caught-rabbit comment branching and the final Mama-comment transition;
- player-relative caught poses and `TicoDemoPos%d` all-caught positions;
- runaway notification and stage-switch A signaling; and
- the handoff into the existing HeavensDoor Rosetta/spin-get demo parts.

The broad `Game/Util.hpp` include was replaced with the exact utility surfaces
used by this actor. `setPosAfterCaught` is marked `NO_INLINE` because the target
contains a distinct function and all four comment entry points call it; that
annotation raises those five functions to exact code matches.

## Assembly-backed details

The RMGK02 data establishes the exact action/demo strings, including:

- `チコとの出会い[開始]`
- `チコとの出会い[チコ変身]`
- `チコとの出会い[ウサギ逃走]`
- `ウサギ追いかけ[開始]`
- `高楼出現[フェードイン]`
- `高楼出現[デモ]`
- `チコガイドデモ`
- `ぼやき`

The retained assembly also fixes the guide timing: Tico wait sound begins at
step 210, floating sound at 560, zoom-out BGM at 617, zoom-out system sound
from 630 through 729, white-out over 90 frames, and the tower-demo handoff at
white-in step 75.

## Build and objdiff evidence

The source object compiles with the repository's GC/3.0a3 RMGK02 flags:

```text
ninja build/RMGK02/src/Game/NPC/RunawayTico.o
[1/1] MWCC build/RMGK02/src/Game/NPC/RunawayTico.o
```

The regular decomp is already configured for RMGK02, and the default `ninja`
build is clean (`ninja: no work to do.`).

Direct comparison against `build/RMGK02/obj/Game/NPC/RunawayTico.o` reports:

- `.text` fuzzy match: **94.61945%** across 3,784 target bytes;
- 39 target code symbols, 38 paired, and 29 exact;
- exact caught-pose/comment methods, runaway state, demo transform, position
  restoration, constructor/destructor, nerve thunks, and functor machinery;
- `init`: **93.458015%**;
- both guide executors: **99.7%+**; and
- all other paired gameplay functions at **83.9% or higher**.

The complete machine-readable comparison is [objdiff.json](artifacts/objdiff.json),
and [functions.tsv](artifacts/functions.tsv) is the compact per-symbol table.
The remaining differences are code-generation/string-pooling differences; no
missing branch or call was found in the reconstructed gameplay flow.

## PC import boundary

The next PC step should still be dependency-first: source-close `Tico`/NPC
behavior and generalized demo-timesheet, talk, camera, LodCtrl, joint, and
player-control compatibility. Registering `RunawayTico` or
`RunawayRabbitCollect` before those layers exist would turn the original state
machine into inert construction rather than advancing the playable route.
