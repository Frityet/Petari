# Round 8 pointer scene binding removal

Removed `StarPointerSceneBinding` declaration, friendship, TLS publication and implementation. Its retired StageSession owner no longer supplies an alternate pointer mode/transform lifetime. The remaining preview `StarPointerDepthOwnership` and actual director teardown are unchanged.

`OriginalStarPointerOwnerTests` now enters the actual original Game process and reads its GameSystem-owned director and actual LayoutHolder archive. Retains original cursor/guidance graphs, decoded blur dimensions, SDK group ordering/dispatch, GPU depth callbacks, stable authored message storage, screen hysteresis and process teardown checks. Retires fake nested stage binding, synthetic Base/Game transitions, fake controller bootstrap and capacity assumptions.

`OriginalImageEffectOwnershipTests` now enters the same actual-process fixture. Retains original area-manager identity, shared texture ownership, repeated actual scene-object identity, water capture resources and manual/off/auto effects. Retires the fake factory-failure injection and manually repeated fake scene lifetimes. The water archive preload follows the original loading worker/main dispatch protocol.

Before copies and SHA-256 hashes are recorded in `owned-manifest.json`. No builds or tests run, per the requested faster cleanup. Root owns test build wiring.
