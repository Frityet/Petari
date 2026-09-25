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

On Apple Silicon macOS, install Xcode (including its command-line tools) and:

```sh
brew install xmake llvm cmake ninja
```

The build requires LLVM 23 with development headers and `libclang-cpp`. Apple's Clang alone is insufficient.
Xmake finds Homebrew LLVM automatically; third-party libraries are managed by
Xmake packages. The first build may download and compile dependencies.

```sh
xmake                              # build the original game and its dependencies
xmake f --disc=/path/to/game.rvz    # remember your disc (optional)
xmake run                          # build if needed, then launch the original game process
xmake run smg-pc --stage HeavensDoorGalaxy --scenario 1   # launch Gateway
```

Disc selection is: explicit `--disc PATH` argument, configured `--disc`,
`SMGPC_DISC_IMAGE`, then a single `.rvz`, `.iso`, or `.wbfs` in the repository root.
Runtime paths are relative to the repository root. No disc is needed to build.

```sh
xmake run smg-pc --disc /path/to/game.rvz --max-frames 60
xmake run -d smg-pc --stage HeavensDoorGalaxy --scenario 1         # LLDB/GDB
xmake run smg-pc --max-frames 60      # bounded runtime check
xmake f --optimize_debug=y                  # retain debug checks with optimization
xmake f -m release                         # select release mode
xmake f -m debug --optimize_debug=n         # restore normal debug mode
xmake build smg-pc                         # rebuild the original game application
```

The original game application is the only default target. Developer tools and tests remain
available by name. `xmake run` always checks whether a rebuild is needed. VS Code
and the Codex Run action use these same commands.

On macOS, Xmake's `xcode.application` rule creates the app beside the executable,
for example `build/macosx/arm64/debug/smg-pc.app`. `xmake clean` removes
the active configuration's products. Use `xmake f -o /path/to/build` to choose an
output directory, or `xmake f --toolchain=llvm --sdk=/path/to/llvm` for a custom LLVM.
`compile_commands.json` is refreshed for clangd after a build and exposes the real
compiler. Original Game strings use explicit compile-time CP932 conversion.

Linux development uses the same commands inside the toolchain container:

```sh
docker build -t petari-build .
docker run --rm -it -v "$PWD:/workspaces/pcport" petari-build
# Inside the container:
xmake
xmake run smg-pc --disc /path/to/game.rvz
```

See [container/entrypoint.sh](container/entrypoint.sh) for X11/Podman invocation
examples. The retired title-showcase package has been removed. Linux runtime
validation has not been performed for this cleanup.

## Developer tools and local packaging

```sh
python3 tests/test_cp932_literals.py
python3 tests/test_source_provider_audit.py
xmake test -g tools
xmake validate-trace-sqlite /path/to/trace.sqlite
xmake source-closeness-audit --output=notes/source-closeness --check-providers
```

Trace tools use the configured target paths; they require debug mode. Render
parity and route capture retain their platform-specific display/Dolphin needs.
`xmake --help` lists the available project tasks.

The two local macOS demo packagers are consolidated into one Lua task. It copies
a selected executable unchanged, records provenance and uses an external disc;
it does not build or establish gameplay correctness. Obtain the expected SHA256
from the executable you selected for validation. Use `--source-app=PATH` to copy
an existing app with its bundled dependencies, or `--binary=PATH` for a standalone
executable with only system library dependencies.

```sh
xmake package-demo --source-app=build/macosx/arm64/debug/smg-pc.app \
    --disc=/path/to/game.rvz --expected-sha256=HASH --dry-run
# Remove --dry-run to create the package; --output must name a new .app path.
```

## Layout

- `src/`: game sources, host services, compatibility code, and debug tools.
- `aurora/`: Aurora submodule providing Wii APIs and graphics support.
- `dolphin/`: Dolphin submodule for reference and parity checks.
- `decomp/`: separate Petari decompilation submodule.
- `tests/`: native regression tests and source-comparison checks.
- `scripts/`: Xmake Lua rules, tasks, compiler tooling and packaging templates.
- `packages/`: local Xmake package definitions.
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
xmake source-closeness-audit --output=notes/source-closeness --check-providers
```

## Validation and current limits

The application runs the original GameSystem and scene sequence. Original game
code owns actors, story progression, and JPA particle simulation. Compatibility
code provides host services and Wii APIs; there is no separate showcase game.

Full gameplay remains incomplete. The default FileSelect scene still lacks
content/rendering; use the explicit Gateway stage command above for the opening
demo. A fresh 1,200-frame Gateway run completes, but this cleanup does not
validate the entire route through Rosalina. Stage loading reports actual original
placement queues, distinguishing supported creators, known but unlinked original
creators, and unknown names. Set `SMGPC_ORIGINAL_PLACEMENT_REPORT_PATH` to save the
report, or `SMGPC_STRICT_PLACEMENT=1` to reject known unlinked content before
construction. A successful bounded run does not establish complete content or
rendering parity. Source and build ownership evidence is recorded in `notes/`.

## Original string encoding

Keep source files in UTF-8. Original narrow strings containing Japanese text
must use `CP932("日本語")` from `resource/TextEncoding.hpp`. Conversion happens
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

Run `python3 tests/test_cp932_literals.py
python3 tests/test_source_provider_audit.py --cxx clang++` to check the complete
mapping, array semantics, cross-translation-unit storage and rejected inputs.

## Credits

Based on [SMGCommunity/Petari](https://github.com/SMGCommunity/Petari), Aurora,
and the original project's credited contributors, including doldecomp for bte,
zeldaret/tp for JSystem, and doldecomp/ogws for source and headers.
See [LICENSE](LICENSE).
