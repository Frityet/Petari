# Retained GX viewport replay into uniform state

This follow-up to Aurora startup-drain checkpoint `df4e279` keeps the actual GX render-state fields consistent with the viewport/scissor commands emitted at the beginning of a new recording. Previously recording startup remapped retained logical coordinates into a new target but only wrote its command caches. The shader uniform source `g_gxState.renderViewport` and corresponding scissor state could retain the old target's dimensions if Game did not issue another setter.

`begin_recording` now seeds its mapped caches, calls the existing `gx::set_render_viewport` and `gx::set_render_scissor`, and emits the existing two initial commands. Matching caches prevent the setter calls from emitting duplicate commands. The ordinary render viewport setter updates the actual uniform source and marks `DirtyUniform` when mapped dimensions differ.

The existing queued-state regression now asserts exactly two initial commands, the full mapped viewport and scissor in both commands and GX state, and the uniform dirty bit after a target change from 640x480 to 1280x960. It clears that bit immediately before replay so the assertion cannot pass solely because an earlier queued logical setter marked it.

Validation command:

```sh
cmake --build build/aurora-upstream-merge-tests --target gfx_recording_tests -j 2
build/aurora-upstream-merge-tests/tests/gfx_recording_tests
```

This is a general recording-state fix. It adds no Game setter, stage condition, discarded FIFO command, or additional frame dependency.

Result: all 10 recording/target-layout cases pass. The parent coordinates the shared native rebuild and next GPU smoke before publishing this follow-up.
