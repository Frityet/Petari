# Original spherical shadow owner

Recovered the missing ShadowVolumeSphere projection/view transform and base ShadowVolumeDrawer drop-position calculation in decomp, then copied the complete Drawer/Model/Sphere classes into the native source tree. Corrected inherited virtual declarations and supplied ordinary empty virtual destructors in the reference headers.

ShadowControllerOwnership now owns a real sphere drawer for spherical definitions, attaches it to the real controller, and registers its original draw category. The original setter updates the drawer radius directly. Original model lookup and shape rendering helpers are enabled; there is no copied live radius or alternate generated geometry.

The two recovered shadow sources and the two small LayoutUtil emitter setters compile with the original Wii compiler. Native shadow sources compiled in the shared seventh startup build. The whole startup executable still has ten unresolved links at that checkpoint; shadow pixels and gameplay have not yet been run.

The recovered LayoutUtil setters at retail 0x803D9020 and 0x803D9058 forward to the real MultiEmitter with index -1. They were recovered in the reference before native import.

The production application linked in `original-app-second-build.json`. Its first real-disc run entered original GameSystem::init, then hit Aurora FIFO rebinding before any game frame. This was a full working-tree run, not a claim of gameplay or an isolated published-commit run.
