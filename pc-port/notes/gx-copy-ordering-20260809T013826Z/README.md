# Generalized GX copy ordering

Aurora commit `4544bbb68f46ace97d4db525c1ee3af1e0f93d05`
closes a renderer-wide FIFO ordering defect: `GXCopyTex` and `GXCopyDisp`
now drain queued GX work before resolving the source framebuffer, and a copied
pass waits for cold render pipelines instead of silently dropping its first
draw. Ordinary presentation remains asynchronous.

The parent integration adds the registered
`smg-pc-gx-copy-fifo-order-tests` target and makes the existing
`smg-pc-render` dependency on Aurora matrix helpers explicit. No `Game/` file
was changed. The disconnected RFL character renderer and the separate
`GXSetZScaleOffset` work are deliberately not part of this closure.

The real Vulkan/Xvfb proof executes a queued colored draw, copies it with
`GXCopyTex`, samples that copy into the display pass, and then proves that two
alpha-blended draws reach `GXCopyDisp` exactly once and in FIFO order.
Five independent cold-cache runs were byte-stat deterministic.

See `verification.log` for commands and exact values.
