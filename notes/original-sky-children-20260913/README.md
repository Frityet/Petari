# Original Sky children and SummerSky creator

Restored complete reference Sky source/header and imported complete original
SpaceInner and MirrorReflectionModel source/header. Enabled the original
SummerSky -> Sky factory entry and removed its obsolete unavailable-creator
record. The existing retail archive table already supplies SummerSky and its
SpaceInner archive; no hardcoded child resource list was added.

Sky now follows original switch B callbacks to appear/disappear its actual
SpaceInner actor, hides the outer model only after the inner appearance finishes,
and restores it during disappearance. Its original Obj_arg0 == 0 mirror child
construction and animation initialization are restored too. The native
actor-specific rejection/control exceptions are removed. Actual actor lifecycle,
switch listeners, scene draw registration and model controllers remain shared
services. This is source activation, not a claim that mirror rendering or a
SummerSky stage has been exercised end-to-end.

Exact production paths:
- src/Game/Map/Sky.cpp and .hpp (complete original source restoration)
- src/Game/Map/SpaceInner.cpp and .hpp (original import)
- src/Game/LiveActor/MirrorReflectionModel.cpp and .hpp (original import)
- src/scene/nameobj/NameObjFactory.cpp (SummerSky entry and its unavailable record only)

All six actor files are byte-identical to current decomp reference. All four
native translation units compile; logs are alongside this note. No reference
recovery, shared build edits, new tests, or full application run was needed.

AstroDomeSky remains explicitly unavailable: its placement-dependent archive
selection needs the original AstroMapObjFunction path, and its complete actor
integration requires the actual SphereSelector graph. Those are outside this
Sky child batch, rather than substituted with plain Sky.

Root integration: fourteenth full production build reached final link and found missing MR::hideModelIfShown, called by restored Sky::control. Copied that complete existing original LiveActorUtil method into the established LiveActorUtilCompat provider; it uses ordinary isHiddenModel/hideModel and adds no Sky-specific behavior.
