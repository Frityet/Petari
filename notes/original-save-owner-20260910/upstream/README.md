# Upstream SDK merge validation, 2026-09-10

The decomp merge is suitable for parent review/publication on this bounded evidence. No production source, build configuration, Git index, or commit was changed by this audit. The decomp working tree was clean after validation.

Merge under review: `499e1034d7f8aad6b1773c36cea8d2f9a9ac2e81`, authored and committed by codex. Its first parent is the local recovered-source checkpoint `6c6cdb24d`; its second parent is upstream `73f5b40dc`. Upstream advanced three commits from the previous `16906807c` anchor: NW4R completion (`0bfa6b46d`), its upstream merge (`3cc98f96b`), and JKRDecomp improvement (`73f5b40dc`). The first-parent delta covers 36 files, including compiler/configuration declarations and SDK source/header completion. Commit titles reporting percentages are upstream claims; this audit does not replace them with an unmeasured full matching claim.

## Conflict and original-toolchain proof

The resolved `libs/nw4r/include/nw4r/ut/RuntimeTypeInfo.h` is byte-identical to the incoming upstream header (Git blob `b1e97e7eb227c129873975e8401656dc4c1c67f4`). Both the existing `NW4R_UT_RTTI_DECL` / `NW4R_UT_RTTI_DEF_DERIVED` family and the const `NW4R_UT_RUNTIME_TYPEINFO` family remain present. The incoming `detail::GetTypeInfoFromPtr_` template supplies the target type to the same parent-chain identity check. Picture, Window, and Bounding still use the retained macros; Layout's newly completed tag-processor traversal exercises `DynamicCast<TextBox*>`.

Eight complete original translation units compiled with exit 0 using the current configure.py RMGK01 `cflags_jsys` / `cflags_nw`, GC/3.0a3, sjiswrap, and wibo: JKRDecomp, lyt_layout, lyt_picture, lyt_bounding, lyt_window, lyt_animation, lyt_textBox, and ut_RomFont. `wii-compile.json` records every exact argument, source hash, output object, and result. Configuration was read as data; no Ninja/Xmake regeneration or shared build occurred.

`RuntimeTypeInfoProbe.cpp` additionally mixes both macro families over a three-level inheritance chain. It checks exact type, ancestors, unrelated descendants, null input, const downcasts, and a null target type. The merged header compiles with the Wii flags, and both merged and current native headers independently pass all 12 runtime checks under LLVM 23 ASan/UBSan with compiler RTTI disabled. See `rtti-probe.json`. The first host attempt lacked Aurora's include directory; `rtti-probe-first-attempt.json` retains that compile-only invocation error, fixed without header changes.

This proves the resolved declaration/template composition and representative original compilation. It is not a full Wii relink, a complete NW4R test, or gameplay validation.

## Native correspondence and bounded carry proposal

### RTTI: no required native behavior change

`src/nw4r/ut/RuntimeTypeInfo.h` uses the previous public `GetTypeInfo` helper spelling with equivalent pointer/const behavior. The same 12 runtime checks pass against that actual header. No external use of the old helper was found in native source. Copying the new spelling is optional source synchronization, with no demonstrated behavioral fix; no change is needed to unblock this merge.

### JKRDecomp: real ownership correction, no active native owner to patch

The incoming `JKRDecomp::create` and `prepareCommand` allocate in `JKRHeap::sSystemHeap` rather than `sGameHeap`. Retail directly confirms the heap loads at `0x804167B8` and `0x80416974` in `notes/gateway-audit-20260907/restoration/retail/asm/JSystem/JKernel/JKRDecomp.s`. Retain this correction whenever the actual original async decompression owner is activated.

There is currently no native JKRDecomp implementation/provider. `src/resource/Yaz0.cpp` supplies bounded native archive decompression, used by RarcArchive and native audio resource loading; FileUtilCompat has its separate archive boundary. The incoming SZS byte-consumption rewrite is algebraically equivalent to the old loop, while its original pointer-width and direct endian assumptions remain unsuitable for an unreviewed native copy. Do not introduce a fake JKRDecomp thread or transplant the unrelated system-heap rule into host scratch storage.

### Aurora BRLAN: demonstrated semantic gap, safe SDK-level follow-up

The completed original `lyt_animation.cpp` provides concrete curve semantics that differ from current `aurora/lib/nw4r/brlan.cpp`:

- Step interpolation performs a binary search and selects the right key when `-0.001F < frame - key.frame < 0.001F`; Aurora currently holds the left key until the exact key time.
- Hermite search uses `frame <= center.frame`. In the same strict tolerance interval, it returns the right key's value, or the immediately following key's value if that following key has exactly the same frame. Aurora currently interpolates toward the first duplicate until reaching its exact frame.
- Original Hermite arithmetic uses frame offset and reciprocal interval intermediates in a defined order. Aurora's normalized polynomial can round differently even away from the tolerance branch. Its additional tiny-duration early return is not in this original function.

Retail confirms the tolerance comparisons at `0x80014540–0x80014588` and `0x80014638–0x800146AC`, duplicate selection at `0x80014678–0x800146A4`, and the constants `-0.001`/`0.001` at `0x806B7D08`/`0x806B7D0C` in the saved `nw4r/lyt/lyt_animation.s`.

`ReferenceCurveHelpers.hpp` contains the exact unchanged helper bodies extracted from the merged original source. Only primitive/key type aliases adapt their input container; `reference-extract.json` records the source and extracted-body hashes. `BrlanCurveDeltaProbe.cpp` compares these helpers against actual public `BrlanAnimation::pane_frame` calls, linked with current Aurora `brlan.cpp`. LLVM 23 ASan/UBSan build and execution both exit 0; that result means the audit reproduced the expected gaps, not that native compatibility passed. Sixteen comparisons produce three differing results:

| Frame | Channel | Original | Current Aurora |
| --- | --- | --- | --- |
| 9.998 | Hermite translation | 9.99999809 | 9.99999905 |
| 9.9995 | Step visibility | visible | hidden |
| 9.9995 | Hermite translation with two keys at 10 | 20 | 10 |

The bounded fix proposal is to port these two original search/evaluation algorithms into Aurora's existing span/key representation, preserving resource ownership, caller animation scheduling, and parser validation. Add direct public-API regressions for tolerance inside/outside/exact boundaries, duplicate keys, clamps, and normal slope interpolation. No Game source or layout scheduler change is required. This is a demonstrated general SDK semantic difference; no claim is made that an observed demo bug currently reaches these particular authored key/frame combinations.

### Remaining completed NW4R surface

The upstream typed Pane factory and recursive TextBox tag-processor assignment, material/resource declaration completion, and RomFont completion are useful original references. Native layout currently uses its own `src/nw4r` declarations, `Nw4rLayoutRecords`, LayoutManagerCompat/LayoutRuntime, and Aurora parsers/rendering; these are not layout-compatible substitutes for blindly copied Wii classes. The reviewed small common/group/pane rewrites did not reveal an additional required behavior change. A full original typed Pane/font/material activation requires an explicit owner/renderer integration task beyond this merge audit.
