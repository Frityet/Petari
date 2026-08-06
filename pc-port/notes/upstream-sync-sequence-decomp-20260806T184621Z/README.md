# Upstream Sync And Sequence Decomp Work Note

Updated: 2026-08-06T18:46:21Z

## Objective

Integrate the current `smgcommunity/petari` decomp baseline, restore a reproducible PC-port build, and replace the inherited `SequenceUtil` approximation with an assembly-backed functional decomp before continuing the Gateway/HeavensDoor route.

## Upstream Merge

- Fetched `upstream/master` at `4afa51b76` (`scsystem` at 99%).
- Pre-merge divergence was 52 local commits and 902 upstream commits from merge base `bb25f391d`.
- Created merge commit `a56d6f448` with parents `c4a39a591` and `4afa51b76`.
- The 87 textual conflicts were all in the sibling root decomp/JSystem tree; none touched `pc-port` or the tracked PC-port WIP.
- Conflict resolution selected the much newer upstream decomp version for all 87 conflicted files. A post-merge blob audit confirmed each result equals `upstream/master` and no conflict markers remain.
- Current branch relation after the merge: 53 commits ahead and 0 behind `upstream/master`.

## Build Dependency Integration

The first fresh build stopped before compilation because two project layers requested incompatible providers for the same dependency:

- Aurora requests packaged `sqlite3 3.53.0+0` and `fmt 11.1.4`.
- `src/common/xmake.lua` requested system SQLite and an unpinned `fmt`.

Xmake unified the SQLite entries and consequently searched the host for the exact Aurora version, which cannot work. `src/common/xmake.lua` now uses the same packaged versions as Aurora. The declared packages were installed with Xmake; no game-specific workaround was added.

## Verification So Far

Fresh debug build:

```text
xmake f -c -y --mode=debug
xmake build smg-pc
[100%]: build ok, spent 30.228s
```

Native tests:

```text
xmake test
8 Aurora-native test(s) passed
100% tests passed, 0 test(s) failed
```

Forced-X11 smoke after installing the container's missing `xvfb` package:

```text
Opened Aurora disc image /workspaces/pcport/RMGK01.wbfs
sequence_state:stage_requested (... requested_stage=FileSelect;scenario=1)
placement:stage_placement_summary (stage=FileSelect;scenario=1;objects=4;created=2;ignored=1;blocked=1)
process exit: 0
```

The smoke confirms the fresh binary initializes Aurora/Vulkan, mounts the disc, loads messages/effects, constructs FileSelect, and exits normally. It does not prove the Gateway route.

## SequenceUtil Review And Decomp

The inherited uncommitted `SequenceUtil.cpp` is not a functional decomp. It collapses distinct `GalaxyMoveArgument` move types into direct stage requests, discards `JMapIdInfo`, contains several empty game-scene actions, and hardcodes retry state. One concrete bad path is `requestChangeSceneTitle()`: the approximation requests a fake `Title` stage, while the original constructs galaxy-move type 7, which routes to FileSelect through the sequence system.

Authoritative assembly is available at:

```text
build/RMGK02/asm/Game/Util/SequenceUtil.s
```

The assembly-backed reconstruction is in progress under the required workflow: write the new decomp to root `src/Game/Util/SequenceUtil.cpp` first, then copy the source-shaped implementation into `pc-port` and add general sequence compatibility beneath it rather than retaining no-op or route-specific behavior.

## Container Changes

- Installed `xvfb` and `xauth` because `pc-port/AGENTS.md` said Xvfb was present but this container lacked `xvfb-run`.
- Installed `xdotool` to exercise the existing debug F10 Gateway request without adding a direct-boot game-code hook.

## Next Evidence

- Complete and review the `SequenceUtil` decomp and its PC compatibility boundary.
- Capture a fresh deterministic HeavensDoor placement report from the current binary.
- Use that report plus the newly merged upstream actor sources to select the next source-close Gateway actor/runtime slice.
