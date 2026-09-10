# Original Gateway sequence integration — 2026-09-10

Previous goal turn: **progress**. Published original full WPad/GamePadUtil processing, recovered controller functions, native SDK lifecycle/motion and BRLAN behavior, merged latest upstream JKernel, and refreshed the verified movement demo. Root451549161, Auroraa941314, decompec3406dfc. The exact packaged movement binary019f7be6e2f99ef7db96cb573f9334eba41f63739271f327d7266a68eb8f0f30 passed13/13 checks at59.939FPS.

This continuation pursues the actual Gateway opening/bunny sequence. The first pass audited ordinary StageHost/GameScene, NPC/demo ownership, and the required rabbit reference source. Rabbit imports were paused when the user redirected work to removing NPCActorSource.inl and permitting a small direct compiler fix. No rabbit or stage production changes were made by these audits. Their supporting evidence and unresolved shared-system dependencies remain in the subdirectories for the next coherent restoration.

The implemented checkpoint is documented in `../original-npc-direct-source-20260910/`: direct original NPCActor, restored original JointController, deletion of the obsolete wrapper/rejections, native owner tests and a broader compatibility reduction review. The four pre-existing user changes remain excluded from the checkpoint. One root Xmake lane was used; subagents used isolated compile/proof only.
