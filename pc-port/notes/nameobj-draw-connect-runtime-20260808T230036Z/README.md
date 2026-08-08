# NameObj temporary draw connection

Date: 2026-08-08 UTC

## Outcome

Restoring the exact `Game/Util/LiveActorUtil.hpp` removed a host-only
`LiveActor*` overload for `MR::connectToDrawTemporarily` and exposed the real
retail API in `NameObjExecuteHolder.hpp`: temporary draw membership belongs to
the registered `NameObj`, not to actor clipping.

The compatibility layer now models that distinction directly:

- `SceneScheduler` retains a `draw_connected` bit for every registration;
- temporary disconnect suppresses both draw-buffer and draw-type dispatch but
  leaves movement, calc, registration order, dead state, and clipping intact;
- reconnect restores the existing registration without registering a second
  object;
- the retail `NameObj*` functions are provided by
  `compat/NameObjExecuteCompat.cpp`;
- an unregistered object or missing active scene is rejected explicitly;
- a small `SceneSchedulerBinding` lets isolated tools/tests use the same real
  scheduler service without constructing a fake `RuntimeContext`.

The old compatibility implementation toggled `LiveActor::mFlag.mIsClipped`.
That shortcut was removed. It conflated frustum clipping with draw-list
membership and could be overwritten by the next clipping update.

The scheduler debug snapshot exposes `draw_connected` only in non-release
builds. No debug behavior or Game-source instrumentation is present in release
code.

## Verification

`smg-pc-aurora-native-tests` passes 25/25. Its PartsModel case now proves the
exact hide/show movement path disconnects and reconnects draw membership while
leaving clipping unchanged. The registration-scope case also proves the
connect/disconnect bit is reversible without removing the entry.

`smg-pc-live-actor-util-real-or-absent-tests` passes 5/5 after consumers include
the retail owner header (`JointUtil.hpp`) instead of relying on declarations
formerly embedded in the host-only LiveActor header.
