# Compatibility consolidation — 2026-09-12

The user requested a substantial reduction of specific providers and workarounds, allowing explicit Game changes where the original native data assumptions cannot be handled by an ordinary SDK compatibility boundary. This checkpoint restores original sources as authoritative and removes parallel state, rather than inventing scene-specific shortcuts.

## Changes

- Removed ActorBinderContactState and every registry getter/setter, integration-time copy and consumer. Original Binder owns contacts, triangle normals and fix-reaction vectors. Resetting the original Binder now immediately affects queries. Restored original ground/wall/roof, rebound, crushing-pressure and gravity-fallback queries. A native scope correction in isPressedRoofAndGround also goes into decomp; no behavior branch was added.
- Compile complete Game StringUtil and MessageUtil. Delete five extracted/rewritten providers and duplicate methods from the remaining native UTF-16 boundary. Missing MessageHolder ownership fails explicitly; a missing message inside a real holder remains absent. Reference recovery corrects basename fallback and message-tag pointer stepping, with retail comparisons in message-string-match-summary.json.
- Compile complete Game MarioSound and remove its 1,864-line compatibility copy. Existing native corrections (typed swap columns for pointer width and numeric high-byte flags for endianness) now live explicitly in the canonical native file. All previous provider text is otherwise retained exactly; the original nerve declarations use the existing native INIT_NERVE mechanism. The redundant charset admission was removed, since Game sources already use the Game execution charset.
- Compile StarPiece directly and remove stale whole-source macros. The current source already compiles without either macro.
- Replace the AudParams whole-source nullptr-to-zero macro with an actual integer initializer fix in decomp and native Game.
- Replace the FileSelectFunc whole-source wchar_t/function-renaming macros with an explicit fixed-width UTF-16 resource boundary and destination element size. RFL buffers remain 11 16-bit units. Real fellow-name copies check all five IDs and guard words.
- Reuse and extend Aurora's existing endian reader across 12 parser/SDK source files. Delete 31 local reader definitions; signed and floating-point values preserve their bits and checked reads reject size arithmetic overflow. See ../compat-resource-consolidation-20260912/.

## Necessary native Game differences

The sound table assumed four-byte pointers and big-endian union byte access. FileSelectFunc assumed a two-byte wchar_t for fixed RFL storage. String/message tag code overlays packed UTF-16 bytes onto wchar_t storage. These are architecture properties inside original code, so the native copies now express their intent directly. SDK/resource behavior and lifetime remain outside Game. No Gateway-, bunny-, or Rosalina-specific branch was introduced.

## Integration

This checkpoint also includes the previously prepared original ClipAreaHolder/filter, LayoutHolder/ResourceAccessor, NPC item table access and ResourceShare ownership expansions, plus original shadow-aware clipping queries. These replace blocked or synthetic compatibility paths with real owners and original methods. Their focused evidence lives in the gateway-* notes packages from this date.

Focused runtime checks and a 240-tick Gateway smoke pass; see the final integration receipt in ../gateway-integration-20260912/. Compilation and bounded Gateway rendering do not establish bunny chase or Rosalina progression. Original GameScene/process ownership remains a frontier.

The final count is 1,956 fewer C++ lines in src/compat than the preceding 2fa4fcbe checkpoint (48,377 to 46,421), including this checkpoint's additional original-owner implementations. Eight C++ provider files and one macro header were deleted; five original helper-provider files were added where complete Game translation units still need other owners. This is a first substantial consolidation pass; remaining-frontiers.md identifies the larger TalkDirector, math and text-processor migrations.

The contiguous UTF-16 DAT1 correction preserves original equal/interior offsets and permits valid fixed-size copies to read the actual authored words following short terminators. It also retires permanent per-message parsed strings/maps after construction. The five fellow-icon copies are compared to independently decoded original DAT1 bytes. See ../compat-resource-consolidation-20260912/UTF16.md.

The independent Power Star flag had no production writer. Its storage and setter are removed; the original MR helper once again asks GameSceneFunction::isExecStageClearDemo. An ordinary StageSession cannot answer this original scene query. Positive original GameScene nerve transitions remain outside the verified demo.
