# ActorSensor real-or-absent boundary

Captured at 2026-08-07 04:27:16 UTC.

## Outcome

- Restored `pc-port/src/Game/Util/ActorSensorUtil.cpp` and `.hpp` byte-for-byte from the regular decomp.
- Kept the exact retail `.cpp` out of the PC target because it requires the unported retail `HitSensorKeeper`, binder, group, and scene-object implementations. Supported host behavior is implemented in `src/compat/GameActorSensorCompat.cpp`.
- Added the regular decomp's `MessageSensorHolder.cpp` and `.hpp` byte-for-byte. The host scene now creates that real scene object.
- Removed the process-static fake `MessageSensorHost` and fake message sensor.
- Removed the host-only `addHitSensorPlayer` alias. The host player calls the exact declared generic API with `ATYPE_PLAYER`.
- Removed the name-based `"Catch"` type guess. Enemy registration now uses the requested retail type.
- Matrix and position sensors retain their real external binding. Joint sensors call `MR::getJointMtx` for the named J3D joint and are rejected when the actor, model, runtime, or joint is absent.
- No actor/base/origin sensor is created after a missing matrix or joint lookup.
- Callback sensors and unsupported binder-contact queries were not implemented as placeholders; their retail declarations remain link-absent until the real backing subsystem exists.
- Unknown debug message values now produce a null name in parity telemetry rather than the fabricated string `"Unknown"`.

## Runtime ownership

Non-actor-relative sensor binding metadata is held in the compatibility layer. `LiveActor::updateHitSensors` performs its ordinary actor-relative update and invokes one general compatibility hook to reapply real position, matrix, or named-joint bindings. Reinitialization and destruction release that compatibility state.

The message sensor is available only when an active `SceneObjHolder` explicitly owns a `MessageSensorHolder`. It disappears with that scene.

## Scope note

The worktree was already heavily modified by parallel real-or-absent sweeps. No unrelated changes were reverted. In particular, shared `tests/xmake.lua`, `Game/xmake.lua`, layout work, and other compatibility additions were preserved.
