# Remaining ownership closures

Complete removal of src/compat remains active (168 files). Avoid moving the remaining services wholesale into a differently named directory. Restore original owners and place genuinely platform-wide SDK behavior in Aurora or canonical SDK source.

The earlier FileUtil/resource audit identified the next substantial closure. FileLoaderThread currently calls MR::createAndAddArchive, which publishes only ArchiveMountService; actual FileLoader::mArchiveHolder stays empty. Donor ResourceHolderManager is complete and restores the original 512-entry basename-hash registry, heap-family matching and main-thread dispatch currently bypassed by ResourceHolderService. Restoring FileUtil alone would break native resource metadata and leases.

Required resource work:

- Make original ArchiveHolder entries the actual mount owners. JKRMemArchive already parses fixed RARC, but bounded JMap/JPC registrations currently live in MountedArchive. Generic bounded resource metadata belongs in JKRArchive; typed Game/JPA boundaries can resolve it. A coherent intermediate implementation can instead place the registration handles in actual Game ArchiveHolderArchiveEntry native fields. Release registrations without reading buffers: FileLoader retires FileEntry buffers before archive entries.
- Give ResourceHolder actual-owned native J3D model/animation/BAS/BTI/CANM backing and a native destructor, with typed registration before original initializeArc. Preserve .bti/.canm cache replacement and restoration. LayoutHolder must own its native resource tables and texture lifetime.
- Migrate ModelManager, MarioAnimator, collision and LayoutRuntime borrowers to actual holder lifetime tokens. Preserve all-affected-holder preflight before heap unload. Do not discard leases merely to simplify registry removal.
- Retire actor/layout borrowers before actual ResourceHolderManager, then FileLoader, then heaps. Coordinate the initially dirty CollisionPartsCompat, RuntimeContext and placement fixture files using scoped snapshots/staging.

The standalone RuntimeContext/MessageHolderOwnership bootstrap is already invalid without original GameSystem/FileLoader. Several legacy fixtures still exercise it and fail before their intended checks. Its eventual removal should migrate those fixtures to actual process resources and consolidate actual message destruction, scene-message binding and debug lookup into their real owners. Do not reinstall a substitute language publisher or create fake GameSystem fields to keep that bootstrap alive.

The broader area and camera fixture gaps are in validation.json and their debugger logs. AreaObjCore's strict current-donor copy gate is preexisting and remains unchanged. Its runtime matrix cases pass. ActorEventCamera constructs a player without a real model; standalone camera fixtures lack the original scene/controller. These should be addressed with their owning system migrations, preserving actual-process behavior.
