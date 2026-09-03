# Aurora upstream merge: lifecycle, window, and WebGPU

Conflict-resolution scope: `aurora/lib/aurora.cpp`, `aurora/lib/window.cpp`, and
`aurora/lib/webgpu/gpu.cpp`. The fork parent was
`deab54e62d85df6188667c6935244f0186e77be1`; the upstream merge parent is
`f1189541e5d8b97fdf61946377853488d504d9df`.

## Resolutions

- `aurora.cpp` uses upstream `gfx/frame.hpp`, `gfx/recording.hpp`, and
  `gfx/resources.hpp`, including the new resource-owned statistics. Initialization
  starts the FIFO worker after graphics initialization; shutdown stops that worker
  before synchronizing and releasing renderer and WebGPU resources. Updates run
  `gx::update()` before polling events, retaining the fork's conditional RmlUi
  event forwarding after polling.
- A successful frame begin clears the fork's display-copy selection before
  `gx::fifo::begin_frame()`. Frame end drains the FIFO before ending its frame,
  retiring textures, and finishing graphics recording. The fork's selected display
  copy still supplies presentation, including its own dimensions and viewport
  policy. This ordering was coordinated with the GX/refactor owner; direct GX
  display-copy synchronization is resolved in that owner's files.
- `window.cpp` keeps the fork's configured framebuffer size and aspect policy,
  SDL pixel-size handling, and platform-layer separation. It adopts upstream
  `time_internal.hpp` pause reasons for background, window, and surface changes.
  It also retains upstream's owned deque of copied SDL drop-event strings through
  the returned event batch. Unused RmlUi and VI headers are not restored; RmlUi
  event delivery remains at the Aurora core boundary.
- `webgpu/gpu.cpp` adopts upstream's `maxImmediateSize` requirement, derived from
  the actual `gx::DrawImmediateData` type, and new Dawn cache callback signatures
  using byte spans. The GX header is used for that compile-time size; no direct GX
  function is introduced into this platform source. Unused frame, recording, and
  render-worker headers are omitted because renderer synchronization and resource
  invalidation still use the fork's explicit callbacks.

## Preserved compatibility

The WebGPU source retains actual adapter-request diagnostics, required
`DepthClipControl` and `ClipDistances` feature checks, `Depth32Float` depth storage,
ASTC/component-swizzle capability flags, and the fork's shutdown resource clearing.
Surface release, refresh, and resize still synchronize through the installed
renderer callback. Resizing invalidates retained graphics resources through the
installed invalidation callback before replacing framebuffer/depth textures.
The GX/refactor owner supplies these callbacks from the new frame/recording
modules, preserving the lifetime boundary rather than restoring `gfx/common`.

## Verification boundary

The three owned files have no remaining conflict markers and pass
`git diff --check`. Their lifecycle/API usage was checked against the merged FIFO,
frame, recording, resource, and WebGPU headers. No build or runtime test was run by
this worker while the other merge conflicts were being resolved. The parent owns
the coordinated native build and macOS/GPU regression gates. Only the three Aurora
source paths are staged by this worker; this note is left for the parent checkpoint.
