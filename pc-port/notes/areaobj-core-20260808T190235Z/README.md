# Exact AreaObj core on PC

This wave restores the retail AreaObj form and manager implementation without adding host behavior to `src/Game`.

## Exact Game boundary

The following PC files are byte-identical to their root decomp source/header counterparts:

- `Game/AreaObj/AreaForm.{hpp,cpp}`
- `Game/AreaObj/AreaObj.{hpp,cpp}`
- `Game/AreaObj/AreaObjFollower.{hpp,cpp}`
- `Game/Util/AreaObjUtil.{hpp,cpp}`
- `Game/Map/SleepControllerHolder.{hpp,cpp}`

`tests/AreaObjCoreTests.cpp` compares all ten pairs at runtime, so this is a maintained source-boundary invariant rather than only a one-time audit. A direct `cmp` verification also reported `10/10 AreaObj core source-boundary pairs byte-identical` on 2026-08-08 UTC.

The retail RMGK02 symbol table and `AreaForm.cpp` data show only the four concrete derived vtables, each exactly `0x10` bytes with concrete `init` and `isInVolume` slots. There is no base `AreaForm` vtable and there are no base method bodies. The root decomp header was therefore corrected to mark both base methods pure virtual, and the exact PC mirror carries the same correction. This supplies the host ABI naturally without a compatibility stub or invented base behavior.

## Compatibility boundary

Retail `AreaObjMgr::find_in` uses the Metrowerks-only spellings `std::mem_func` and `std::rfind_if`. These were added as a generalized Aurora compatibility header at `aurora/include/functional.hpp`; the independent nested Aurora commit is `7b80ef30bb813f544637fb9350bf99644babc7a6`. `MetrowerksStdCompat.hpp` now consumes that header instead of carrying a PC-port-local `mem_func` workaround.

The excluded retail math translation units are closed by generalized host providers for the exact `MR::isInRange` interval semantics and `MR::tmpMtxRotXDeg`, `tmpMtxRotYDeg`, and `tmpMtxRotZDeg` matrix behavior. The implementations follow the recovered retail bodies and remain outside `src/Game`.

The compatibility probe covered both zero-argument `std::mem_func` use with `std::for_each` and the exact one-argument `bind2nd(mem_func(...))` reverse-search expression. Both C++23 probes returned exit status 0.

## Behavioral coverage

The focused AreaObj core target verifies:

- rotated and non-uniformly scaled cube containment through a follow matrix;
- exact Cube2 base-origin and exclusive upper-bound behavior;
- followed cylinder axis/radius/height behavior;
- recovered temporary degree-matrix sign conventions used by `AreaFormCylinder::calcDir`;
- reverse entry priority for overlapping `AreaObjMgr` members;
- continuation to the next earlier real area when a later entry is invalid;
- absence (`nullptr`) when every overlapping area is inactive.

No placement creator or stage-specific path is enabled by this core wave. Scene ownership and strict supported-object registration remain in the generalized AreaObj runtime layer.
