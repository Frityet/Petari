# Original scene collision ownership

Twenty-third original real-disc run passes the mounted-model archive fix and reaches PlanetMap initialization for HeavensDoorMysteriousPlanet. Its actual resource, sensor, and SceneObj holder are valid; only the primary StageCollisionService is absent. The earlier standalone scene host created that service, while OriginalSceneSupport had only execution, resource and lifecycle bindings.

Every original Scene now owns and activates its primary collision service alongside its scheduler. It outlives original actor and SceneObj retirement, then deactivates through its normal destructor. Existing auxiliary category owners remain attached to the original CollisionDirector. No collision geometry is inferred or preloaded: original CollisionParts calls supply the exact KCL, attributes, matrices, sensors, category and placement zone.

CollisionParts now publishes the native query index after registration for category zero as well as auxiliary categories. Original actor initialization may query earlier parts before placement finishes; it cannot depend on an external demo host to publish the index later. Game methods and original placement order are unchanged.

Validation is the next production stage load. Exact prior predicate evidence: ../gateway-wakeup-demo-20260912/original-app-twenty-third-predicate.log. No new tests or component campaign.

Thirty-third production build passed. Twenty-fourth real run passed creation of the planet CollisionParts and reached a later original LOD model archive lookup. Thirty-fourth build also passed with the case-insensitive archive identity and additional original object factories. Stage loading remains in progress.
