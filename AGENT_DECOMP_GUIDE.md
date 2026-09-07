# AGENT Decomp Guide

This guide captures the practical patterns to follow when editing decomp code in this repo.

## Completion

- We are only caring about functional matches (where the functionality is close to 1-1) and/or a high fuzzy match, we do not care about exact matches. Aim for a 90-95%+ match

## Baseline

- Use `#pragma once` in headers.
- Follow root `.clang-format`.
- Use `nullptr` for pointers and keep pointer declarations as `Type* p` / `Type const* p`.
- Keep include order consistent: project headers first, then system/SDK `<...>` headers.
- Use constructor initializer lists instead of implicit member assignment.

## Object lifecycle and state

- Use `initNerve` / `INIT_NERVE` for new state setup.
- Define state names with `NERVE_DECL`/`NERVE_DECL_*` where that pattern exists in scope.
- Keep transition logic in focused `exe*` handlers (`setNerve(&...::sInstance)` where transitions occur).
- Keep `control`/`control`-entry work short and declarative: check guards, then delegate.

## Nullability and message safety

- Use explicit checks: `ptr == nullptr` / `ptr != nullptr`.
- Guard pointers before dereference and return early on invalid state.
- Use early exits (`return`, `if (...) { return; }`) in guard-heavy logic paths.

## Casting and access patterns

- Use `static_cast` for well-defined conversions.
- Prefer typed members and direct field access over temporary cast-heavy arithmetic.
- Use `reinterpret_cast` only when layout/ABI constraints require it.
- Remove redundant cast noise by relying on existing abstractions and utility APIs.

## Math, vectors, and constants

- Use existing vector helpers (`length()`, `dot`, `cross`, `normalize`, `squared`) instead of manual scalar re-derivations.
- Use constants from engine helpers (`PI`, `HALF_PI`, `PI_180`, `MR::clamp`, etc.) instead of inline literals.
- Keep gameplay tuning values as named locals/constants with explicit semantic meaning.

## API and utility preference

- Prefer existing `MR::` helpers and engine APIs before writing local one-off equivalents.
- Prefer existing constructors/method calls (`MR::setDrawMtx`, `LayoutUtil`, `StarPointerUtil`, etc.) over manual equivalent code.
- Keep symbol names as-is when touching existing decomp symbols unless renaming is part of an intentional migration.

## `MR::isNearZero` usage

- `MR::isNearZero` has a default epsilon; omit the default argument unless behavior is intentionally different.

## Good examples in `MapObj` and `Screen`

Use these files as direct style references:

- `src/Game/MapObj/AstroCore.cpp`
- `src/Game/MapObj/AnmModelObj.cpp`
- `src/Game/MapObj/BenefitItemInvincible.cpp`
- `src/Game/Screen/BackButton.cpp`
- `src/Game/Screen/CounterLayoutAppearer.cpp`
- `src/Game/Screen/CounterLayoutController.cpp`
- `src/Game/Screen/GalaxyMapIcon.cpp`

Consistent patterns shown there:

- Constant pools in anonymous namespaces for local config.
- Clear constructor initialization with deterministic members.
- Explicit null checks and early exits in control paths.
- Helper calls over inline arithmetic duplication.
- Straightforward state transitions and `exe*`-scoped behavior.
- Small, focused layout/UI methods with delegated sub-objects.

## FORBIDDEN:

- **NO INLINE ASM**
- Avoid (unles sabsolutley 100% neccicary) files that are outside our core decomp objective, especially do not modify stuff like the matrix and vec, things like that
