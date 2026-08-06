# Switch watcher decomp audit

Date: 2026-08-06 UTC

## Scope

The listener chain was checked against the retained RMGK02 objects and assembly:

- `Game/Map/SwitchWatcher`
- `Game/Map/SwitchWatcherHolder`
- `Game/Map/ActorAppearSwitchListener`
- `Game/Util/SwitchEventFunctorListener`

Only `src/Game/Map/SwitchWatcherHolder.cpp` required a source correction. No `pc-port/src/Game` file was changed by this audit.

## Assembly-backed semantics

`SwitchWatcherHolder` is a counted, append-only array for the life of a scene.

- `movement` (`0x8019F558..0x8019F5E0`) computes the exclusive end as `this + 0x0C + mWatcherCount * 4` and invokes `SwitchWatcher::movement` only over that range. It does not traverse all 256 capacity slots.
- `findSwitchWatcher` (`0x8019F604..0x8019F67C`) uses the same counted end and compares each watcher's `StageSwitchCtrl` by pointer identity.
- `addSwitchWatcher` (`0x8019F704..0x8019F720`) reads the old count, increments the stored count, and writes at the old-count slot. The previous source used pre-increment and left slot zero unused.
- `SwitchWatcher` polls A/B/Appear listeners independently with flag bits `1`, `2`, and `4`. A callback fires only on an observed edge; the corresponding bit is then set or cleared.
- Adding another listener of the same kind replaces the prior pointer. Multiple kinds for one `StageSwitchCtrl` share one watcher.

The matching source form for holder movement is a counted `std::for_each` with `std::mem_func(&SwitchWatcher::movement)`. This reproduces the pointer-to-member temporary/call sequence in the original assembly.

## Listener behavior and ownership

- `ActorAppearSwitchListener::listenSwitchOnEvent` appears a dead actor only when its on flag is enabled.
- `ActorAppearSwitchListener::listenSwitchOffEvent` kills a live actor only when its off flag is enabled.
- `SwitchEventFunctorListener::setOnFunctor` and `setOffFunctor` clone the supplied functor with a null heap argument; event methods invoke a clone only when non-null.
- The original destructors for `SwitchWatcher` and `SwitchWatcherHolder` only run the `NameObj` base destructor. They do not delete the control, listener pointers, watcher objects, or cloned functors. `SwitchEventListener` itself has no virtual destructor in this source.

That lifecycle is consistent with scene-scoped allocation in the original game. A host port should preserve validity for the whole polling scope and reclaim registrations/allocations at general scene teardown; it should not infer per-actor ownership or delete listeners through `SwitchEventListener*`.

## Verification

The restored RMGK02 build graph compiled the four audited units successfully:

```text
ninja build/RMGK02/src/Game/Map/SwitchWatcherHolder.o \
      build/RMGK02/src/Game/Map/SwitchWatcher.o \
      build/RMGK02/src/Game/Map/ActorAppearSwitchListener.o \
      build/RMGK02/src/Game/Util/SwitchEventFunctorListener.o
```

The focused objdiff report is reproducible with:

```text
build/tools/objdiff-cli report generate \
  -p pc-port/notes/switch-watcher-decomp-20260806T191255Z \
  -o pc-port/notes/switch-watcher-decomp-20260806T191255Z/report.json \
  -f json-pretty
```

The generated `report.json` records 100% fuzzy/code/function/data matches for all four units:

| Unit | Code | Functions | Data |
| --- | ---: | ---: | ---: |
| `ActorAppearSwitchListener` | 244 / 244 | 4 / 4 | 24 / 24 |
| `SwitchWatcher` | 588 / 588 | 6 / 6 | 56 / 56 |
| `SwitchWatcherHolder` | 728 / 728 | 11 / 11 | 72 / 72 |
| `SwitchEventFunctorListener` | 228 / 228 | 5 / 5 | 24 / 24 |

An attempted repository-wide `ninja all_source` reached unrelated camera compilation failures (`CamTranslatorSpiral::getNum1High` and `MR::startSystemSE` declarations). The focused watcher objects compiled before those failures and do not depend on them.
