# Original DemoRabbit reference recovery — 2026-09-19

The port already carried the complete DemoRabbit recovery from August, but the current decomp submodule retained only its constructor, destructor, and archive selector. Restore that existing recovery to the canonical decomp source/header so future guide investigations have an auditable original reference. This checkpoint changes no native Game behavior.

Followed `decomp/AGENT_DECOMP_GUIDE.md`. Removed the native CP932 wrappers. The native-only `zeroInline` alias is expressed as `set(0.0f, 0.0f, 0.0f)` in decomp; no JGeometry donor changes were necessary. `native-equivalence.json` records the complete normalized source/header comparison.

Focused MWCC compilation against the current RMGK01 reference passes. The 4,668-byte text section matches 99.45844%; `updateRun` matches 98.23364%, `exeGuide` 98.29214%, and all reconstructed actor functions exceed 98%. This is a functional recovery, not a claim of byte-perfect code. The data section retains the same literals with different pool ordering (notably `Change`) and vtable placement; the aggregate data match is 47.61905%. All actor/nerve/functor vtable symbols individually match 100%.

The full reference text was compared to the actual RMGK01 retail DOL, SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`, masking only explicit ELF relocation fields. All 4,668 bytes pass. `verify.py` reproduces compilation, object comparison and retail-reference verification; object binaries and disc data are excluded from publication.

No new rabbit traversal or story completion is established by this reference-only checkpoint. The next original-process chase uses the separately verified Mario movement restoration and normal scene actors.
