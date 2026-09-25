# Scene integration for actual owners

The scene binding no longer creates EffectSystemOwnership, CollisionDirectorOwnership or GlobalGravityOwnership. The actual CollisionDirector and EffectSystem constructors are used; EffectSystem entry selects the scene heap and process ParticleResourceHolder. EffectSystem retires keeper graphs before scene publication clears, then actual destructors unwind the dependency graph. CollisionParts keep category services alive independently. Callback and object rollback retain the existing general NameObj transaction, without collision-specific prepare/reclaim hooks.

The old standalone effect-ownership fixture and target were retired because they required a private fallback catalog and budgeted sidecar heap that no longer exist. No replacement fixture framework or broad tests were added. The initially dirty tests/xmake.lua changes are preserved; only this target removal belongs to the batch.
