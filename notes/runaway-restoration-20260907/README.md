# Restore prior runaway NPC decompilation — 2026-09-07

The separate decompilation reference retained only the collector foundation and
no RunawayRabbit source after the PC-port repository was flattened. Restore the
previous completed source from parent commit
`e1985ac3a9736a9c432c1d7af6ce8bf96e3abed4` (`1347a481f^`), preserving the current
upstream baseline `151d5a53a` and local reference checkpoint `2b7a64332`.

## Changes and provenance

Four files are byte-identical restorations from that parent commit:

- `src/Game/NPC/RunawayRabbitCollect.cpp` (300 lines; replaces 41-line foundation)
- `src/Game/NPC/RunawayRabbit.cpp` (503 lines; previously absent)
- `include/Game/NPC/RunawayRabbitCollect.hpp` (restores destructor declaration)
- `include/Game/NPC/RunawayRabbit.hpp` (restores destructor declaration)

The matching RunawayTico source and header were compared and already matched
that commit exactly. They are unchanged. No new gameplay reconstruction, timing
adjustment, host shortcut or API substitute was introduced. The restoration
follows `AGENT_DECOMP_GUIDE.md` and preserves the prior decomp source conventions.
The parent history records the prior restoration as `bac110f54`.

The collector restores child creation, catch grouping, message-controller
linking, sibling activation, catch notification, the wait/active nerves, Tico
comment selection and the original switch/message calls. RunawayRabbit restores
the full existing actor source needed for those calls. This restoration makes
the reference usable for subsequent PC imports; it does not enable either actor
in the current PC factory.

## Fresh validation

All three translation units, including unchanged RunawayTico, compile using the
retained GC/3.0a3 `mwcceppc.exe` through `wibo` and `sjiswrap.exe`, with the current
`configure.py` Game flags and include order. The exact argument vectors are
recorded locally in the parent note directory's `compile-results.json`.

Target objects were freshly split with `dtk dol split --no-update` using current
`config/RMGK01/{symbols,splits}.txt`, a copied configuration with absolute input
paths, and the retained original main.dol. Its SHA-1 was independently checked:
`25c5959534b3c21246c6c7e42021b916b41fb578`, matching the repository RMGK01 config.
The `--no-update` option preserved the original split and symbol files.

| Actor | Text fuzzy match | Exact / paired code symbols | Target text bytes |
| --- | ---: | ---: | ---: |
| RunawayRabbitCollect | 97.40519% | 13 / 17 | 2468 |
| RunawayRabbit | 95.95944% | 42 / 61 | 5720 |
| RunawayTico (unchanged) | 98.82346% | 32 / 39 | 3784 |

Every target code symbol pairs with a compiled symbol. Collector `.data`,
`.ctors` and `.sbss` match completely. Rabbit and Tico have lower whole-section
`.data` similarity, so these results are text matches, not a claim of a complete
byte-identical binary. The source was restored exactly instead of being tuned
for a new matching score. `git diff --check` passes.

Machine-readable provenance and match summaries are retained beside this note.
Full generated target objects, compiler outputs, objdiff reports and per-symbol
TSVs are local artifacts under the parent repository's
`notes/gateway-audit-20260907/restoration/`; proprietary binary data is not added
to the repository. No full decompilation link or playable PC chase is claimed.
