# J3D renderer real-or-absent cleanup

Unsupported material paths are now omitted rather than approximated.

Removed paths:

- the one-pixel white-texture substitute for lit, textureless materials;
- CPU per-vertex substitutes for unsupported multi-pass and indirect TEV;
- precomposed multi-pass/indirect textures that ignored per-geometry runtime behavior;
- the legacy one-texture-per-pass fallback for unsupported TEV state;
- `packet_mode_fallback` reporting and the fallback-only packet modes.

Retained paths are backed by parsed model/material data and one of:

- supported GX TEV shader state;
- exact constant-material composition for unlit textureless state;
- exact supported single-texture CPU TEV composition;
- the explicit constant-backdrop load option.

Named J3D joints are also retained by the renderer and can be evaluated through the real parent hierarchy and active BCK transform tracks. Missing names return absence.
