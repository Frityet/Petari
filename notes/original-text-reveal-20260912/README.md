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
