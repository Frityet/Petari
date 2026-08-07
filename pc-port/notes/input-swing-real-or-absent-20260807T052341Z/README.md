# Input swing real-or-absent cleanup

`RuntimeContext` no longer treats Wii Remote button ONE as a synthetic shake.

The PC host already has a dedicated `CORE_PAD_SWING` input path (keyboard X)
which feeds Aurora's WPAD swing state. Debug button scripts continue to model
real buttons, including ONE, but pressing or scripting ONE no longer creates a
motion event that did not occur.

Verification:

- `RuntimeContext.cpp` compiled successfully in the aggregate `smg-pc` build.
- The aggregate then stopped in the concurrent Layout exact-source migration;
  no input-runtime diagnostic was emitted.
- `git diff --check` passed.
