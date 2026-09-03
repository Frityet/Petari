# Original J3D draw initialization and GX region metadata

Native J3DDrawInitCompat imports the remaining original J3DSys draw/reset
methods, including texture-cache region setup. Every method body is unchanged
from root src/JSystem/J3DGraphBase/J3DSys.cpp. The existing MW-only GQR setup
remains excluded on the host. Native NullTexData owns a full 32-byte IA8 tile;
the retail 16-byte BSS symbol at 0x8060D680 was followed by zero-initialized BSS,
whereas a native upload must not read past its C++ allocation. This is an SDK
storage-boundary change, with no Game algorithm edits.

Aurora now implements GXInitTexCacheRegion and GXInitTexPreLoadRegion from the
original GXTexture.c behavior. Image1/Image2 retain hardware field widths and
native integer layout. Cache initialization preserves the preload sizes and
padding; preload initialization sets the bank sizes in 32-byte units and
retains original u16 truncation. The native GXBool interface accepts Boolean
values. Invalid even-cache/NONE input is outside the original SDK precondition.
This provides real region records; it does not claim texture preload transfers,
region callbacks, or invalidation APIs are implemented.

Three Aurora CMake tests pass for every supported cache-bank combination,
address masking, untouched fields, preload sizes and transition back to cache
mode. Target: gx_texture_region_tests in build/aurora-upstream-merge-tests.

The isolated draw probe constructs a complete retained J3DModelData, a real
base SDK J3DModel and original opaque/translucent J3DDrawBuffers. It invokes
original calc/calcMaterial/viewCalc/entry/draw, sending actual material and
shape display lists to Aurora. An optional authored BCK uses actual XanimeCore
on the same model joint tree; it does not build a second render skeleton.

The first executed prototype, linked against the preceding shared libraries
plus fresh draw-init/region objects, rendered and captured mario.bdl in its
bind pose and MarioAnime.arc/run.bck at frame 15. Both exited successfully;
steady frames reported 11 Aurora draws. Captures were inspected and show
textured Mario geometry and the changed authored pose. This probe uses its
own fixed inspection camera and light, not a gameplay camera. The light is
visibly too bright; this is not a visual-parity claim. It deliberately tests
the base SDK model, not MR::newJ3DModel's actual Mario J3DModelX specialization.

The reusable probe.cpp uses the explicit process resource owner introduced
by the concurrent ResourceHolder migration. verify-native.py rebuilds it from
native fixture flags and the shared showcase link configuration. The integrated
version was rebuilt and executed successfully after the shared ResourceHolder
build, capturing the same authored run pose at frame 15. The new capture was
visually inspected; the fixed inspection light remains overbright.
Run from the repository root:

```
python3 pc-port/notes/original-j3d-draw-20260903/verify-native.py
build/original-j3d-draw-20260903/probe build/original-j3d-joint-resource-20260903/mario.bdl build/original-j3d-draw-20260903/run15.png build/original-resource-holder-20260903/MarioAnime.arc run.bck 15
```

The model/animation paths above are extracted local disc assets, not committed
fixtures. Generated executables, logs and PNG captures remain under build/.
This is verification of the general original J3D draw path; actor renderer and
MarioAnimator/ModelManager integration, original scene lighting and jumping
remain subsequent work.

The probe was rebuilt and rerun after the actual Aurora matrix target ABI fix
(`05460bf`), and its authored run capture was inspected again. Earlier builds
had omitted TARGET_PC on the matrix library; isolated C-source tests did not
expose that linked-library error. The post-fix run exits successfully with the
same visible authored pose. See the ResourceHolder migration notes for the
real-library matrix copy bounds regression.
