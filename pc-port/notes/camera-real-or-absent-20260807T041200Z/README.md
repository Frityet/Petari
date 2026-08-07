# Camera state: real pose or absent

## Outcome

The host no longer invents a default scene camera or a player-relative follow
camera when stage camera resolution fails.

- `RuntimeContext::draw_3d_normal()` omits the 3D pass when neither the camera
  system nor the scene supplies a pose.
- Debug free-camera mode can begin only from an existing real scene pose.
- `StageHostScene` leaves the camera absent when the requested stage camera is
  unavailable.
- `StagePlayerRuntime` no longer manufactures a follow pose and performs no
  camera-relative movement or jump input until a real effective camera exists.

The existing follow-camera math remains available only to preserve and track a
pose that was actually supplied by the stage camera system. No file under
`pc-port/src/Game` was changed for this cleanup.

See `verification.log` for tests and build results.
