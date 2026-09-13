# Original unique-planet creator activation

Replaced the parsed planet catalog's name-only unique list with all 39 original name-to-actor-class mappings from decomp/src/Game/Map/PlanetMapCreator.cpp, sUniquePlanetCreateFuncTable. Catalog rows retain the exact original class identity independently of native link availability. This keeps data-only catalog consumers free of the actor dependency graph.

The central NameObj factory now registers available unique creators by original class and uses one resolution function for actual construction, availability reports and visual classification. SimpleMapObj is enabled after the complete original MapObjActor, initialization-info, rail/rotation/seesaw and guide dependencies were restored. Its original unique-table row is HeavensDoorInsidePlanet. Other unique rows retain their real class identities and remain unavailable; they cannot fall through to ordinary PlanetMap. Existing force-low restrictions are unchanged and take precedence.

Registered planet lookup now precedes the ordinary creator table, matching original NameObjFactory::getCreator. All other archive collection and scene ownership remain with their existing common owners. No specialized actor behavior or fallback geometry was introduced.

Both changed native TUs compile under the production compiler configuration. No component tests or full build were run in this lane; Peirce owns the next production build and reached stage-loading frontier. Existing PlanetMapCatalogTests required only two references to the renamed neutral UniqueCreator enum; no new tests were added or run.

Files and hashes: source-manifest.json. Original mappings: original-creator-map.txt. Earlier rotator recovery remains independently frozen in notes/original-map-parts-rotators-20260913.
