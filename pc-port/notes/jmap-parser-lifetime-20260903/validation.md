# Validation

Commands run from `pc-port` on macOS ARM64, existing LLVM 23 debug configuration:

```sh
xmake build -j8 smg-pc-original-jmap-heap-lifetime-tests
build/macosx/arm64/debug/smg-pc-original-jmap-heap-lifetime-tests
xmake build -j8 smg-pc-original-jmap-resource-tests
build/macosx/arm64/debug/smg-pc-original-jmap-resource-tests
xmake build -j8 smg-pc-original-resource-holder-tests
SMGPC_REAL_DISC='../Super Mario Wii - Galaxy Adventure (Korea).rvz' \
  build/macosx/arm64/debug/smg-pc-original-resource-holder-tests
xmake build -j8 smg-pc-showcase
build/macosx/arm64/debug/smg-pc-showcase title \
  --disc '../Super Mario Wii - Galaxy Adventure (Korea).rvz' --smoke --max-frames 600
build/macosx/arm64/debug/smg-pc-showcase gateway \
  --disc '../Super Mario Wii - Galaxy Adventure (Korea).rvz' --smoke --max-frames 600
```

- Lifetime target: build passes; all **5 groups pass** (`lifetime.log`).
- Original JMap resource target: build passes; all **10 groups pass**
  (`jmap-resource.log`). These include parent-owned deferred archive validation,
  retention, aliasing and concurrent-reader changes.
- ResourceHolder target: initial link failed because the new original CSV actor
  overload references `MR::getResourceHolder(const LiveActor*)`, which has no
  current provider. The failure is recorded in `build-resource-holder.log` and
  reported to the parent owning CSV/actor integration. Parent then supplied the
  two unchanged original actor-resource accessors and the original ModelManager
  header. Final build passes (`build-resource-holder-final.log`), and all
  **5 groups pass with the actual RVZ** (`resource-holder.log`), including
  original CSV vector/default checks and real model/material holder resources.
- Showcase rebuild passes. **Title smoke passes in 2 rendered frames** and
  **Gateway smoke passes in 5 rendered frames** (`title-smoke.log`,
  `gateway-smoke.log`). The former verifies actual sky BCK/BTK packets; the
  latter verifies animated Mario/planet packets, GPU submission, gravity and
  exact KCL probe contact. These are bounded showcase gates, not full gameplay
  or jump completion claims. Both use the updated native JMap layout and
  Aurora viewport replay checkpoint `41383f0`.

The lifetime production source and fixture had no changes between their
successful build and execution. `verify-source.py` passes and records source
hashes in `source-evidence.json`. `git diff --check` passes for the owned source
and test wiring.
