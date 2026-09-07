# Original effect utilities — 2026-09-07

The six restored translation units compile successfully with the original
Metrowerks GC 3.0a3 compiler through wibo and sjiswrap. `wii-compile-results.json`
records every complete command, source hash, log path, and successful exit code.
Run those argument arrays from `decomp/` to repeat the builds. AutoEffectInfo's
initial missing `strtoul` declaration was resolved before this proof run.

Retail references are the original objects in
`notes/gateway-audit-20260907/restoration/retail/obj/Game/`. The six saved
`*.objdiff.json` reports contain complete instruction comparisons. Reproduce each
with `decomp/build/tools/objdiff-cli diff -1 RETAIL.o -2 COMPILED.o -o REPORT.json
--format json-pretty`. `summarize-wii-proof.py` regenerates the compact JSON and
per-symbol Markdown reports from these comparisons and validates the attribute
lookup jump table directly from the ELF relocations.

| Translation unit | Retail functions supplied | Objdiff 100% | Lowest score |
| --- | ---: | ---: | ---: |
| AutoEffectInfo | 10 | 5 | 99.191666% |
| AutoEffectGroup | 6 | 5 | 99.805824% |
| AutoEffectGroupHolder | 8 | 8 | 100.000000% |
| EffectSystemUtil | 32 | 27 | 93.571430% |
| EffectUtil | 43 | 41 | 72.400000% |
| HashUtil | 11 | 11 | 100.000000% |

All 110 retail function symbols are supplied; 97 score 100%, and 109 score at
least 90%. These are per-function objdiff scores, not claims that complete object
files including layout, local labels, debug information, and data are identical.
`wii-symbol-matches.md` gives every symbol and its baseline comparison.

The baseline is the exact restored source from commit
`e1985ac3a9736a9c432c1d7af6ce8bf96e3abed4`, independently compiled in `preflatten/`
using the same current headers and compiler flags. All 100 previously supplied function scores are unchanged relative to
the baseline rebuilt with the same final headers. The value-owning legacy
bind2nd correction improves two additional exact matches in both builds. This is a source comparison under a common header
environment, not a reconstruction of every historical header dependency.

Ten EffectSystemUtil functions are newly supplied. The eight movement, drawing,
and deletion lifecycle wrappers each score 100%. `getEffectAttributeName` scores
98.043480%; its 28 jump-table entries, for floor codes 5 through 32, have exactly
the original function-relative relocation targets. The remaining differences
are local data labels/relocations. It preserves the original Water, Sand, Ice,
DamageFire, Mud, and Default mapping. `createAutoEffect` scores 93.571430% and
retains the original 308-byte size, UniqueName lookup using the second argument,
GroupName read, allocation, construction, and initialization. Its differences
are constant loading order, local data labels, and allocation-result register
selection. The original first argument is unused.

The existing `addEffectHitNormal` score of 72.4% is unchanged from baseline. Both
objects add HitMarkNormal, conditionally rename it when the supplied name is
non-null, and call the same EffectKeeper methods. Its instruction scheduling and
choice to reload or retain the string address account for the mismatch; the
candidate is 96 bytes versus the retail 100 bytes. No correctness regression was
found in this bounded review, and no source change was made by the verifier.

This validates Wii code generation against the retail objects. Native resource
and lifecycle execution are separate checks; it does not establish visible
particle rendering or full gameplay.
