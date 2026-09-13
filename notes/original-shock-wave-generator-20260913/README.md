# Original ShockWaveGenerator activation — 2026-09-13

Parent requested the next bounded missing creator after the reached Coin construction. The retained authored HeavensDoorGalaxy scenario-1 placement report has ten common Coin placements and nine ShockWaveGenerator placements. Original PlacementInfoOrdered sorts resource priority and then descending same-identifier count; resource readiness can affect exact ordering, so this is a source/data candidate rather than a claim that live initialization has reached this actor. Parent approved its bounded closure.

Imported complete original `Game/MapObj/ShockWaveGenerator.cpp/.hpp` unchanged. Added its exact constructor and ShockWaveGenerator archive row to the generic central NameObjFactory. No stage conditional or substituted actor. Existing original source preserves its spin/star-piece trigger, cylindrical hit test, nearby enemy message traversal, animation/effects, camera/demo behavior, stage sleep switch, and original stop durations.

Only direct missing Game helper was `MR::sendMsgToEnemyAttackShockWave`, copied from original ActorSensorUtil into the existing sensor provider. It sends the original ACTMES_TO_ENEMY_ATTACK_SHOCK_WAVE message through the shared sensor dispatch.

Three narrow native compiles pass (actor, factory, sensor provider). The actor's undefined Game-symbol comparison against the current archive exposed only that restored sensor helper; SDK vector calls and C++ RTTI remain normal linked dependencies. No new recovery, tests or shared build. Parent/Peirce own integrated application validation.

Frozen production paths:
- src/Game/MapObj/ShockWaveGenerator.cpp
- src/Game/MapObj/ShockWaveGenerator.hpp
- src/scene/nameobj/NameObjFactory.cpp (ShockWave include/creator row only)
- src/compat/GameActorSensorCompat.cpp (original message helper only)

No reference edits, build-file edits, index changes or commits.

Follow-up cleanup: removed the unused, non-retail `ShockWaveGenerator_FORCE_MATCH_SDATA2` dummy from reference and native source. Gameplay methods remain unchanged; the source pair is still identical. Original compiler passes after removal; one comparison is saved in reference-diff.json/method-matches.json. Parent does not need to restart the running build solely for this dead symbol deletion. The reference cpp is now an additional publication path.

Thirty-fourth integrated production build passed with this cohort. Twenty-fifth original run reached a pending TicoBaby archive request; it does not establish completed placement of every imported actor or working gameplay.
