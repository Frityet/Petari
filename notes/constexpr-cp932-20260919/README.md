# Explicit compile-time CP932 literals

`src/compat/Cp932Literal.hpp` provides `CP932("UTF-8 literal")`, including ordinary adjacent literal tokens inside the macro. The result is a `const char (&)[encoded_size + 1]` expression backed by inline static constexpr storage. It is not a temporary string or a runtime codec. The C++23 compiler performs all encoding; game builds do not generate or rewrite source text.

The template input retains every literal byte, including embedded NULs. A strict UTF-8 decoder rejects malformed leading/continuation bytes, truncation, overlong forms, surrogate scalars, and values outside Unicode. The frozen CP932 mapping rejects unrepresentable text at compile time. ASCII and control bytes encode identically. The output has exactly one additional final terminator; embedded NULs remain part of its exact array extent. `sizeof`, ordinary array-reference template deduction, constant-expression pointer initialization and pointer decay preserve narrow-literal use. Identical inputs share stable storage across translation units.

`src/compat/detail/Cp932Mapping.tsv` preserves the canonical 9280-entry mapping previously used by the compiler pipeline, including Windows aliases, extension characters, halfwidth kana, and private-use entries. Its SHA-256 is `46778ae55afa614d3ece7e4f7160d9f1205626f09a461e6d7a933b57b582d600`. `Cp932Table.hpp` was generated offline once from that data and is ordinary checked-in constexpr C++; no table generator is a build prerequisite. ASCII's 128 entries are handled directly. The compiler-only tests independently verify every generated table entry against the frozen TSV and Python's strict CP932 codec.

## Contracts and limits

- C++23 is the project language requirement. UTF-8 is the source/execution input contract; both narrow and `u8` literals are supported. Wide UTF-16/UTF-32 literals are not an input interface.
- Wrap the complete adjacent-literal run. An encoded array expression cannot participate in lexical string concatenation outside the macro.
- A literal's escapes are resolved by the normal compiler before the helper sees bytes. ASCII numeric/control escapes and embedded NULs work. The helper cannot distinguish a high numeric escaped byte from an identical UTF-8 input byte; input must be valid UTF-8. Raw CP932 byte strings must remain ordinary unwrapped byte literals. Root's full migration inventory found no affected mixed high-byte numeric escapes.
- `char array[] = CP932("...")` cannot initialize one array from another array expression. No such initializer was found in the affected corpus. Pointer tables and array-reference uses are supported. This is an explicit source API, not a preprocessor emulation of every possible string-token context.
- The previous compiler pipeline encoded selected narrow tokens after macro expansion and preserved raw numeric escapes. The new helper does not infer a token's origin: the source explicitly selects CP932. The full current corpus below produces byte-for-byte identical strict CP932 output, including its one embedded-NUL run.

## Validation

Portable standalone command:

```
python3 tests/test_cp932_literals.py --cxx clang++
```

On this macOS host:

```
python3 tests/test_cp932_literals.py --cxx /opt/homebrew/opt/llvm/bin/clang++ --output notes/constexpr-cp932-20260919/tests.json
```

PASS with LLVM 23, C++23, `-Wall -Wextra -Werror`. All **9408 ASCII and mapped codepoints** compiled to exact expected bytes. Focused positive checks cover adjacent literals, normal compiler UCNs, raw strings, escaped UTF-8 bytes, halfwidth/Windows/private-use mappings, embedded NUL/empty strings, array extent/type, and static lifetime/address identity across two translation units. **12 negative translation units** fail compilation with the expected diagnostics, including valid-but-unrepresentable Unicode and malformed UTF-8. `tests.log` and `tests.json` preserve the result.

The independent real-corpus check, `verify_corpus.py`, reads the parent's pre-migration snapshots using a separate lexer, then compiles each literal expression twice: once as the ordinary UTF-8 input array, once through CP932. The binary writes framed pairs including complete array extents and terminators. Every encoded output is checked against `source.decode("utf-8").encode("cp932")`.

Result: **330 files, 3580 literal runs**, 72679 original bytes and 49921 encoded bytes, including all terminators; **one embedded-NUL run preserved**. No array-initializer hazards were detected. `corpus-results.json` records compact hashes; generated test source and binary were temporary and are not committed. This confirms compiler escape and concatenation semantics for every migrated literal, beyond the single-codepoint tests.

Integration audit: the `INIT_AUDIO_KEY` macro is used only as an ordinary pointer argument, so its encoded expression remains valid. `MarioEffect.cpp` checks `sizeof("共")` and `sizeof("属")`; the helper returns their exact three-byte encoded arrays and preserves the original size-dependent branches. The full native build is owned by the root task and recorded under `../explicit-game-encoding-20260919/`.

Expected runtime behavior is unchanged encoded names, format text and original table lookup keys, with ordinary direct C++ compilation replacing the source-rewriting pipeline. These compiler tests do not by themselves establish completion of Gateway gameplay; the root task owns native process validation.
