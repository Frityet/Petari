# Exact NerveExecutor synchronization

PC now compiles the byte-identical retail `Game/System/NerveExecutor.hpp` and `.cpp` rather than a host rewrite with null-safe `setNerve`, `isNerve`, and `getNerveStep` fallbacks.

The retail contract is explicit: callers may update an uninitialized executor, but querying or changing a nerve requires a preceding `initNerve`. Keeping that contract exposes invalid lifecycle ordering instead of silently returning a fabricated step or false state.

## Exactness

- header SHA-256: `e45056380d44412818feaa6b2272edb25fcac64500ab360a5a3e8229007fe212`
- source SHA-256: `4256bfb674d4f33b1e8e1422129da25ba4331c8e3fe6136b473552a66d76c191`
- RMGK02: 308/308 code bytes, 16/16 data bytes, 7/7 functions, 100% fuzzy match
- both PC files compare byte-for-byte equal with root

## Verification

- all 77 configured Game source-mirror pairs pass
- Aurora-native tests pass 27/27, including initialized nerve, pending replacement visibility, and replacement execution
- the existing layout, demo, FileSelect, story, restart, and real-disc SphereSelector suites remain green with the exact contract
- the full `smg-pc` debug executable links

No Mario creator, factory route, compatibility fallback, or debug-only behavior was added.
