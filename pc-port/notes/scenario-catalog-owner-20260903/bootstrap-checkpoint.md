# Resource-ready catalog publication

RuntimeContext now retains the actual ScenarioCatalogOwnership after its
ArchiveMountService, so dependent scene services retire before the catalog and
archive service. Both showcase entrypoints and the default application factory
explicitly initialize it after platform/resource startup and before Game scene
construction. The original GameSystem similarly preloads scenario archives and
constructs its parser after stationed resources finish loading.

Platform-only RuntimeContext construction still needs no StageData catalog.
The MR accessor performs no lazy I/O and rejects use before explicit successful
publication. Initialization uses the configurable scenario_catalog_bytes
budget (default8MiB) from GameResourceRuntime; authored rows and capacities are
validated by the existing actual catalog owner. Camera/scene owners can retain
the same shared catalog within the enclosing RuntimeContext lifetime.

Validation on the M5 Max:

* smg-pc-runtime-context-construction-tests builds and runs with the actual disc
  and Metal. Real mapped allocation failure and injected startup failure clean
  up; two full contexts explicitly initialize the actual48-archive parser and
  retire all registry, JKR, archive, catalog, texture and video ownership.
* smg-pc-app builds, including the changed default bootstrap factory.
* Complete Showcase.cpp compiles with its regular native compiler arguments.

The lifecycle test target discards unused functions, as the other bounded
original-owner fixtures do, to avoid linking unused incomplete Mario routines.
This does not replace those routines. The full showcase remains blocked by the
separately documented original MarioEffect API/EffectSystem owner work.

The Metal fixture emits its existing shutdown device-loss and pending-buffer
mapping messages when its short-lived device is destroyed. It exits0 and all
explicit lifecycle checks pass; no gameplay or screenshot result is claimed.
