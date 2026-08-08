# Scene effect scope ownership release

Date: 2026-08-08T01:32:15Z

PC port base commit: `8491611bb04b7162ef7f2370b19c81645237c6d7`

Aurora commit: `9465d71d2a329dfa097159e015921589db5a7848`

## Failure reproduced

A direct HeavensDoor stage construction reached the honest gravity boundary:

```text
unsupported PlanetGravity placement requires its real implementation: GlobalPlaneGravity (switch/follower lifecycle)
```

While `StageHostScene` unwound, its scene registration cleanup incorrectly treated every scheduler registration as an effect owner. The first scheduler-only object was `DemoDirector`, which had never registered an `EffectKeeper`. Cleanup called strict retail effect deletion anyway and raised a second exception:

```text
Effect deletion requires a registered effect keeper.
```

Because this happened in `StageHostScene::~StageHostScene`, the secondary exception caused `std::terminate` and hid the real gravity boundary.

## General compatibility fix

- Explicit Game/API `delete_effect` and `delete_all` remain strict and still require a real keeper.
- Internal scene ownership release is idempotent and non-throwing.
- Ownership release clears only the matching active effects, keeper registration, and transform binding.
- It emits no synthetic retail `DeleteAll` event.
- Scheduler names are no longer inferred to own effects.
- Concrete scheduler identities are released so layout/live-actor transform bindings do not leak.
- Name-keyed scene tracker entries are removed when their keeper is unregistered.

No `pc-port/src/Game` source was changed for this fix.

## Verification

Focused test executable:

```text
[ok] exact rumble pattern drives real actuator
[ok] camera shake changes projection exactly
[ok] effect deletion requires keeper
[ok] effect lifecycle release is identity-safe
[ok] Game feedback boundary reports absence
5 feedback real-or-absent test(s) passed
```

Full test group:

```text
100% tests passed, 0 test(s) failed out of 32, spent 1.551s
```

The identity regression covers an absent/idempotent release, two distinct identity-owned hosts with the same name, a persistent name-owned host with that same name, binding and active-effect removal, and the absence of a manufactured `DeleteAll` event.
