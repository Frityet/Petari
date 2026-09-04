# Original integral vector conversion

The original particle callback converts degree rotations to TVec3<s16>. Gekko truncates each float to a signed word with fctiwz and then keeps the low halfword. A native floating-to-short cast has undefined behavior outside the short range; ordinary angles such as 180 and 360 reach that range.

The general JGeometry component conversion now uses Aurora's existing PowerPC integer semantics for floating inputs to s16, u16 and s32 vector constructors, setters and TVec2 arithmetic conversions. Other conversions retain their existing casts. This changes JSystem compatibility, with no Game algorithm change. Aurora's four conversion helpers are now constexpr; the NaN test uses value != value so constant evaluation does not depend on the library's constexpr cmath support. Runtime integer results are unchanged.

oracle.cpp compiled with GC3.0a3 verifies the actual original TVec3<s16>, TVec2<u16> and TVec3<s32> instantiations use fctiwz followed by sth or stw. oracle.asm and oracle-command.json preserve the complete instructions and command. The underlying Aurora instruction behavior predates this change; emulated FPSCR/CR status is still outside this numeric API.

smg-pc-original-vector-integer-conversion-tests passes 209575 cases: full two-turn degree sweeps, deterministic floating bit patterns, signed boundaries, finite overflow, infinities, NaNs, constructors, setters, integer-vector arithmetic and compile-time wrapping. An independent arithmetic reference uses truncation and modulo for the verified instruction sequence. The same test passes ASan, UBSan and float-cast-overflow instrumentation with no diagnostics. Logs and sanitizer command are stored here.
