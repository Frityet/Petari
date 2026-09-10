# Original Game execution character set

## Confirmed failure and chosen boundary

Native Game source is UTF-8 on disk, while the original `sjiswrap` compiler emits ordinary narrow literals in CP932. `JMapInfo::getStringValueByHash` preserves BCSV string bytes, and `findElement<const char*>` compares them with `strcmp`. Consequently a native Japanese source literal cannot find the exact embedded StoryEventBCSV row. The original `isPassedStoryEvent` / `followStoryEventByName` methods then consume a missing result rather than the intended progress row.

The flag-table problem is independent of JMap: original `GameEventFlagTableInstance` sorts truncated hashes computed from its literal names and checks only the first equal key. UTF-8 creates a collision between `テレサマリオ初変身` and `CocoonExGalaxy` at `0x389a`; CP932 creates no collision among the original 188 names. A save-only hash scope cannot repair this cached gameplay table. The existing inventory is in `../chunk-encoding/flag-lookup-collisions.json`.

The consistent contract is **original Game identities remain CP932**, including compiled ordinary literals and retained resource bytes; native presentation remains UTF-8. Decode/encode at explicitly directed native boundaries. Do not guess the encoding from a `char*`, change Game algorithms, normalize raw resource identities through a Unicode round trip, or create a second flag/story table. CP932 has duplicate byte encodings, so decode then re-encode is not an identity-preserving resource operation.

Fresh LLVM 23.1 probes reject `-fexec-charset=CP932`, `-fexec-charset=SHIFT-JIS`, and `-finput-charset=CP932`; `-fexec-charset=UTF-8` succeeds. See `clang-charset-probes.json`.

## Implementation outside Game

- `script/game_literal_preprocessor.cpp` uses Clang's actual preprocessor and its documented final-token watcher. Clang expands macros and emits its normal preprocessed output, including pragmas and original line markers. The helper records each final literal's source provenance and checks exact token spelling/kind against a second Clang lexical pass over that output. An unexpected divergence fails compilation.
- `script/game_execution_charset.py` converts only ordinary narrow execution literals admitted by a Game source root or explicit original-provider file. It uses strict CP932 and fixed three-digit octal escapes. It preserves numeric/simple escapes, raw contents, UCN values, comments/identifier processing, and line counts. Explicit `u8`, `u`, `U`, and `L` literals remain unchanged. Adjacent ordinary strings inheriting a Unicode prefix are recognized **after macro expansion** and remain Unicode.
- The wrapper preprocesses and compiles one actual C++ translation unit, then removes temporary `.ii` and literal metadata files. It retains original compiler target/ABI/optimization/debug flags and original dependency paths. Link/compiler-probe invocations delegate directly to Clang.
- `script/game_execution_charset.lua` installs the wrapper only on `smg-pc-*` C++ targets, not Aurora dependencies. It builds the helper against the selected LLVM, hashes the Python/C++/Lua contents and LLVM configuration using a bytes manifest, disables Xmake's preprocessing cache, and invalidates objects/archive/link when the successful tool fingerprint changes.

The implementation reuses the proven literal-encoding logic from `notes/original-game-execution-charset-20260903/staged/tools/game_execution_charset.py`. It replaces that proposal's pre-preprocessor VFS conversion with post-expansion conversion. This closes the earlier `#` stringizing, `##` prefix-pasting and macro-expanded Unicode concatenation gaps. The older VFS rule must not be activated alongside this wrapper.

Literal provenance comes from Clang's actual spelling file, not an arbitrary `#line` filename. Stringizing/token-pasting scratch tokens use their macro-expansion location. Therefore ordinary literals from Game macro definitions remain Game bytes when expanded in a host TU, while a literal newly stringized in a host invocation retains the host execution encoding.

## Build interface

Build the helper once:

```
/opt/homebrew/opt/llvm/bin/clang++ -std=c++23 -O2 script/game_literal_preprocessor.cpp \
  -I/opt/homebrew/opt/llvm/include -L/opt/homebrew/opt/llvm/lib \
  -Wl,-rpath,/opt/homebrew/opt/llvm/lib -lclang-cpp -lLLVM -o <helper>
```

Compiler wrapper:

```
python3 script/game_execution_charset.py \
  --compiler /opt/homebrew/opt/llvm/bin/clang++ --helper <helper> \
  --game-root <absolute-path>/src/Game \
  --game-file <absolute-path>/src/compat/<verified-original-provider>.cpp \
  -- <the original C++ compiler arguments>
```

Both admission options can repeat. `--report <path>` is optional for bounded verification. The rule currently admits `EventUtilCompat.cpp`, `OriginalSceneWipeUtil.cpp` and `OriginalMarioSound.cpp` as complete original providers; mixed native services must encode their Game-facing inputs explicitly.

Do not let Xmake/ccache preprocess first: that discards source provenance before the wrapper runs. The normal build's original source/header dependency checks determine whether a TU recompiles. No persistent per-TU preprocessed cache or multi-gigabyte source overlay is retained. One fresh actual `GameEventFlagTable.cpp` invocation took approximately 0.80 seconds here; a minimal no-Game TU took 0.11 seconds. These are bounded timing samples, not a whole-build benchmark.

## Explicit unsupported inputs

Mixed adjacent non-ASCII ordinary Game/host literals fail because the concatenation has no unambiguous byte encoding. Convert at the native boundary or give the concatenation an explicit Unicode prefix where appropriate for a host API.

Ordinary non-ASCII character constants evaluated by `#if` / `#elif`, or defined in Game macros that could participate in those expressions, fail explicitly. Preprocessor evaluation precedes this conversion. Ordinary C++ multibyte character constants are supported and tested. PCH/modules and unexpanded response-file arguments also fail explicitly; the current root build uses textual headers and expanded arguments. No encoding heuristic or fallback silently changes these cases.

## Validation

`verify-wrapper.py` records 16 bounded successful verification steps in `wrapper-proofs.json`, including expected failures:

- CP932 ordinary strings, raw strings, UCNs, multibyte characters and `sizeof` results;
- actual macro stringizing, pasted `u`/`u8` prefixes and expanded Unicode concatenation;
- preserved host UTF-8, including host stringizing, and explicit original-provider admission;
- normal pragma-pack behavior, original `__FILE__`, DWARF compilation-unit/header names, dependency paths and error line;
- rejection of unrepresentable Game literals, mixed ordinary encoding domains and preprocessor character-constant cases;
- compiler-probe delegation and removal of all temporary compiler files.

The unchanged **whole actual `src/Game/System/GameEventFlagTable.cpp`** also compiled with the final helper. Its object contains the Japanese CP932 name and no UTF-8 copy of that name; `game-table-wrapper-proof.json` records the source/object SHA, exact command and timing. This is compiler emission proof, not full flag-table gameplay validation.

`verify-rule.py` passed all 11 steps in a copied isolated Xmake project, with a real static library, dependent executable and an Aurora-named exclusion target. `rule-proofs.json` records:

- initial `game=8ba4 host=e6 sdk=e6`, then `game=91ae` after changing the original header;
- a no-op build retaining object, archive and executable mtimes;
- original header dependencies rebuilding and relinking the affected outputs;
- Python, C++ and Lua helper edits retaining their old mtimes but each changing the content fingerprint and forcing object/archive/link rebuilds;
- the excluded Aurora target retaining its compiler, enabled cache policy and unchanged object throughout tool edits;
- original source/header paths in runtime `__FILE__` and DWARF.

The no-op check exposed Xmake persisting the wrapped compiler program across invocations. The final rule now recovers the real compiler only from its own exact wrapper path's `--compiler` argument, preventing recursive wrapping or a false `llvm-config` lookup failure.

All successful isolated commands select their project with explicit `-P`. The first attempt used only the nested working directory; Xmake unexpectedly selected the parent for its debug configuration command, then rejected the unknown isolated target before compiling any root objects. The parent was informed immediately; the corrected test never builds root objects or alters production tools during mutation checks.

Implementation hashes are in `implementation-manifest.json`. Parent integration owns the full save-owner fixture and real movement/camera replay after all explicit host text boundaries migrate.

## Native boundaries to migrate together

Current code still has explicit decoded identity consumers in ObjectNameTable, LightData, Shadow CSV/registry, DemoSheet/DemoScene/GeneralPos, EventCamera catalog, and PlanetMapCatalog. Retain raw CP932 where those values return to Game or form Game lookup keys; decode only separate display/trace values. Native calls into Game, including tests and Showcase controls, encode their known UTF-8 literals once. Original Game names reaching JSON/UI need explicit CP932-to-UTF-8 presentation conversion. Parent and the other agents own these migrations.

The former `SaveChunkHashScope` UTF-8-to-CP932 conversion must retire when Game literals become CP932; otherwise original names are converted twice. Generic save scalar byte-order conversion remains necessary and is independent of text encoding.
