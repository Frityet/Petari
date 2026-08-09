# Mario.cpp final functional-completeness tranche (RMGK02)

## Scope and evidence

- Retail unit: `main/Game/Player/Mario`
- Source: `src/Game/Player/Mario.cpp`
- Evidence: `build/RMGK02/asm/Game/Player/Mario.s` and the regenerated `build/RMGK02/report.json`
- The unit audit found seven inactive target-owned functions. All seven now emit code:
  - `Mario::fixHeadFrontVecByGravity()` — 87.94749% fuzzy
  - `Mario::getAirFrontVec() const` — 100%
  - `Mario::updateLookOfs()` — 86.14035% fuzzy
  - `__sinit_\\Mario_cpp` — 100%
  - `XanimeCore::getJointTransform(u32)` — 100%
  - `MarioState::draw3D() const` — 100%
  - `TriangleFilterDelegator<Mario>::isInvalidTriangle(const Triangle*) const` — 100%
- Functional corrections retained from the retail control flow include gravity-relative head/front reconstruction, portable cross-product destination ordering, airborne forward projection fallback, wall/swim/fall look offsets, and the frame-ramped look blend capped at `0.2f`.
- No shared header or PC production/compat/factory source was changed.

## Result

- Unit: 55/55 target functions active; zero functions at 0%.
- Unit fuzzy similarity: 96.54665%.
- Exact functions: 39/55 (70.90909%).
- Exact matched code: 31.612349%.
- Focused compile: `ninja -C . build/RMGK02/src/Game/Player/Mario.o` passed.
- Fresh report: `ninja -C . build/RMGK02/report.json` passed.
- Full retail link: `ninja -C . build/RMGK02/main.dol` passed.
- Retail DOL SHA-1: `54b71431af0d509097bfdef4ec28617afc487e89`, matching `config/RMGK02/config.yml`.
- Retail DOL SHA-256: `8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`.

## Provider implications

- The recovered update/orientation path now has concrete implementations for both gravity-relative facing and camera/model look offsets.
- The three small wrappers provide the retail-owned joint-transform lookup, default state draw hook, and Mario triangle-filter dispatch.
- The Mario actor nerve instances and their static initializer are emitted by the target-owned unit, completing its retail symbol surface.
