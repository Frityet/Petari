# Final KPAD API audit

Read-only production review before Aurora publication. No source edits, builds, Xmake runs, or commits were performed during this audit. All five implementation/test source hashes still match `source-manifest.json`.

## Acceleration speed: vector delta magnitude

The SDK definition is `acc_speed = ||current_acc - previous_acc||`, independently for the core controller and FreeStyle extension. It is not `abs(||current_acc|| - ||previous_acc||)`, and it is not divided by elapsed time.

Authoritative source: `decomp/src/RVL_SDK/kpad/KPAD.c` lines397–407 first preserve the old core vector, process the three new components, subtract each new component from its old component, and square/sum/sqrt the resulting vector. Lines423–434 repeat that calculation for `ex_status.fs.acc`. The subtraction order is old-minus-new; a vector magnitude is invariant under the simultaneous sign reversal used by the native implementation.

The available retail assembly independently confirms the source:

- `notes/gateway-audit-20260907/restoration/retail/asm/RVL_SDK/kpad/KPAD.s`, addresses804506B0–80450708: load the three saved old core components, subtract new components at offsets0x0C/0x10/0x14, square each difference, sum, call sqrt, and store `acc_speed` at0x1C.
- The same file,804508D8–8045092C: identical per-component subtraction for FreeStyle offsets0x68/0x6C/0x70, then store the magnitude at0x78.

Current `aurora/lib/wpad.cpp` lines370–384 use that same vector-delta meaning. Existing regression `CoreAndFreestyleAccelerationsHaveIndependentMagnitudesAndFrameDeltas` deliberately changes core `(0,0,-2)` to `(0,0,+2)`: both magnitudes remain2 while speed must be4. A difference-of-magnitudes implementation would fail that case. The FreeStyle change `(-3,4,0)` to `(0,8,0)` independently expects magnitude8 and speed5, rather than magnitude difference3.

A targeted search in `dolphin/Source/Core` did not find a separate `acc_speed` provider. The local SDK source plus matching retail instructions are the direct evidence for this field; this review does not infer its contract from emulator UI labels.

## Records and stale extension fields

`KPADStatus` and `KPADEXStatus` were checked again against `decomp/libs/RVL_SDK/include/revolution/kpad.h` lines37–78. The final native declarations and fixture assertions cover every field offset, 0x84/0x24 sizes, alignment4, the signed byte error and DPD fields, unsigned device/format bytes, and both union alternatives. Classic is a declared record only; the native capability enum exposes only Core and FreeStyle.

Each nonempty-buffer KPADRead begins with `sampling_bufs[0] = KPADStatus{}` before deciding whether a connected sample exists. This value-initializes the FreeStyle record fields, so reusing an output buffer for a later Core/disconnected read cannot retain its previous FreeStyle stick, acceleration, magnitude or speed fields. Core publication leaves those initialized fields untouched and masks C/Z from held/trigger/release/repeat outputs. FreeStyle explicitly fills those fields from current native state. No Classic sample is advertised or interpreted. This is field-level validity; this audit makes no extra promise about C++ padding/object-representation bytes.

Disconnect clears the native stick/acceleration samples and both previous acceleration vectors. Configuration is retained, so a later explicit reconnect produces the selected device with cleared measurements until native setters publish current input. Device removal alone suppresses the extension in the SDK record without mutating cached native physical inputs. Capability changes and probes do not consume the input sample.

## Publication and accuracy limits

Aurora intentionally keeps its existing one non-consuming native frame snapshot: begin_frame saves prior acceleration vectors, setters supply already-processed KPAD coordinates/units, and every read during that frame reports the same delta. A later unchanged frame reports zero delta. A valid read returns one sample even if the caller offers a larger array; following records remain untouched. Empty/invalid buffers return zero, and invalid/disconnected channels return zero samples with the existing cleared first-record failure state.

The original SDK instead drains its queued samples (`KPAD.c`1191–1203) and computes deltas between successive processed sensor samples. Therefore the native frame snapshot does not establish original 200 Hz sample timing, queue consumption, raw accelerometer calibration/clamping, configurable filtering, or exact initialization history. Native std::hypot computes the same norm; it does not claim identical floating-point intermediate rounding to the retail scalar multiply/add/sqrt sequence. These are already-declared limits of this prerequisite and are not newly introduced Game workarounds.

No material correctness issue was found within the scoped records, selected-capability publication, and frame-snapshot contract. Full original WPad/WPadHolder activation and original sensor processing remain separate work.
