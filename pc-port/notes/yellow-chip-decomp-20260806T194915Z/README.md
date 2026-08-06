# RMGK02 YellowChip progression-stack reconstruction

Updated: 2026-08-06T19:49:15Z

## Outcome

The root `ChipBase`, `ChipCounter`, `ChipGroup`, and `ChipHolder` stack is source-ready again. This removes the known decompilation defects that previously made a faithful PC import premature; it does not yet claim that the larger YellowChip dependency graph is available in the PC runtime.

This stack is progression-critical in HeavensDoor scenario 1. All five placed YellowChip actors use group 0, and group completion ultimately turns on global switch 1009, the `SW_APPEAR` switch for the main-zone `SuperSpinDriver`.

## Reconstructed behavior

- `ChipBase::initModel` now initializes the original chip model, scene connection, optional AirBubble parts model, and Move animation.
- `ChipBase::receiveOtherMsg` now handles item-get/show/hide/start-move/end-move messages. The target-required receiver/sender ordering passed to `requestGet` is preserved.
- `ChipCounter` now contains its full show/hide/complete/demo/layout-animation state machine, including FrameIn, Show, FrameOut, TryDemo, Complete, and CompleteOut nerves.
- `ChipGroup::updateUIRange` now uses the supported `TVec3f::add` operation in place of the stale nonexistent `addInline` call. Constructor loop form and local ordering were also brought closer to target code generation.
- `ChipCounter::requestShow` and `MR::showChipCounter` now correctly return `void`; the target defines no return value and every caller discards it.

## Match evidence

| Unit | Text size | Fuzzy | Detail |
| --- | ---: | ---: | --- |
| `ChipBase` | 3640 B | 99.7033% | 34/39 functions exact; `initModel` 99.6721%; `receiveOtherMsg` 100% |
| `ChipCounter` | 2764 B | 99.7178% | 18/30 functions exact; reconstructed methods 99.4737–100% |
| `ChipGroup` | 2548 B | 98.1586% | constructor 99.2857%; `updateUIRange` 91.8906% |
| `ChipHolder` | 1308 B | 100% | `showChipCounter` remains exact |

The remaining differences are compiler/code-generation differences rather than missing control flow. ChipCounter's `tryEndFrameIn`, `tryEndFrameOut`, `tryEndComplete`, destructor, and most nerve execution functions are exact.

## Build verification

```text
ninja
ninja: no work to do.

54b71431af0d509097bfdef4ec28617afc487e89  build/RMGK02/main.dol
54b71431af0d509097bfdef4ec28617afc487e89  orig/RMGK02/sys/main.dol
```

The complete source-built RMGK02 DOL remains byte-exact. `git diff --check` passes for all six changed root files.

## PC import boundary

A generalized PC import still needs source-close providers for `CollectCounter` layout panes/animation, demo lifecycle, the ChipHolder scene object, actor sensors/effects/clipping/sound, and the relevant rail/collection behavior. Those dependencies should be implemented as reusable runtime/compatibility substrate; the root Game actors should not gain HeavensDoor or switch-1009 special cases.
