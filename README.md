# Petari PC port

A work-in-progress native PC port of Super Mario Galaxy using Xmake and Aurora.
The PC port now occupies the repository root. Supply your own game disc image.

```sh
git submodule update --init --recursive
./script/build_and_run.sh --build-only
./script/build_and_run.sh --disc /path/to/game.rvz
```

See [MACOS.md](MACOS.md) for Apple Silicon setup and current gameplay limits.
Sources are in `src/`, tests in `tests/`, and build output in `build/`.
Aurora and Dolphin are submodules at `aurora/` and `dolphin/`.

The decompilation reference is the `decomp/` submodule from
[Frityet/Petari, branch pcp-decomp](https://github.com/Frityet/Petari/tree/pcp-decomp).
Follow its README for decompilation builds. Make decompilation changes there,
then copy relevant sources and headers into the port's `src/` tree.
Commit changes in the submodule before recording its revision in this repository.
On a fresh checkout, use `git -C decomp switch pcp-decomp` before editing it.

This repository layout is an intermediate cleanup checkpoint. Native builds,
CI, and editor integration have not yet been validated or fully updated for the
new layout. Source-mirror checks already failed before this cleanup and the
selected decompilation branch also differs from the previously embedded tree.

Based on SMGCommunity/Petari, Aurora, and the original project's credited
contributors. See [LICENSE](LICENSE).
