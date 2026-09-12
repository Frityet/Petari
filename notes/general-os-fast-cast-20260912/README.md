# General SDK fast casts

Replaced raw native float-to-small-integer casts in Aurora OSFastCast with the original unscaled quantized-store behavior: truncation toward zero, saturation to the target range, NaN/+infinity to the positive maximum, and negative infinity to the lower bound. All four signed/unsigned 8/16-bit SDK entry points use one shared C-compatible conversion. Integer-to-float conversion remains exact. No Game-specific rounding or material-color workaround was added.

Source: [IBM Gekko User Manual, section 2.3.4.3, pages 2-57/2-58](https://doc.kodewerx.org/documents/gekko_user_manual.pdf#page=111). Original OSInitFastCast sets GQR2/3/4/5 integer formats with zero scaling in decomp/libs/RVL_SDK/include/revolution/os/OSFastCast.h. The manual is the special-input oracle: the checked-in Dolphin interpreter/native FCVT path does not reliably implement its documented NaN result.

Five focused CMake tests pass, covering fractional boundaries, both signs of quiet/signaling NaNs, infinities/denormals, every representable input in all four integer formats, the separate fctiwz/narrowing semantics, and an actual C translation unit. An additional 65,552-pattern raw-float corpus passes Clang UBSan including float-cast-overflow at -O2. The evidence archive retains commands/results and source; no binaries.

This is the SDK default-GQR fast-cast surface, not arbitrary dynamic GQR state or PowerPC exception/FPSCR emulation. The existing Aurora fctiwz helpers have different semantics and were left intact.
