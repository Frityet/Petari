# WPADProbe signed SDK result — 2026-09-10

Aurora's probe incorrectly returned BOOL-style connected=1/disconnected=0. Original consumers interpret zero as success and negative values as SDK errors. The declaration and implementation now use `s32`, returning `WPAD_ERR_NONE` (0) for the actual connected native channel and `WPAD_ERR_NO_CONTROLLER` (-1) otherwise. The optional device output is `WPAD_DEV_CORE` (0) for the currently implemented core device and `WPAD_DEV_NOT_FOUND` (253) for absence. Added the missing `WPAD_ERR_TRANSFER` (-3) and not-found constants; BUSY (-2) already existed.

The synchronous native input service currently has no transport/handshake state, so this change does not invent busy or transfer failures. It does not add an extension device, expand KPAD records, alter WPadStick, or change keyboard/mouse publication. Out-of-range native channels retain safe rejection and now return the same no-controller result instead of falsely reporting success.

## Reference evidence

- `decomp/libs/RVL_SDK/include/revolution/wpad.h:107–110,419` declares the signed status values and `s32 WPADProbe(s32, u32*)`.
- Retail `WPADProbe` at `0x804D9F9C` copies the channel device type when the output pointer is non-null, loads its signed status at `0x804D9FD8`, preserves no-controller -1, maps not-found device type253 to -1, and changes an incomplete handshake to BUSY -2 at `0x804DA004`. The bounded assembly is copied to `retail-WPADProbe.s`.
- Retail `__ClearControlBlock` at `0x804D8810–0x804D884C` initializes status -1 and device type253. Source: `notes/gateway-audit-20260907/restoration/retail/asm/RVL_SDK/wpad/WPAD.s`.
- Original `decomp/src/Game/System/WPadHolder.cpp:164–181` accepts NONE/BUSY/TRANSFER in its home-menu record lookup and rejects no-controller. These are the existing original consumer semantics, rather than a new boolean conversion.

## Validation

Built the existing Aurora `wpad_tests` sources directly with Homebrew LLVM23, SDK26.5, the existing GoogleTest static libraries, and the fixture's existing PAD motor stubs. No root Xmake lane or renderer was used. `commands.txt` records every exact compile/link and runtime command; `results.json` records exits and binary hashes.

- Previous implementation plus the five new regressions: compile/link0, runtime1; all five probe tests fail and all three pre-existing stick tests pass.
- Corrected implementation: compile/link0, runtime0; **8/8 tests pass**, zero skips.
- Coverage includes all four channels, optional null device output, disconnect/native reconnection, preservation of existing button/pointer/stick samples, and invalid native channels.
- Root `tests/AuroraNativeTests.cpp` now expects the actual signed errors and device types. Its full target remains for root to build/run; this standalone result does not claim that integration run.
- Scoped diff whitespace checks pass. No remaining source/test declarations use BOOL for WPADProbe or compare its result with TRUE/FALSE.

Only three Aurora files and the affected root fixture were changed. No Game/decomp changes, root build, or commits. Exact frozen sources are in `source-manifest.json`; the fixed standalone binary SHA-256 is `b77839146894fce47e6ea9b431ec298b123b5f11292cdd8b1617a1e5cd416023`.
