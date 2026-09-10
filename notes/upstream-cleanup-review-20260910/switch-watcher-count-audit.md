# SwitchWatcherHolder movement count — independent review

Read-only comparison of the incoming `decomp/src/Game/Map/SwitchWatcherHolder.cpp`, original Array.hpp records, current native source and retail `notes/gateway-audit-20260907/restoration/retail/asm/Game/Map/SwitchWatcherHolder.s`.

The incoming `mSwitchWatcher.mArray.callAllFunc(&SwitchWatcher::movement)` is incorrect. `mArray` is `MR::FixedArray<SwitchWatcher*,256>`; its begin/end iterate all256 elements, including unpopulated slots. The surrounding `MR::Vector` has the actual populated `mCount`; Vector::end returns `&mArray[mCount]`.

Retail `SwitchWatcherHolder::movement` at8019F558–8019F5DC:

- `8019F58C` loads the count at holder+0x40C.
- `8019F594` multiplies count by four and `8019F5A0` forms a cached end pointer from holder+0x0C.
- `8019F5AC–8019F5B8` invokes the watcher method through the original member-function pointer.
- `8019F5BC` increments one pointer; `8019F5C0` compares against the cached end in r30.

Thus the current native expression is appropriate:

```cpp
std::for_each(mSwitchWatcher.begin(), mSwitchWatcher.end(), std::mem_func(&SwitchWatcher::movement));
```

For a reference-compatible explicit expression with no dependence on a removed standard-library helper, the same behavior is:

```cpp
SwitchWatcher** end = mSwitchWatcher.end();
for (SwitchWatcher** it = mSwitchWatcher.begin(); it != end; ++it) {
    (*it)->movement();
}
```

Snapshot end before callbacks. Unlike this movement routine, the retail findSwitchWatcher routine reloads the count at each comparison; do not copy that loop's changing bound into movement. A watcher appended by a callback is intentionally deferred until the next movement.

No source was changed or compiled by this reviewer. Initialization agent owns the reference correction and proof. Useful bounded runtime coverage is empty storage, a partially populated array with inaccessible unused entries, and a callback that appends one watcher: only the original count should run during that first movement.
