# Strict HeavensDoor construction probe

Date: 2026-08-08T01:37:40Z

Base commit: `61f77cda673813535c9d297559e1298fed785375`

Disc used locally: a user-provided RMGK01 image (not included)

## Policy change

The `fail_unsupported_placement` request flag was removed. Stage construction now has one behavior: every actor-bearing placement must have its real retail creator, or the entire scene remains unavailable. There is no public or internal request mode that exposes a partially constructed stage.

The new debug probe is generic and accepts a disc, stage, scenario, and start identifiers. It calls the same scene controller, scene lifecycle, placement resolver, demo setup, and gravity boundary as a normal request. It does not weaken any checks.

## Commands

```text
xmake -vD smg-pc-stage-construction-probe
xvfb-run -a build/linux/x86_64/debug/smg-pc-stage-construction-probe \
  --disc /workspaces/pcport/RMGK01.iso \
  --stage HeavensDoorGalaxy \
  --scenario 1
```

Build exit: `0`

Probe exit: `1` (expected unavailable boundary)

## Result

The real disc loaded 1994 Korean messages, 3327 particle names/resources, and 225 particle textures. Scene construction registered the required retail scene holders and the real `DemoDirector` runtime, then stopped at the first currently unavailable gravity actor:

```text
Strict stage construction failed: unsupported PlanetGravity placement requires its real implementation: GlobalPlaneGravity (switch/follower lifecycle)
```

The process returned normally with exit code 1. The previous secondary `Effect deletion requires a registered effect keeper` exception and SIGABRT/exit 134 did not occur. This proves scene-scope unwind preserves the original strict failure.

No `pc-port/src/Game` source was edited for the strict request or probe.
