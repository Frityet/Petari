# Original rabbit/Tico actor import — 2026-09-10

Imported the exact 11 complete CPP/header pairs from the earlier audited manifest: RunawayRabbit, RunawayRabbitCollect, RunawayTico, Tico, TicoDemoGetPower, TrickRabbitUtil, WalkerStateRunaway, WalkerStateBlowDamage, SpotMarkLight, ActorStateUtil and AstroDemoFunction. All 22 files are byte-for-byte copies of the current reference; no actor behavior edits or new decompilation were made.

The shared Util umbrella now includes the existing ActorStateUtil, BaseMatrixFollowTargetHolder and TalkUtil declarations. NERVE_DECL_NULL again only declares its singleton, matching the original macro contract and allowing the original explicit RunawayRabbit definitions. Its previous native inline storage duplicated those definitions; no preexisting native actor used that macro. Other nerve macros are unchanged.

One isolated native object batch compiled all 11 CPPs successfully. Existing retail/Wii evidence under notes/original-gateway-sequence-20260910/rabbits was reused; it was not rerun. Current debug archive symbol comparison found 25 direct missing Game providers; `unresolved-direct.json` records exact names. This is not a transitive closure or runnable-actor claim. Parent owns factory activation, source target selection, shared linking and smoke.

`source-manifest.json` captures reference/native hashes and both shared header changes. Actor imports are frozen. Follow-on original provider activation is tracked separately in providers/.
