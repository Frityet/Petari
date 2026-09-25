# Remaining scene cleanup after draw ownership

Eight scene files remain, totaling512 lines at the start of this follow-up audit: GameSceneBinding, OriginalSceneSupport, SceneInitializationState and SceneLifetimeBinding pairs. These are actual lifecycle responsibilities with no remaining factory/placement/collision service tables.

GameSceneBinding caches GameScene's pause control, opening camera, pause checker and window callback, plus its unclaimed NameObj registration marker. Move cleanup onto actual GameScene/GameScenePauseControl/GamePauseSequence owners before deleting the binding; preserve child teardown before SceneObjHolder and executor teardown. A global current GameScene binding should not survive that migration.

SceneLifetimeBinding is a global linked list used only to order the current native services around derived/base Scene destruction. Replace its callbacks with explicit actual Scene/GameScene destruction phases as those services disappear. OriginalSceneSupport currently retains the scheduler, heap domain, initialization binding and scene-object owner borrow. Split those fields onto actual Scene and GameSystemSceneController lifetime owners; do not move the entire support class into another directory.

SceneInitializationState already reads/writes GameSystemSceneController::mSceneInitializeState. Its scope/restore behavior belongs on that controller. Callers should use that actual API and retain original phase ordering. Any active-scope retirement protections must stay with the owner; no replacement global initialization stack.

Read-only follow-up notes. No additional production changes or validation are included here.
