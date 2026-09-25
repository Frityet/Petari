# Save owner migration review

Reviewed the ten Game save owner translation units and both new JSupport stream helpers against HEAD's deleted SaveChunkEncoding, BinaryDataChunkHolderCompat, ConfigDataMiscCompat and SysConfigFileCompat. The SHA256 snapshot is in reviewed-snapshot.json. Root owns production edits, builds, test execution and commits.

## Findings

1. **Fixed by root during review:** MISC accepted input sizes above INT_MAX even though JSUMemoryInputStream accepts s32. Its read then transferred zero bytes into an uninitialized flag. Output likewise returned zero silently. Current source rejects the signed overflow range in serialize, deserialize and validateData and initializes the local flag. Added a regression covering UINT_MAX, nine-byte Wii payload and legacy one-byte payload.
2. **Fixed by root after review:** SPN1/FLG1/VLE1 serialization initially accepted capacities above INT_MAX even though their JSU streams use signed lengths. The outer SPN1 loop could repeatedly write at position zero; the scalar serializers returned zero silently. Current SPN1, FLG1, VLE1 and GALA output entry points reject null destinations and capacities above INT_MAX before writing, preserving the deleted non-PLAY wrapper policy. Normal holder scratch buffers are small; this was an invalid-span API regression rather than an observed ordinary-save failure.

No other new correctness blocker found in this review. Runtime validation is pending root execution; no build or test executable was run by this lane.

## Bounds, byte order and preserved original behavior

- Holder offset checks avoid subtraction underflow and preflight every recognized chunk before applying it. Chunk serialization now validates its own emitted payload. Unknown chunks remain skipped; known zero-payload chunks retain prior rejection. calcBinarySize remains a pointer-only, extent-unchecked legacy method, unchanged from the deleted provider; it currently has no production callers.
- Schema validation widens record-count multiplication, checks descriptor and record extents, resolves the first matching descriptor, permits unknown and reordered fields, and rejects overlaps among recognized fields. The GALA rules match the prior walk_galaxies validator, including optional old fields, first-match duplicate handling, and zero-record schemas without required attributes.
- SYSC writes the actual original serializer header and big-endian scalar values. Reads allow valid reordering, extra fields and unaligned scalar positions. The new validator additionally rejects overlapping recognized fields before changing state.
- GALA name hashes and eight coin fields use unaligned big-endian loads/stores; optional missing fields still default to zero with original recoverable status 1. No catalog synthesis was added.
- PCE1 retains all sixteen fixed values and field widths. FLG1/VLE1 preserve their original hash dispatch and recoverable unknown-hash behavior.
- VLE1's original size/2 iteration count is preserved even though each row consumes four bytes. Exhausted reads keep the last initialized hash/value and repeat the final assignment, preserving observable behavior. No speculative correction was made.
- SPN1 validation keeps the former token grammar, sixteen-entry capacity and hundred-step bound, nested extent checks and opaque galaxy-tail handling. Original scenario mismatch/reset behavior is unchanged. Output now bounds nested records before writes.
- JSU readBig(T&) snapshots the initialized native scalar into Wii byte order, replaces only the prefix actually read, then converts back. Every current call with a reference passes an initialized value. This preserves PLAY's partial most-significant-byte loads and default life-supply low byte; writeBig similarly preserves exact short-write prefixes. PLAY's intentional null/oversize/short-prefix contract is unchanged.
- MISC legacy one-byte records remain accepted; two-to-eight-byte partial timestamps are rejected by holder preflight. Direct MISC deserialize still performs original initialization before reporting a malformed input, as before.

## Test changes

Both edited tests were clean at lane start; copies and status/hash inventory are retained here. No initially dirty tests were touched. tests.patch records this lane's changes before parent formatting.

- OriginalSaveOwnerTests: the existing OriginalStageResourceProcessFixture supplies the complete GameSystem, original FileLoader and actual scenario catalog; assertions use its original SaveDataHandleSequence current/backup UserFiles and backupCurrentUserFile. Both file payloads and PLAY runtime fields are snapshotted/restored around diagnostics, retaining actual selected identities; GALA exact descriptor header and coin-byte assertions; reordered fields, unknown descriptors, duplicate-first lookup (including ignored later out-of-range-width duplicate), unaligned records; malformed table/record extents, overlaps and missing names leave existing records unchanged; legacy missing optional fields and empty schemas retain original behavior. Existing full six-chunk snapshot and malformed later SPN1 atomicity checks remain.
- SaveConfigRealOrAbsentTests: keep the Dolphin config byte oracle and SYSC byte oracle; reject bad descriptor counts, record widths, required fields, field extents and overlaps without changing state; load valid reordered/extended schemas with unaligned signed timestamps; exercise MISC golden, legacy and invalid signed stream sizes.
- Removed source-byte-equality requirements for changed save owners. Root also removed a stale SaveDataHandleSequence.cpp equality assertion because existing native Functor.hpp/GET_NERVE_ANON changes already differed from donor. Unchanged ConfigDataHolder, ConfigDataMii and UserFile source checks and the remaining applicable header checks stay.

Run smg-pc-original-save-owner-tests with SMGPC_REAL_DISC using the real 120-frame original-process fixture and smg-pc-save-config-real-or-absent-tests from the repository root. The save-owner target also requires smg-pc-app and aurora-main, which root wires. The latter also needs the checked Dolphin oracle noted in its source. Parent owns execution. git diff --check passes for both tests.

## Final independent stream recheck

After the signed output guards were applied, reviewed direct scalar call sites again. No remaining critical issue found in this migration. readBig rejects bool via Aurora Scalar, supports only 1/2/4/8-byte arithmetic representations, and preserves signed bits through bit_cast; no pointer punning or alignment requirement is introduced. Its initialized local byte array makes short/zero transfers defined. Every current reference argument is initialized before use (PLAY initializeData, MISC explicit zero, VLE1 explicit zero), and schema/flag value-return reads initialize their temporary to zero. Raw readU16/readU32 and writeU16/writeU32 bodies remain unchanged and native-endian. Existing OriginalJSUStreamTests already independently checks native scalar bytes and partial raw/typed EOF behavior, so no redundant PLAY test expansion was needed.

Initial standalone save test failed before save assertions because its stale bootstrap lacked the now-required original FileLoader; root captured save-lldb.log. The fixture migration removes that standalone path instead of adding a production fallback. Parent owns rerunning tests; no runtime pass is asserted here. SaveConfig's two newly added 32-bit field writes were corrected to the actual aurora::endian::write_big API before final handoff.
