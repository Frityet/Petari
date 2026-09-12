# Original model-changing factory startup closure

The central creator catalog now retains the original model-changing table membership and exact case-sensitive prefix/name selection: twelve rows, eleven distinct names, including the repeated TripodBossRotateParts. All eleven actor closures are currently absent native. Querying a known missing creator throws an explicit unavailable-runtime error; an unknown name returns null. No alternate actor, success stub, or ordinary-factory fallback was introduced. Original Game PlacementInfoOrdered therefore reaches the correct explicit boundary instead of silently interpreting known unsupported actors as absent.

The two MR archive methods retain their original operations. Recovered MR::getMapPartsObjectName(char*,u32,const char*,s32) in the canonical reference MapPartsUtil TU and imported its complete currently recovered source/header. The 0x1c-byte function matches all seven retail instructions modulo address relocations; its original format at 0x805E1772 is `%s%02d`. Native authored placement and lifecycle now use those shared functions instead of independent formatting/archive-read code. Availability/classification and creation now consult the same table, with existing generic name retention reused for either creator family.

PlanetMapCreatorFunction::isLoadArchiveAfterScenarioSelected delegates to the actual retained PlanetMapDataTable catalog, and fails explicitly without that owner. The query is true when any of the eight force-low scenario fields is nonempty. Fixed the same reference error in isDataForceLow and isScenarioForceLow: both retail callers compare label 0x806B1C60 (one zero byte, defined in PlanetMap.s), whereas the separate adjacent `Low` string begins at 0x806B1C61. The native catalog already implements those exact empty-string semantics.

Five native source compilations and two original compiler compilations passed (compile.json). No component tests or shared builds were run. New native Game/Util/MapPartsUtil.cpp uses the existing Game glob; the other changes are already registered sources.

Owned hunks in shared files are limited to model factory metadata/query/creation sharing in NameObjFactory; its two public declarations; planet query forwarding; model availability/classification/construction and archive ranking in AuthoredPlacementInstantiator; and shared model name/mount use plus associated includes in NameObjLifecycleService. Earlier callback retirement and other unrelated changes in those files remain intact.

## Pending raw BRLYT boundary

The prior ISBN/ARC cohort is source/link closure only. ArcResourceAccessor returns raw big-endian BRLYT data; complete SDK Layout::Build still expects host-endian structures, and GetSignatureInt currently reads a native scalar from literal signature bytes. A general retained native BRLYT conversion owner is still needed before claiming original ISBN rendering. No actor or layout-specific conversion was added here.
