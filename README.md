# Petari PC port

A work-in-progress native PC port of Super Mario Galaxy using Xmake and Aurora.
The PC port occupies the repository root. Supply your own game disc image;
game assets are not included.

## Setup and build

```sh
git clone --branch pcp-aurora --recurse-submodules https://github.com/Frityet/Petari.git
cd Petari
```

For an existing checkout, run `git submodule update --init --recursive`.
On Apple Silicon macOS, install the dependencies listed in [MACOS.md](MACOS.md),
then use the root launcher:

```sh
./script/build_and_run.sh --build-only
./script/build_and_run.sh --disc /path/to/game.rvz
./script/build_and_run.sh gateway --disc /path/to/game.rvz
```

The launcher also discovers a single `.rvz` in the repository root. The VS Code
build task and Codex Run action use this launcher on macOS.

Linux uses the root Xmake project and Dockerfile. CI builds the toolchain image,
then configures and builds `smg-pc` and `smg-pc-showcase` from the repository root.
For an interactive development container:

```sh
docker build -t petari-build .
docker run --rm -it -v "$PWD:/workspaces/pcport" petari-build
# Inside the container:
xmake f -y -m debug --toolchain=clang --runtimes=c++_shared
xmake build smg-pc-showcase
```

See [container/entrypoint.sh](container/entrypoint.sh) for X11/Podman invocation
examples and [showcase-package/README.md](showcase-package/README.md) for Linux
packaging. Build artifacts live in `build/` and tool caches in `.cache/`.

## Layout

- `src/`: game sources, host services, compatibility code, and debug tools.
- `aurora/`: Aurora submodule providing Wii APIs and graphics support.
- `dolphin/`: Dolphin submodule for reference and parity checks.
- `decomp/`: separate Petari decompilation submodule.
- `tests/`: native regression tests and source-comparison checks.
- `scripts/`, `script/`, `packages/`: Xmake tasks, launcher, and package definitions.
- `notes/`: local development notes and evidence, normally ignored by Git.

The PC build uses its own source tree. It does not compile against `decomp/`.

## Decompilation work

The `decomp/` submodule comes from
[Frityet/Petari, branch pcp-decomp](https://github.com/Frityet/Petari/tree/pcp-decomp).
The parent repository pins a particular revision. On a fresh checkout, switch
the submodule to its development branch before editing:

```sh
git -C decomp switch pcp-decomp
```

Follow `decomp/README.md` for decompilation builds and run its configuration
commands from `decomp/`. Make decompilation changes there, then copy relevant
sources and headers into the port's `src/` tree, following [AGENTS.md](AGENTS.md).
Commit changes inside the submodule before recording its revision in the parent
repository. Publish the submodule commit before publishing the parent revision
that refers to it.

To deliberately update the reference to the branch tip, first finish or save
local decompilation work, then run `git submodule update --remote decomp`.
The reference is independent of the port and may lack newer port counterparts.

```sh
xmake source-closeness-audit --output=notes/source-closeness
```

## Validation and current limits

The game library builds from the root. The flattened layout passes the
debug-path, text-encoding, fixed-step-clock,
and scene-scheduler-heap tests on macOS arm64 with LLVM 23. All six debug probes
using shared path discovery compile, and the source audit reads the reference
from `decomp/`. The relocated submodules initialize recursively.

Clean application builds expose existing unfinished game code: the showcase
now reaches incomplete effect APIs used by the original `MarioEffect.cpp`, and the main PC target
fails to link missing game methods. The animation pointer fields now retain full native pointer width. The latest
upstream and compatibility dependencies are integrated; eight focused tests,
including exact Metal GX copy pixels, pass. Source-mirror checks report
actual source differences and absent counterparts in the selected decompilation
branch. These failures are retained rather than bypassed. Full gameplay remains
incomplete. The Linux CI workflow has been linted, but has not been run here.

## Original string encoding

Keep source files in UTF-8. Original narrow strings containing Japanese text
must use `CP932("日本語")` from `compat/Cp932Literal.hpp`. Conversion happens
entirely at compile time, with no compiler wrapper or runtime allocation. ASCII
strings and original wide strings keep their normal spelling. Put adjacent
literals inside one wrapper: `CP932("日本" "語")`.

The result is a static `const char` array with the exact encoded size, including
its terminating NUL. It decays to a pointer normally and supports `sizeof`.
For array references, use `constexpr const auto& name = CP932("日本語")`;
C++ does not permit copying it through `char name[] = CP932(...)`. Malformed
UTF-8 and characters outside the frozen CP932 mapping are compilation errors.
Raw CP932 byte escapes already contain encoded bytes and must stay unwrapped.

Source-mirror checks recognize only this explicit encoding adaptation; other
Game differences remain visible. After importing new decompiled Game code,
annotate its non-ASCII narrow literals before compiling the port.

Run `python3 tests/test_cp932_literals.py --cxx clang++` to check the complete
mapping, array semantics, cross-translation-unit storage and rejected inputs.

## Credits

Based on [SMGCommunity/Petari](https://github.com/SMGCommunity/Petari), Aurora,
and the original project's credited contributors, including doldecomp for bte,
zeldaret/tp for JSystem, and doldecomp/ogws for source and headers.
See [LICENSE](LICENSE).
