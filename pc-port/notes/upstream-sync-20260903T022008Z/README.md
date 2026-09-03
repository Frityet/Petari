# Upstream merge and PC source integration

Merged `SMGCommunity/Petari` master at `c600c594461f9626c1badd9d0ad9850d0333f7dc`
into `pcp-aurora`, starting at `96e5ef0decce22e5bfd7d0ee876fb15ac80a725b`.
The common ancestor is `75a62af8d828b343df6cc36c25d9229049cb36be`; upstream
contributed 347 commits and changes to 719 files since that ancestor.

The merge favors upstream. Conflicting root files were taken from upstream,
then reconciled with already recovered local code that still needs to compile:
Mario member names and return types, duplicate animator declarations, and
`SphereSelector::registerPointingTarget`, which upstream still leaves undefined.
The existing recovered pointing implementation was retained with the new camera
matrix interface. Additional local Player sources now use upstream's field names
and the split `_6C8`/`_6CC` layout. No new decompilation was attempted.

## Files applied to pc-port

`mirror-plan.json` records 164 source/header pairs, their disposition, and SHA-256
hashes. “Exact” means byte-identical to the final merged root, which may retain
local recovered code absent from upstream.

| Disposition | Files |
| --- | ---: |
| Existing copies synchronized exactly | 107 |
| Newly added exact copies | 15 |
| Existing Player host branches with exact retail branches | 10 |
| Upstream interfaces/helpers applied to existing host implementations | 17 |
| Existing host implementations retained | 15 |

New Game translation units are `AudBgmVolumeController`, `AudParams`,
`AnimScaleController`, `CollisionCode`, `WPadRumbleData`, and `Color`.
The existing BGM-volume and animation-scale placeholders were removed so the
upstream implementations supply those functions. `Color.cpp` currently contains
only upstream's commented definition. The other additions are `AudParams.hpp`
and eight J3D declaration headers required by the updated Game headers.

Compatibility changes cover camera matrices returned by reference, real boolean
talk results, `Binder::getPlane` returning `HitInfo`, projection-controller state
stored outside upstream's opaque layout, portable vector/box/matrix operations,
frame-state masks, and Aurora display-list float writes. A small JAISeMgr header
exposes the parameter-table category types while Aurora retains audio playback.
The legacy integer `nullptr` spelling is confined to the AudParams translation
unit through a forced include.

The existing Player host branches remain necessary for the available PC state
providers. Their explicit guards now retain the exact merged retail text;
guards do not split block comments. The existing production exclusion list is
unchanged. Source mirroring does not make every Player state executable.

The retained lighting, archive, actor, camera, draw, pointer, and rail code owns
native resources or depends on host services. Replacing it wholesale with Wii
implementations would require additional porting. `candidate-review.json`
records the missing-source review, including current compiler diagnostics for
unavailable dependencies and reasons for retaining existing providers.

## Validation

Both `smg-pc` and `smg-pc-showcase` build and link on macOS arm64 with LLVM 23
in debug mode. Nine selected test targets pass:

- Game source mirror and Player source mirror (96 source branches, 63 headers).
- New upstream components: BGM fades/mute/recovery, scale impulses and vibration,
  geometry helpers, collision tables/defaults, and rumble-pattern lookup.
- NPCActor (6/6), LiveActorUtil/materials (6/6), and SphereSelector (4/4).
- Talk runtime, InformationObserver, and real Gateway Mario stand/walk/release.

The real-disc tests used the existing local RMGK01 RVZ. The Mario proof traversed
325.685 units, selected `Wait -> Run -> Wait`, and returned to stable rest.
`validation.json` contains exit codes and concise evidence. Retail Wii binary
matching and a complete interactive playthrough were not run. Upstream's
existing trailing whitespace was preserved, so `git diff --check` reports it.

Builds and tests ran with the user's pre-existing macOS and Aurora changes
present. Those changes remain uncommitted. Untouched files were verified by
SHA-256; for four overlapping build/test files, the original patch was replayed
and verified before staging only this integration. The disc image, macOS setup
files, package changes, and Aurora working tree were preserved. Nothing was
pushed. The initial patches and intermediate diagnostics remain locally under
`.git/upstream-sync-20260903T022008Z` and `.git/upstream-*.log`.

To repeat the build, run xmake from `pc-port`, with the configured LLVM tools on
PATH. Real-disc tests require `SMGPC_REAL_DISC` to name a local compatible image.
