# Original GameEventValueChecker retained read destinations

Recovered only `GameEventValueChecker::deserialize` in decomp, following `AGENT_DECOMP_GUIDE.md`, then copied the whole original source to native. The parent had already copied the exact header; it was verified identical and left unchanged. The StoryEventBCSV cohort remains frozen.

## Actual retail contract

Retail `GameEventValueChecker.s:156–222` (803B57A8, 240 bytes) computes **input size / 2**, and each iteration reads **two u16 fields**. It deliberately reaches EOF during the second half of a complete input's loop. The raw read destinations are distinct fixed stack slots at `r1+0xA` (hash) and `r1+0x8` (value). It snapshots the hash after its read, then snapshots the value after its read, before the lookup and possible write. Raw EOF reads copy zero bytes, so those two slots retain their last successfully read values.

The previous C++ called `readU16()` twice. That happened to match the Wii stack arrangement at 100%, but each helper's indeterminate temporary does not preserve a defined destination lifetime across native calls. The general native helper initializes its temporary to zero; applying that to this original loop would change its EOF lookup results.

The recovery introduces two persistent raw-read destinations outside the loop, plus the original per-iteration hash/value snapshots immediately after their respective reads. It preserves the size/2 bound, both raw read calls, lookup sequence, writes, sticky unknown-hash result and final stream-state clearing. No Game-specific caching was added to JSU and no `/4` rewrite or guessed EOF padding was introduced.

Valid VLE1 serializers emit complete four-byte hash/value records. After the first complete pair, both retained destinations are initialized and EOF reuse is defined in C++. The file-format validator owned by the integration task rejects malformed partial pairs before mutation. This recovery does not invent deterministic initial bytes for an invalid/truncated first record. Empty payloads execute no loop iterations.

## Proof

- Unchanged baseline whole-TU Wii compile and objdiff both exit 0; baseline method was 100%/240 bytes under the current corrected JSUIosBase header.
- Recovered whole-TU Wii compile and objdiff both exit 0: **98.583336%, 240 retail/candidate bytes**. `function-scores.json` records remaining register/load differences; call targets, loop bound and field updates are preserved. The first simpler raw-read candidate was discarded because it moved load timing and compared below the requested threshold; it was not mirrored.
- Native whole-TU syntax check exits 0 against the current general JSU headers.
- Native/decomp source and header equality verified; `git -C decomp diff --check` exits 0.
- Exact compiler commands, object/source/SDK header hashes and patch are recorded in `baseline-proof.json`, `recovered-proof.json`, `native-syntax.json`, `source-manifest.json`, and `reference-recovery.patch`.

No Xmake, commit or index mutation was performed. Syntax/Wii comparison is not claimed as integrated save runtime success.

## Coordinated runtime cases

The stream owner confirmed raw reads copy only the available prefix, preserve remaining destination bytes, advance by copied bytes and set IO_ERROR for a short `read`. The save-codec fixture owner was given these checks against the actual original method:

1. One known complete record (4 bytes) returns 0 and preserves its value across **two** loop iterations.
2. Two distinct known complete records (8 bytes) return 0 and preserve both values across **four** iterations; EOF iterations repeat the second pair.
3. An unknown hash followed by a known final hash retains result 1 and still applies the known value, proving the error remains sticky while the final pair repeats.
4. Partial pairs reject at the bounded format boundary before modifying original state; this is distinct from PLAY's intentional legacy partial-prefix/default behavior.

These cases also exercise the separately implemented scoped CP932 save-ID hashing and native-byte payload conversion. Integrated runtime results belong to that fixture's final proof; none have been fabricated here.
