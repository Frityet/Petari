# Process particle resource integration

Applied the four-file ownership/query proposal and copied the complete original EffectSystemUtil header needed by its literal query methods. The ordinary native build has no root-header fallback. Existing globs select the actual providers.

RuntimeContext now retains the real ParticleResourceOwnership beside its scenario/archive owners. Explicit resource-ready startup in Application and both Showcase paths constructs the original holder after the scenario catalog. A 2 MiB particle resource budget is appended to GameResourceBudget, preserving existing aggregate arguments. Consumers can retain the actual owner; scene consumers must retire before the RuntimeContext archive service. Platform-only RuntimeContext construction does not eagerly access Game particle files.

LLVM 23 ARM64 debug builds pass for the regular owner test, runtime construction test and application static library. The regular owner fixture passes on the real disc: all 3327 resource queries, 225 textures, 2591 auto-effect rows, 612 case-insensitive holder groups, minimum/default budgets, two reconstruction cycles, prior-mount identity, early/late allocation failure and backing release. The isolated ASan/UBSan coverage is retained from the staging package.

The real-disc RuntimeContext fixture runs on the actual Aurora Metal renderer and verifies both explicit resource-ready initializations return the original particle holder, then reclaim every owner across two full reconstructions. Existing allocation/logger failure tests and original SC/CameraContext startup also pass. Pre-existing pending buffer-map/device-destruction warnings appear during renderer teardown; the process exits zero.

Actual scene EffectSystem and pointer-returning EffectUtil replacement remain pending. This resource-owner integration does not claim visible particles, Mario jumping, or complete gameplay. The legacy effect service remains selected until its actual scene owner and facade replacement are ready together.

Focused evidence: production-test.log, runtime-test.log, probe-runtime.log. Build warning logs are local execution artifacts.
