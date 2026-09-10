# Exact KPAD record and native extension prerequisite

Status: standalone LLVM 23 compile/link exit 0; all 18 WPAD tests passed, zero failures/errors. The executable SHA-256 is `53f000213c1b1ac98d25ecc9d42bd5efed77f825d0913a8b9c49a05d1238efa2`. Full commands, source hashes, SDK reference hashes and logs are stored alongside this note. This cohort has not been committed by its authoring agent; root owns integration/publication.

## Change and reason

Aurora previously omitted the KPAD extension union, two core acceleration values and the SDK metadata bytes; its error field was 32 bits. Original `MR::isDeviceFreeStyle` and `WPadStick::update` require the real `dev_type` and `ex_status.fs.stick` fields. Existing host stick samples never reached them through KPADRead.

The public record now has the SDK field order, a 0x24-byte FreeStyle/Classic union, a 0x84-byte KPADStatus, 4-byte alignment, and exact u8/s8 metadata types. Every field offset, both union layouts, sizes, alignments and trivial-copy properties are asserted in the standalone fixture. `revolution/kpad.h` exposes the same declaration for unchanged original includes. The vector records are plain scalar records, as in the SDK; KPADStatus retains explicit value-initialized members.

The native API is `WpadService::set_device_type(s32, WpadDeviceType)` with `Core` and `Freestyle` enumerators. Selection is explicit, per channel and does not connect a channel. Core remains the default. Existing setters do not infer an attachment from movement, so selecting FreeStyle continues to report an attached extension with a neutral stick. The configured type survives disconnect; clearing the service restores Core defaults.

Connected WPADProbe and KPADRead report the same selected device. KPADRead publishes CORE_ACC_DPD or FREESTYLE_ACC_DPD metadata because the service supplies those core/pointer and extension fields. Disconnected or invalid ports return no sample and publish the existing cleared-buffer failure state with DEV_NOT_FOUND/ERR_NO_CONTROLLER; WPADProbe likewise returns -1 and NOT_FOUND. This preserves the existing native read/buffer contract.

FreeStyle samples copy the actual native stick and sub-acceleration inputs without sign flips, dead-zone changes, or Game state writes. The native acceleration setters explicitly accept already-processed KPAD coordinates/units. Core/sub magnitudes and deltas are independent; previous accelerations advance at the existing begin_frame boundary. Repeated KPADRead calls do not consume a sample or change its delta. Disconnect clears measured accelerations and their previous values so reconnection cannot expose retired sensor data. Existing pointer projection/history and FreeStyle button trigger/release/repeat publication are preserved. Core records mask the extension-only C/Z bits from hold/trig/release, and suppress the repeat flag if its only source was C/Z; the native input cache remains unchanged.

Classic is represented in the public union and SDK constants; it is not a selectable native capability because this service has no Classic input representation. No fictitious Classic inputs are published.

## Evidence and limits

`results.json` contains the exact compile and execution arrays, exit statuses, count and binary digest. `build.log`, `tests.log` and `tests.xml` contain the linked run evidence. The test links the actual Aurora `lib/wpad.cpp` plus the existing unrelated motor stubs; KPADRead/WPADProbe/sample state are real production implementations.

The ten added runtime groups cover explicit/default/per-port capability, signed/diagonal/neutral FreeStyle samples, separate accelerations and previous-frame deltas, repeated reads, preserved pointer/button transitions, extension removal, disconnection/reconnection, invalid channels, bounded writes, Core masking of C/Z at trigger/held/release transitions, and extension-only repeat suppression. The previous five probe tests and three directional-stick tests also pass.

This is the native SDK record/publication prerequisite. It preserves Aurora's one non-consuming snapshot per native input frame; it does not implement the original 200 Hz queue, raw Wii accelerometer calibration, or KPAD filtering. It does not activate whole original WPad/WPadHolder or replace existing GamePadUtil providers. Root owns explicit KBM FreeStyle selection and full-port rebuild/runtime verification. No Game sources, root production files, Xmake configuration or commits were changed here.

## Original sources

- `decomp/libs/RVL_SDK/include/revolution/kpad.h`: KPADEXStatus and KPADStatus field layout.
- `decomp/libs/RVL_SDK/include/revolution/wpad.h`: device, format and signed result constants.
- `decomp/src/RVL_SDK/kpad/KPAD.c`: read_kpad_acc magnitude and vector-delta meanings, read_kpad_stick extension layout, read_kpad_button C/Z sharing of the core button word.
- `decomp/src/Game/System/WPad.cpp`: isDeviceFreeStyle checks valid status and device type.
- `decomp/src/Game/System/WPadStick.cpp`: actual consumer of ex_status.fs.stick. Its separately recovered missing store belongs to the original-source cohort, not this SDK change.

## Independent review

Initialization agent checked the SDK declarations, byte offsets, types and constants against the reference header and found no layout mismatch or Game stick processing moved into Aurora. It identified the retained C/Z publication inconsistency on Core devices. Root requested that SDK edge be closed before the full build: the final source masks only C/Z at KPAD publication, leaves the native cache untouched, and preserves them for FreeStyle. Two focused regression groups verify transition and repeat boundaries. The final frozen source compiled/linked successfully and all 18 tests passed. The original Game WPadStick source activation remains a separate root-owned cohort.

Final pre-publication source review: see `final-api-audit.md` for retail instruction evidence that acc_speed is vector-delta magnitude, extension field reset behavior, exact layouts, and the preserved frame-snapshot limits. No source hash drift was found.
