# Original planet submodel construction

Replaced PlanetMapRuntimeCompat's Water, Indirect, Bloom and LOD rejection
bodies with the existing original LiveActorUtil construction paths. Water and
Indirect share the original createSubModel routine: test the real model resource,
construct actual PartsModel, initialize it, and start its authored animations.
Bloom constructs and initializes the actual ModelObj and registers the original
demo simple cast. Planet LOD creates original middle/low children, preserves the
original distance and far-clipping setup, and starts each child's own animations.

The existing actor-owned LOD lifetime remains: an owning temporary guards
construction until generic ActorRuntimeRegistry adoption. Game-generated model
names remain in the real Game allocation lifetime. One shared generated-name
helper now serves low/middle/water/indirect names; its old duplicate and low/middle
wrappers were moved out of LodCtrlRuntimeCompat. Original fullwidth punctuation
keeps CP932 bytes in the host-charset compatibility TU.

Exact current native paths:
- src/compat/PlanetMapRuntimeCompat.cpp
- src/compat/LodCtrlRuntimeCompat.cpp

Sources are from already recovered `decomp/src/Game/Util/LiveActorUtil.cpp`;
no new reference recovery or Game behavior edits. Explicit `u32` zero resolves
native ResTable's index-versus-pointer overload, since host unsigned long is
wider than the original type. Root's preceding model-existence provider deletion
is preserved, with no RuntimeContext lookup restored. The separate original
OceanHome controller frontier remains unchanged.

Both changed TUs compile; logs are alongside this note. Root requested a native
source freeze for the next production build/run, so the PlanetMapCatalog optional
submodel exclusion, enum, factory diagnostics and existing expectations are
currently unchanged. Enabling those catalog entries is the remaining step once
that build window ends. Unique and force-low creator exclusions must remain.
No new tests, shared build, or full application run was performed by this lane.
