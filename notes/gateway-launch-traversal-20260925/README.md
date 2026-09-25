# Original Gateway launch and later planets

Continuing at source checkpoint c3f55804a. One longer fresh-save route uses ordinary controller input, actual actors and authored progression. No actor/flag/nerve/position writes. The previous route reached the first launch-star reveal; this run targets traversal beyond it.

The original launch star reached Shoot, exposing a direct Wii graphics FIFO write in SpinDriverPathDrawer. The captured exception is EXC_BAD_ACCESS at0xcc008000 in sendPoint; the game exited SIGSEGV, after sample24300 and input log24303. This is a renderer submission fault, not an authored launch-state failure.

The correction sends the same three position floats and two texture floats through GXPosition3f32/GXTexCoord2f32. Aurora now exposes the raw Wii FIFO lvalue only on non-PC targets, turning future untranslated imports into compiler errors. Bounded scans found no other direct graphics MMIO stores in the active Game/JSystem/NW4R/render owners. There are no stage predicates, geometry changes, replacement actors or new stubs.

One integrated build passed in 55.140 seconds. No fixture suites or new tests were run. The long route predates this fix and proves the crash path, not successful post-fix flight. The next live route will exercise the corrected launch.
