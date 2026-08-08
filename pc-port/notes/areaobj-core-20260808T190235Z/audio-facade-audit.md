# Read-only audio facade audit

This audit was requested while exact `RestartCube` linkage was landing. No restart/audio-owned implementation file was edited by the AreaObj-core wave.

## Retail evidence checked

- RMGK02 `AudBgm::setVolumeController` at `0x8002FBC4` stores its pointer at object offset `0x4`.
- RMGK02 `AudBgm::resetAuxVolume` dereferences that same field as an `AudBgmVolumeController*`.
- RMGK02 base construction initializes rhythm index `-1` and rhythm BGM pointer `nullptr`.
- RMGK02 `AudSingleBgm` construction calls `init`; its `init` stores `_18 = -1`, and `start` stores the requested sound ID there.
- RMGK02 `AudMultiBgm` construction initializes `_1F8 = -1` and calls `init`.

## Final observed host boundary

- The facade constructs real `AudBgmMgr`, `AudBgmKeeper`, concrete BGM, fader, track-controller, and `JAISoundHandle` objects. It does not use raw byte storage or reinterpret-cast fake objects.
- `AudBgm::_4` was corrected in the root decomp header to `AudBgmVolumeController*` and mirrored byte-exactly into PC Game source.
- The retail constructor state above is now represented by the compatibility implementation.
- Raw current/next/last stage-BGM IDs and the cube-change-invalid flag are explicit runtime-service state.
- Unsupported object-access and stateful manager paths reviewed for RestartCube throw instead of silently reporting success; exact no-op methods and explicit negative/absent results remain distinguishable.

The exact RestartCube factory remains intentionally disabled; this audit only established honest constructor/linkage/state boundaries for later integration.
