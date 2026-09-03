# BckCtrl recovery and residual comparison

The missing `add`, data assignment, `find`, and `reflectBckCtrlData` methods were
recovered in the authoritative root source first, then copied unchanged into
the PC Game tree. The native header is also an exact root copy. The count and
used-count fields are signed, matching retail signed comparisons and loops;
their Wii layout is unchanged. Explicit XanimePlayer/StringUtil includes close
the native compilation boundary without altering behavior.

`verify-bck-jmap.py` recompiles with the configured GC3.0a3 Game flags and the
Shift-JIS wrapper, verifies the known RMGK01 DOL SHA1, compares the split retail
object with objdiff, and checks the 100-byte data assignment against relocated
retail bytes at 0x80017C70. The recovered methods compare as follows:

| Method | Retail bytes | objdiff |
| --- | ---: | ---: |
| add | 288 | 93.81944% |
| data assignment | 100 | 100% |
| find | 204 | 92.94118% |
| reflectBckCtrlData | 332 | 95.54217% |

The existing constructor compares 99.78102%; existing overWrite,
changeBckSetting, and BckCtrlData constructor compare 100%. These are compiler
comparison results, not claims that every recovered routine is byte identical.

The instruction review checked these original contracts:

- `add` copies into the next available slot before shifting entries. Empty
  names sort first; other names use MR::strcasecmp. Equal keys are inserted
  before existing equal keys. The capacity test is signed; no allocation,
  copied name, or alias-protecting temporary is introduced.
- `find` performs a lower-bound search and then checks case-insensitive equality.
  It uses only the actual used range, without changing ordering or empty-table
  behavior.
- Assignment copies the borrowed name pointer, all five signed 16-bit settings,
  and all four trailing bytes, including fields without descriptive names.
- Reflection checks start against the old end, then end against that end, then
  repeat against the updated end. Start also resets frame and loop and records
  the player's resulting frame. A zero play duration gives rate zero. Negative
  intervals remain negative; the method does not normalize or clamp them.
- Interpolation is applied before the new loop attribute. Consequently the
  actual XanimePlayer interpolation method observes the preceding loop mode.

Remaining code-generation differences were reviewed explicitly. `add` reuses
the count loaded for its capacity comparison instead of reloading it, schedules
those initial loads differently, and omits a sign extension before a zero
comparison. `find` removes a redundant pointer add/subtract and uses a signed
comparison instead of extracting its sign bit; its working registers differ.
Reflection uses different registers and drops multiplication by literal 1.0 in
the nonzero play-duration branch. That branch divides a finite signed-16-bit
endpoint difference by a positive signed-16-bit duration, so the eliminated
multiply cannot affect that branch's finite result. Calls, setting offsets,
comparison inclusivity, and their order remain the same. No generic math helper
was modified to improve these percentages.

`OriginalBckCtrlTests.cpp` contains seven independent parameter cases using an
actual J3DModel, XanimeResourceTable, XanimePlayer, frame controller, and Core.
Its syntax-only probe passes. Linking/running is delegated to the parent's
coordinated build and is not claimed by this note. Constructor/find/add runtime
coverage awaits the real ResourceHolder ownership migration; the test does not
pretend that the archive-only wrapper has the original Game layout.

Run the reproducible comparison and shared-name lifetime checks with:

```sh
python3 pc-port/notes/original-resource-holder-20260903/verify-archive.py
python3 pc-port/notes/original-resource-holder-20260903/verify-bck-jmap.py
```
