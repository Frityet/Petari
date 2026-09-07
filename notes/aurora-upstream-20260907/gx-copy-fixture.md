# GX display-copy test lifecycle correction

The coordinated root validation initially failed
`smg-pc-gx-copy-fifo-order-tests` at its exact 640x456 display texture check
on the M5 Max Retina display (initial host framebuffer 1280x960).

The fixture called `AuroraSetViewportPolicy(AURORA_VIEWPORT_NATIVE)` and
immediately rendered without pumping window events. Aurora queues a viewport
policy change in `set_viewport_policy`; `aurora_update` invokes `gx::update`
to apply it. The normal application loop reaches this through
`AuroraWindow::poll_events`. Without that lifecycle step, the fixture retained
the default FIT policy and requested a display copy scaled to the host
framebuffer. `gx.cpp`, `aurora.cpp`, `GXFrameBuffer.cpp`, and `GXAurora.cpp`
are all unchanged between the pre-merge and merged Aurora revisions.

The fixture now polls its window once after selecting native viewport policy,
asserting the window remains open. The exact display dimensions, retail clear,
vertical filter colors, all quadrant pixel counts, FIFO blend values, and copy
ownership/destruction assertions remain unchanged. No rendering API behavior
was changed and no Game source was touched for this correction.

After rebuilding the specific xmake target with the existing LLVM 23 config,
the complete test passes on Metal. See `gx-copy-fifo-build.log` and
`gx-copy-fifo-run.log`. This is an inherited fixture initialization issue,
identified from unchanged-source evidence and the lifecycle correction, not
a separately executed pre-merge binary comparison.
