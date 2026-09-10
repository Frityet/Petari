# Original WPad owner and acceleration recovery

The native owner currently constructs actual `WPad` records but drives only button, pointer and stick processing. This cohort restores the complete original acceleration child so the original `WPad::update` can replace that partial scheduling. It does not activate `WPadHolder`, claim complete speaker support, or change the platform sampling rate.

## Provider and owner audit

The original update order in `decomp/src/Game/System/WPad.cpp` is button, core acceleration, pointer, core swing, both rumble objects, sub acceleration, sub swing, stick, leave watcher, and information checker. `_34` is false during that sequence and true afterward; channel `-1` returns before it.

At the start of this audit, `src/compat/OriginalWPadRecords.cpp` owned copied WPad/acceleration/button/rumble/watcher/information constructors and several accessors; `OriginalWPadRumblePause.cpp` owned rumble selection and pause methods. `WPadOwnership::update_samples` bypassed all processing except button, pointer and stick. Full original `WPadPointer.cpp` and the previously recovered `WPadStick.cpp` were already active. `WPadHVSwing.cpp` contained only its constructor.

Whole original `WPad.cpp` additionally needs the missing swing methods, LeaveWatcher update, complete rumble lookup/update support, and the SDK connection, information and speaker callbacks. `SpkSystem`'s static connect/disconnect/extension methods forward to `SpkSpeakerCtrl`; the latter uses actual global `SpeakerInfo` records and SDK speaker-control callbacks. Its `isEnable` explicitly requires a non-null mixing buffer. This callback path must not be replaced by a fabricated GameSystem or an enabled speaker without its owner. Speaker production work is outside this cohort.

`WPadHolder` owns four `WPadReadDataInfo` buffers and two actual WPad objects. It initializes KPAD and allocator callbacks, registers connection/extension callbacks for all four channels, loads sensor-bar position, reads all four channels, projects the first two read records, and updates/disconnects according to its original mode. Its class methods can run against a real native-owned holder. Its anonymous global accessor instead follows `SingletonHolder<GameSystem> -> GameSystemObjHolder -> WPadHolder`; GameSystem ownership remains a separate prerequisite. The parent owns that native publication seam and the general SDK implementation. No shadow GameSystem is introduced here.

Ownership for this recovery is limited to decomp/native `WPadAcceleration.cpp/.hpp` and `tests/OriginalWPadAccelerationTests.cpp`. The parent owns `WPadOwnership` and duplicate-provider removal; the other input agent owns `GamePadUtil` and subsequent swing recovery.

## Retail recovery

Guide: `decomp/AGENT_DECOMP_GUIDE.md`. Retail input is the existing disassembly/object at `notes/gateway-audit-20260907/restoration/retail/{asm,obj}/Game/System/WPadAcceleration.*`, covering `803ABD30–803AC4AC`.

The original class has nine functions. Three were missing; the existing source also had confirmed incorrect operands and loop conditions:

| Original address | Recovered behavior |
| --- | --- |
| `803ABDF8` | Current getter rejects nonpositive count and reads history at the current ring index. |
| `803ABE60–803ABE88` | History count comparisons are signed; the past index is current ring index minus requested age, wrapped by 128. |
| `803ABEBC` / `806C1494` | Balance uses the exact single-precision `0.018f` threshold. |
| `803ABEF4–803ABF80` | Count less than or equal to zero duplicates the last sample, or inserts zero before any sample exists. The old source did this for positive counts. |
| `803ABFB0–803ABFFC` | Core axes are `(-acc.x, acc.z, -acc.y)`. Nunchuk axes use the separate extension acceleration at offsets `0x68/0x70/0x6C` with the same transform. An unsupported extension skips the sample. |
| `803AC080–803AC0DC` | Signed loop visits all available history records; their vector sum is divided by that count. The old `j >= count` loop was inverted. |
| `803AC110–803AC304` | Rotation accumulates triangle cross products around the newest record, up to the signed minimum of `_644` and history count. Detection starts at age20 with magnitude greater than6. |
| `803AC308–803AC414` | Average sums squared adjacent differences over at most32 samples and divides by the number of samples, including the first record. |
| `803AC418–803AC4A8` | Stability uses floating-point absolute values and an inclusive `0.3f` threshold. |

The header changes only `_628` and `_644` from unsigned to signed32; offsets and sizes are unchanged. Those signed comparisons and the signed integer-to-float conversion are visible in the retail instructions.

Two surprising retail behaviors are deliberately preserved: stability observes physical `mHistory[0]`, not the current ring index; the vertical rotation's optional magnitude arbitration stores `abs(_638.z)` after testing X. The optional zero-initialized direction-arbitration boolean is retained in an anonymous namespace. The short-history rotation return also preserves prior direction flags. None of these were “fixed” by intuition.

The retail constructor does not initialize the whole history array. A no-sample update inserts the initial zero record. A sub controller that first receives a core-only valid packet skips insertion and still reaches the retail stability read of slot zero; native input initialization and fixture setup must be considered against that contract rather than modifying Game behavior. The walking demo normally publishes a Freestyle device, providing both acceleration records.

## Proof

`verify-acceleration-wii.py` independently compiles the entire reference TU using the existing GC3.0a3/SJIS toolchain and runs objdiff against the retail object. Baseline and recovered command logs, source hashes, and per-symbol scores are retained. All nine functions now exist and exceed93% similarity:

| Function | Baseline | Recovered |
| --- | ---: | ---: |
| constructor |99.60%|99.60%|
| getAcceleration | absent |100.00%|
| getPastAcceleration |62.40%|100.00%|
| isStationary |100.00%|100.00%|
| isBalanced |99.17%|99.17%|
| update |82.72%|98.25%|
| updateRotate | absent |94.90%|
| updateAccAverage | absent |99.26%|
| updateIsStable |65.68%|93.65%|

The balance percentage includes relocation matching, so its old incorrect constant and corrected constant happen to receive the same aggregate score. The exact constant bytes and branch operands were checked independently; aggregate similarity alone is not the semantic proof.

The reference source/header were copied exactly into native Game. Native whole-TU syntax and actual object compilation through the final execution-character-set wrapper both exit0. Undefined symbols are limited to `KPADSetAccParam`, `PSVECCrossProduct`, `MR::isDeviceFreeStyle`, the two WPad sample accessors and `bzero`; no new Game owner or cached input service is required by this child.

The dedicated fixture uses actual `WPadOwnership`, original allocated core/sub children, and additional original acceleration instances bound to that real WPad. It checks newest-first120-record batches,300 inputs through two128-record wraps, every retained history age, invalid-age output, core/sub axes, extension rejection, no-input duplication, mean and adjacent-difference average, both rotation planes in both directions against the independent polygon-area formula, threshold boundaries, the retail slot-zero stability behavior, and repeated input heap retirement. Fixture syntax exits0. Its linked runtime proof is pending the parent's whole input provider activation; syntax is not claimed as a runtime pass.

No root Xmake command or commit was performed by this recovery task. `source-manifest.json`, `function-scores.json`, `native-compile.json`, `native-undefined.txt`, and `acceleration-reference.patch` provide the compact checkpoint evidence; large local objdiff/object files are regeneration artifacts.

## Original speaker connection prerequisite

The parent subsequently authorized the bounded speaker connection closure in `src/compat/OriginalWPadSpeaker.cpp`. It contains twelve byte-for-byte original function bodies: the three `SpkSystem` connection/disconnection/extension forwards and nine `SpkSpeakerCtrl` methods for connection, disconnection, on/play/off commands, both callbacks, reconnect initialization, and extension processing. The original `SpeakerInfo[4]` state is the sole global object defined by this provider. Seven required Game/Speaker headers are copied unchanged; no Game method, mixer object, speaker alarm, sound handle, or fake GameSystem is added.

This extraction is bounded at the existing original static API, which never dereferences the optional `SpkSystem` or mixing-buffer owner. It allows whole original `WPad.cpp` to retain its original connection callbacks. The full speaker TUs still contain unrelated mixer/stream and resource dependencies; those remain unavailable until their actual owners are integrated. The extracted methods must be removed as one cohort when their original complete TUs are activated, to retain a single provider for each symbol and the same original global state.

Fresh whole-reference-TU Wii compilation of both `SpkSystem.cpp` and `SpkSpeakerCtrl.cpp` succeeds. All twelve selected methods are100% retail objdiff matches, including the genuinely empty original `extensionProcess`. `speaker-wii-proof.json` lists every exact command and score. `speaker-extraction-manifest.json` records each original function-body hash and every copied header hash.

Native object compilation against the parent's typed SDK declarations succeeds. Its only functional external dependencies are `OSDisableInterrupts`, `OSRestoreInterrupts`, and `WPADControlSpeaker`; standard C++ exception unwinding adds the normal compiler ABI references. There are no hidden sound/mixer constructor references. See `speaker-native-compile.json` and `speaker-native-undefined.txt`.

The parent implements actual SDK capability reporting separately: a connected controller without a speaker rejects an enable/play request, rather than reporting success. The original SDK invokes the callback synchronously for an immediately rejected command (`WPAD.c:1900–1997`), so the platform must preserve that ordering and the same returned error. `WPAD_ERR_INVALID` is the declared capability result; a missing controller returns `WPAD_ERR_NO_CONTROLLER`. This task does not claim successful speaker output. No encoder implementation is needed by the extracted connection closure; `WENCInfo` is only the original state layout used by `SpeakerInfo` and the successful-play callback.
