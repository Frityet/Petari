# Original fog and texture material controllers

Restored five original functions in root `src/Game/LiveActor/MaterialCtrl.cpp`
and the missing TexMtxCtrl virtual declaration in its root header. The original
compiler produces **100% matching output for all five functions and both vtables**.
The verifier resolves every actual relocation and compares all 752 instruction
bytes with the supplied RMGK01 DOL. No register or instruction normalization is
needed beyond symbol names. No native activation is claimed by this recovery.

```sh
python3 pc-port/notes/original-material-fog-texture-20260903/verify-source.py
```

FogCtrl selects materials whose actual fog type is nonzero, or all materials when
its original boolean requests that. It copies initial fog information from the
first selected material, or material zero when none is selected, and retains the
selected material pointers. Each update copies its current fog information by
value through the actual PE block's fog accessor. The explicit by-value SDK
setter is significant: the original compiler copies all 44 bytes to a temporary
before the virtual accessor call. This preserves the original evaluation order.
There is no null-material fallback; the original constructor requires a material
and each selected material requires a real fog object.

TexMtxCtrl clears eight borrowed matrix pointers. Its setter stores the requested
slot, and its update forwards each nonnull pointer to the material's actual
TexGenBlock setter, leaving null slots alone. The source adds no index clamp or
alternate matrix owner. The existing base controller still chooses either the
named material or all actual materials.

These classes complete the non-mirror material controllers required by the
original DisplayListMaker. Native import must retain the model and its material
backing through the controllers' lifetime. This recovery does not activate
ModelManager, MarioAnimator, or jumping.
