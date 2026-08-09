# Exact player-stick input prerequisite

UTC: 2026-08-09T02:07:14Z

## Outcome

The three previously missing `GamePadUtil` functions used by the player and
camera-relative movement are reconstructed in the root RMGK02 decompilation:

- `MR::getPlayerStickX()`
- `MR::getPlayerStickY()`
- `MR::calcWorldStickDirectionXZ(float*, float*, long)`

The existing `TVec3f*` overload remains 100% matching. The two player-stick
accessors now match RMGK02 instruction-for-instruction. The scalar world-stick
function is functionally complete and 81.76923% fuzzy matching; it copies the
real inverse camera view, flattens and normalizes the camera-right and
camera-forward axes, applies the real Nunchuk stick, and returns a normalized
world X/Z direction.

The root and PC `GamePadUtil.hpp/.cpp` pairs are byte-identical. The exact PC
translation unit remains deliberately excluded from `smg-pc-game`, so no
partially linked Wii `WPad` object graph is advertised. The active compatibility
provider instead reads the real Aurora `WpadService` sub-stick and the active
real camera pose. Both paths throw through their existing strict boundaries
when no `RuntimeContext` or camera pose exists; they do not manufacture neutral
input or an identity camera.

## Decompilation evidence

Current `main/Game/Util/GamePadUtil` measures:

- fuzzy: 98.694214%
- matched code: 4044 / 4356 bytes (92.83746%)
- matched functions: 62 / 63 (98.4127%)
- data: 24 / 24 bytes (100%)

Recovered function measures:

- `getPlayerStickX`: 216 bytes, 100%
- `getPlayerStickY`: 180 bytes, 100%
- scalar `calcWorldStickDirectionXZ`: 312 bytes, 81.76923%
- vector `calcWorldStickDirectionXZ`: 20 bytes, 100%

The full RMGK02 build and canonical DOL SHA check pass.

## PC boundary

`GamePadUtilCompat.cpp` now exposes the same required surface with concrete host
state:

- player X/Y come from Aurora channel-0 Nunchuk input;
- world-stick projection uses the active camera's normalized right/forward
  basis and the selected channel's Nunchuk input;
- zero input remains a finite zero vector;
- missing runtime/camera state remains explicitly unavailable.

The permanent source-mirror test now covers both `GamePadUtil` files. The
Aurora-native suite freezes the no-runtime behavior and the real Nunchuk
threshold/edge state. Grounded Mario walking still additionally requires the
exact player creator, `MarioWalk`/`MarioWait`, and retail Binder/contact/gravity
semantics; this change does not enable a proxy player.
