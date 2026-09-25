# Retire scene wrapper fixture contracts

Removed SceneInitializationStateTests and SceneLifetimeBindingTests together with their two xmake targets. These fixtures asserted alternate binding publication, nested binding rules, callback registration and cancellation contracts of deleted wrappers. They do not validate the actual Scene owner path being restored this round. No replacement framework or extra cases were added.

SceneExecutionFixture now marks completion on the actual GameSystemSceneController and temporarily sets/restores its actual state around existing initAfterPlacement callbacks, including exception restoration. NameObjFactoryPlacementTests extends its existing local placement rollback record to save/restore that same controller state alongside the original placement zone. Its wall construction, collision, provenance and retirement assertions remain unchanged.

All five owned paths were snapshotted before modification; before status is recorded in owned-manifest.json. No remaining SceneInitializationBinding, SceneInitializationScope, current_scene_initialization_state or SceneLifetimeBinding references were found under tests. Production files were not edited. No tests, builds, staging or commits were run.
