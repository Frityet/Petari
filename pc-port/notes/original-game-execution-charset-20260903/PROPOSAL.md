# Compiler-only CP932 execution literals

This is a staged compiler-view proposal. No production Game files, root compiler configuration, native shared build, or resource bytes were changed. Runtime activation must accompany the host/Game boundary work documented in `../host-game-text-boundary-20260903/README.md`.

## Implementation

- `staged/tools/game_literal_lexer.cpp` uses the actual LLVM 23 Clang raw lexer. It reads token offsets/kinds from the original UTF-8 source, including disabled preprocessor branches. Token boundaries are not approximated with a regular expression.
- `staged/tools/game_execution_charset.py` changes ordinary narrow string and character payloads only. Non-ASCII characters and universal character names become CP932 bytes represented by fixed three-digit octal escapes. Existing numeric/simple escapes retain their spelling. Raw strings become ordinary strings with the same execution bytes; physical LF/CRLF/CR newlines normalize as Clang does, while generated line splices preserve source line locations. Raw delimiters and user-defined suffixes are handled from lexer-owned tokens.
- Wide, u8, u, and U literal kinds remain untouched. Direct adjacent unprefixed literals inherit the Unicode/wide kind of their concatenation group, including when separated by comments. Comments, identifiers, include/header names, diagnostic/directive strings, and ordinary host sources remain unchanged.
- Compiler views are content-addressed, atomically written, and checked by hash before cache reuse. Source changes during tokenization are rejected. The VFS overlay sets `use-external-names: false`: includes, `__FILE__`, dependency paths, debug paths and diagnostics use the original source identity. The code does not replace source files or change file names under Game.
- Original Game roots must be admitted explicitly. Mixed compatibility files require a manifest containing the native file hash plus byte-identical ranges from actual original source. Other literals in those files remain UTF-8. Do not treat the entire compatibility directory as original Game source.

## Validation

Run, from the repository root:

```
python3 pc-port/notes/original-game-execution-charset-20260903/verify-native.py
python3 pc-port/notes/original-game-execution-charset-20260903/verify-original.py
python3 pc-port/notes/original-game-execution-charset-20260903/verify-build-hook.py
```

The first script builds only the isolated lexer and small compiler probes. The second uses the existing original `sjiswrap` + GC3.0a3 compiler. The third runs an isolated xmake project under `build/original-game-execution-charset-20260903/isolated-xmake`, not the native project.

Results:

- Full inventory: **3,876** root/native Game source and header files; **7,590** converted literals in **817** mapped files. All 50 wide string/character tokens remain untouched. No unrepresentable CP932 literal was found in this corpus. This is a full lexical scan, not a build of all root Game algorithms.
- Twenty lexical cases cover ordinary strings/characters, UCN forms, escaped backslashes, numeric escapes beside converted text, raw delimiters, line splices, raw LF/CRLF/CR, comments/identifiers, includes, all Unicode literal kinds and concatenation. Three invalid/unrepresentable narrow cases are rejected; explicit UTF-8 emoji remains unchanged.
- Six original-compiler probes have exactly the same object data bytes as the native VFS executable. These include Japanese prefixes, adjacent literals, embedded NUL/simple escapes, a multibyte character constant, and CP932 characters whose second byte is 0x5C (`表ソ十`). This establishes the conversion against the actual original compiler path, not just a second native encoder.
- The existing `MarioActor::isCommonEffect` and `isMaterialEffect` bodies are extracted verbatim into a small fixture. They accept the converted `共通常` and `属性着地` literals using their original 0x8BA4 / 0x91AE checks. No alternative prefixes or Game algorithm changes are introduced.
- The native probe verifies UTF-8/UTF-16/UTF-32/wide literals, raw contents, original `.d` paths, unchanged source hashes, repeat cache hits without rewritten overlays, one-header invalidation, repair of a damaged compiler-view cache, and strict original-extract provenance.
- A deliberate error after a multiline raw literal reports the original file at line 3. The isolated xmake object’s DWARF line table references the original header. A mapped header's `__FILE__` remains the original relative spelling (`./Game/Literal.hpp`), without a generated view path.
- The isolated xmake fixture has a real static library and dependent executable. It preserves UTF-8 in the unmapped host source while the mapped Game header returns CP932. An unchanged build reuses the object. Same-timestamp header literal changes rebuild the objects, archive and dependent executable; generator changes also invalidate the target. The runtime output changes from `game=8ba4` to `game=91ae`, while `host=e6` is unchanged.

Evidence: `native-proof.json`, `original-proof.json`, `original-compiler.log`, `native-fixture.log`, `diagnostic-path.log`, `build-hook-proof.json`. Exact build commands are retained. Large token inventories and compiler views remain under ignored build output.

## Proposed build integration

`staged/tools/game_execution_charset.lua` is a working isolated xmake rule. It generates views in `on_config`, before xmake reads object dependencies or caches compiler flags. Content hashing avoids re-lexing unchanged inputs. It adds the VFS flag plus tool/provenance and converted-payload fingerprints to the compiler flags.

The payload fingerprint is necessary: a real probe showed that xmake's coarse mtime checks reused an object after an encoded header literal changed. After forcing object compilation, the same mtime issue could still reuse the executable. The rule therefore marks the target with xmake's normal rebuild state when its last **successfully built** encoding fingerprint differs, and writes that success stamp only after the target finishes. This covers both archive and link stages without arbitrary sleeps or changing source timestamps. Ordinary source/comment edits retain normal original-file dependency behavior; rare execution-literal changes intentionally invalidate the target.

For atomic native activation:

1. Build the lexer against the configured LLVM installation and provide its path as `game.charset.lexer`; the fixture uses the existing Homebrew LLVM 23 installation.
2. Apply the rule to every native C++ consumer of original Game headers and every final target linking the affected archive, including tests. The VFS maps only admitted files, so attaching it to a host source does not convert that source's UI strings. Applying it only to the Game archive is insufficient for inline Game literals and same-timestamp dependent-link invalidation.
3. Admit native `src/Game` and any original fallback Game headers explicitly. Source-only root cpp files need not be mapped for a native build unless actually compiled. Add compatibility extraction records individually after provenance review.
4. Retire incompatible host text assumptions in the coordinated runtime boundary checkpoint. `JMapInfo` already retains CP932 bytes; resource bytes must stay authoritative.
5. Preserve the original Game compiler options and narrow numerical special cases. This rule changes neither arithmetic flags nor gameplay source.

## Explicit limits before activation

This filter operates before preprocessing. Direct adjacent literal kinds are handled, but a Unicode/wide prefix introduced only through macro expansion, token-pasting a prefix onto a literal, or stringizing a literal's **source spelling** requires separate preprocessing-aware treatment. Source escapes necessarily change the spelling seen by `#`. This proposal is not a universal C++ execution-character-set implementation for those constructions.

The current scanned Game corpus has one non-ASCII literal in a macro definition: `GameSystem.cpp::INIT_AUDIO_KEY`. Its three uses pass ordinary narrow names to async execution helpers. No Game macro that pastes a wide/UTF prefix was found. These findings support the current source boundary but are not a substitute for rejecting or auditing new spelling-sensitive macro uses. Do not silently activate this generator for arbitrary third-party or mixed host source.

Likewise, direct non-ASCII ordinary literal text must be representable in CP932; the generator deliberately reports an error otherwise. Comments and Unicode literals are not subject to that restriction. Native wide-character ABI differences are a separate SDK issue; preserving a wide literal's source spelling does not claim the native wchar_t layout matches Wii.

CP932 has duplicate byte encodings. Runtime decode-to-UTF-8 followed by encode-to-CP932 is **not** an identity-preserving substitute for retaining original BCSV/JMap/archive strings. This tool supplies the original execution bytes for source literals only. It does not transcode resource names, change Mario's prefix checks, or resolve all host presentation boundaries.
