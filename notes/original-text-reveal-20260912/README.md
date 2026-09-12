# Original custom text reveal recovery — 2026-09-12

This work restores the game specialization `nw4r::ut::TextWriterBase<wchar_t>::PrintImpl` and seven original `CustomTagProcessor` tag groups. New bodies were authored under `decomp/src/Game/Screen/` first, compiled with the original Wii toolchain, then handed to the process-services agent for merging into canonical `CustomTagProcessor.cpp` and native integration. The temporary fragments retained here are proof snapshots; they are not alternate runtime providers.

## PrintImpl

Retail reference is `decomp/build/RMGK01/obj/Game/Screen/CustomTagProcessor.o`, address `80350D04`, size `0xD70`. The specialized method performs the actual custom-processor reset, per-character alpha, character index advancement and last-character tracking. Its normal drawing algorithm also handles cursor alignment, line measurement, tag lookahead, synthetic newline replay, character spacing, fixed glyph width, baseline positioning and restoration of the original alpha.

`PrintImpl.wii-build.json` and `PrintImpl.native-build.json` record successful original compiler and native Clang probes. Raw object similarity is **20.78%**, not a high fuzzy match. Retail emits extensive Wii memory-region assertion code through SDK inline methods, while current reconstructed SDK methods omit those assertions. No Wii address masks were introduced into the native game source.

To establish the functional behavior independently of that low raw similarity, `retail-differential.py` loads both the **actual retail object** and the freshly compiled recovery into a PowerPC Unicorn executor. It relocates their real instructions and runs them with identical deterministic SDK callbacks. The actual PrintImpl conditional branches and arithmetic execute in both objects. Only paired-single stack save/restore instructions and general register save/restore callbacks are skipped; the oracle does not replace any PrintImpl branch. SDK callbacks provide cursor adjustment/line measurements, font metrics, character reading, tag processing and glyph recording. The alpha callback is deterministic, making the timing/order of alpha queries and index changes visible in glyph output; alpha's actual math has a separate recovery probe.

All **21 scenarios match exactly** for return width, final cursor, final alpha, character index, last character and ordered glyph events (code, X/Y, alpha). The scenarios cover:

- default and custom processors; empty input; explicit newlines and end-draw tags;
- forced wrapping, oversized first glyph, wrapping around a tag and replay of that tag;
- spacing control, fixed width and scaled font metrics;
- left/center/right origins and alignments, top/middle/bottom/baseline vertical positioning.

The raw outputs and original/recovered object SHA-256 hashes are in `PrintImpl.differential.json`. This proves the tested loop semantics at defined SDK boundaries. It does **not** establish integrated GX text rendering or full dialogue gameplay.

Reproduce with Python packages `unicorn==2.1.4` and `pyelftools==0.33`, then run the script from any directory. The local proof environment is `/tmp/petari-printimpl-oracle-venv`.

## Tag groups

`Groups.cpp` and `Groups.wii-build.json` contain the exact recovered source and successful compiler command. `Groups.objdiff.json` compares the following methods to their actual retail object bodies:

| Method | Retail address | Fuzzy match |
| --- | --- | --- |
| exeDisplayGroup | 8035222C | 100% |
| exeSoundGroup | 803522D8 | 98.07% |
| exeFontSizeGroup | 8035251C | 90.89% |
| exeSystemGroup | 80352614 | 100% |
| exeLocalizeGroup | 803526A8 | 100% |
| exeNumberGroup | 80352704 | 99.53% |
| exeStringGroup | 80352834 | 100% |

The recovery identified that processor bytes `0x30` and `0x31` are an unsigned sound bit mask and unsigned sound index, rather than bools. Header ownership was coordinated with the process-services agent, which corrected them to `u8` before proof compilation.

The original number group intentionally falls through every subsequent format case and finally `%d`; this behavior is preserved. Its original code and string group directly overlay a packed tag payload with integers/pointers. Native integration must use the already agreed native code-unit representation: original pairwise numeric values via `getParam32`, alignment-safe native pointer read/write over two widened code units. The process-services agent owns that architecture correction; the proof fragment retains the original Wii representation.

## SDK accessors

Recovered missing `nw4r::ut::CharWriter` inline accessors in both SDK reference and native headers: `MoveCursorX/Y`, `GetAlpha`, `SetAlpha`, `GetTextColor`. They operate on the actual original writer fields. SetAlpha invokes UpdateVertexColor after storing alpha, as retail `80353CB4`; GetTextColor returns the start color, as retail `80353708`. No parallel writer state was added.

## Related alpha work

Five alpha-controller methods, corrected object layout, original compiler receipts and a native alpha boundary probe are in `notes/original-custom-tag-alpha-20260912/`. Together they provide the original timing state used by the specialized text writer.

## Nested display groups and mutable origin

Recovered the original display wait/offset/center methods in `Display.cpp`, compiled with the original compiler (`Display.wii-build.json`). Wait is 100%, offset 90.89%. The isolated center fragment emits a large SDK writer copy inline, so raw fuzzy comparison is not a useful semantic measurement there; the recovered control flow and context-origin write were checked directly against the retail body. The actual retail center method writes `PrintContext::xOrigin`. Corrected that SDK member to mutable `f32` in both reference and native headers; `yOrigin` remains const. This avoids a const-cast write through a const member. The process-services agent owns final shared translation-unit integration.

## Independent Ruby proof

Reviewed the recovered Ruby group against original PowerPC branches and found that `rubyLength` must be signed, although initialized by an unsigned parameter-length expression. Its later alpha-index division uses signed `divw`; an unsigned declaration changed negative-index behavior. The canonical source now uses `s32`. `RubyProof.cpp`, `RubyProof.build.json`, `ruby-differential.py`, and `Ruby.differential.json` record a direct original-versus-recovered PowerPC execution comparison: **7/7 cases pass**, including Japanese dispatch, measurement-only behavior, base/ruby width asymmetry, negative indices, fractional alpha rescaling and restoration of all alpha-controller fields after recursive printing.

The emulator proof executes the retail and recovered branch/arithmetic bodies with identical SDK callbacks. It does not independently validate font rasterization or integrated dialogue draw timing. ABI register-save/restore helper callbacks are no-ops in this bounded harness because nested SDK calls are callbacks; extending it to nested actual game functions requires implementing those helpers.

## Original replacement processor cohort

Recovered all 12 original ReplaceTagProcessor functions and both dispatch tables directly into `decomp/src/Game/Screen/ReplaceTagProcessor.cpp`, expanded its original header, then copied both to native Game. The native copy is byte-identical and passes Clang. `ReplaceTagProcessor.wii-build.json`, `.native-build.json`, `.function-proof.json`, and `.objdiff.json` contain exact receipts. Lookup functions, string, localization, player-name, variadic wrapper, va_list copy and both tables match 100%; Replace is 93.54%, ReplaceArgs 93.58%, number 99.65%, race time 96%, picture 82.98%.

Character/tag copying uses `sizeof(wchar_t)` and reads tag group from the logical code unit, preserving both original big-endian Wii and native widened-character representations without raw low-byte assumptions. The original MSL headers lacked a `va_copy` macro; added its ordinary array-struct copy definition in the reference SDK, while native code uses the host standard library. Full integrated replacement behavior remains owned by the process-services integration tests; standalone source compilation is not a gameplay proof.

## Disabled alpha architecture regression

After the process-services agent copied the original alpha bodies with the existing `aurora::ppc::truncate_s32` boundary helper, refreshed the extracted native probe from that exact source. Added three updates on a newly constructed inactive controller: the original method still increments its frame even though its frame step is zero, and the PowerPC conversion saturates positive infinity. The expanded probe passes with undefined-behavior and float-cast-overflow sanitizers. Receipts and hashes are in `notes/original-custom-tag-alpha-20260912/native-proof.json`.
