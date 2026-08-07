# Real-or-absent compatibility audit

Date: 2026-08-07 UTC

## Rule

Unsupported game content must remain absent. Compatibility code must not turn
an unrelated archive entry, model, actor, animation, collision mesh, camera,
particle, Mii, message tag, or player state into a substitute for requested
retail data.

Retail objects whose actual names contain `Dummy`, mathematical defaults for a
degenerate vector, BCSV field defaults, and debug assertions that forbid old
fake placement statuses are not substitutions and must not be removed by a
lexical sweep.

## First completed cleanup

`LiveActorModel` now resolves only the requested model and animation basenames:

- a model archive must contain `<ModelArcName>.bdl` or
  `<ModelArcName>.bmd`;
- BCK, BTK, and BRK requests select only their requested resource name;
- a missing named BTK clears a previously bound BTK instead of leaving the
  unrelated animation active;
- the former first-BDL/BMD/BCK/BTK/BRK selection paths are gone.

This is generalized render compatibility work and does not edit `src/Game`.

The rebuilt executable passed the complete required handoff: title, all six
file-select slots, the five-page picturebook sequence, and Gateway scenario 1.
The exact-name rule did not regress any of those retail resources. Screenshots
and per-checkpoint manifests are retained under `route-smoke/`; the Gateway
image is intentionally sparse because unsupported placements are absent rather
than replaced with generic models.

## Ranked remaining structural work

The audit identified these real-or-absent violations for follow-up:

1. replace the synthetic `StagePlayerRuntime` and fabricated follow camera with
   the real Mario/player and camera stack, or omit them;
2. register collision only through real `CollisionParts` lifecycle requests,
   never by selecting a sole or arbitrary KCL;
3. omit unsupported JPA shapes rather than drawing them as billboards;
4. expose an empty RFL database when real RFL data is absent rather than
   manufacturing Miis;
5. leave absent messages/placement display names absent rather than displaying
   their tag or English object identifier;
6. reject unresolved child zones rather than assigning synthetic zone IDs;
7. remove no-director demo/talk success paths as the real director machinery is
   imported;
8. move host layout and live-actor state out of `pc-port/src/Game`, then restore
   those Game files to the regular decompiled source.

The opening route must remain title, six-slot file select, five-page
picturebook, then Gateway while these are replaced. No route, stage, actor, or
demo-name branches are acceptable as replacements.

See `verification.log` for build and route evidence.
