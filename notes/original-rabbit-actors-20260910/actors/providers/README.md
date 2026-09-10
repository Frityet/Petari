# Original NPC, talk camera and event providers

Recovered the four missing NPC helpers (`setNPCActorPos` by name, `tryStartMoveTurnAction`, `tryTalkNearPlayerAtEndAndStartTalkAction`, `tryTalkForceAndStartMoveTalkAction`) and both `startNPCTalkCamera` overloads in decomp first, following AGENT_DECOMP_GUIDE.md and retail NPCUtil.s. Native NPCUtil.cpp is then copied exactly, with genuine ActorShadowUtil/CameraUtil/TalkUtil/JMATrigonometric includes.

Named NPC positioning initializes an identity transform, resolves the authored name, applies actual NPCActor::setBaseMtx, copies translation, resets position and schedules one-time shadow calculation. Move/talk wrappers retain the original order and selected action string fields.

The full talk camera selects the actual balloon follow matrix when present, uses the actual player matrix up/position and retained follow offset, and computes the original distance/height framing before calling the original CameraDirector-backed startTalkCamera. Constants are decoded directly from retail: cosDegree67.5, height ratio0.75, offset divisor6, scale factor2 and minimum camera distance450. It retains the original zero-up fallback and floating point operation ordering, without invented camera framing or state.

One full Wii NPCUtil compilation succeeds: named NPC positioning **90.32143%**, camera wrapper **100%**, full camera **96.456%**, three action/talk functions **100%**. No score-tuning reruns were performed. Proof JSON retains the exact command and bounded symbol results. Unrecovered overloads are explicitly null in the report.

Existing complete reference functions copied without behavioral changes: EventUtil's RosettaTalkAboutTico flag and calcOpenedAstroDomeNum, GameDataFunction's seven-entry calcGrandStarNum, and SoundUtil's limitedStarPieceHitSound. The actual hasStageResultSequence declaration was added to its native header. Its current existing provider still explicitly reports unsupported stage-result ownership; no false state or substitute holder was introduced.

A single native object batch compiled all four changed providers successfully. `unresolved-direct.json` records dependencies against the previous debug archive; named positioning adds onCalcShadowOneTimeAll (parent shadow cohort). Other unresolved PowerStar/demo/water helpers are previously existing NPCUtil paths. Parent/depth own actual LiveActorGroupArray and group count wrappers, so this cohort does not duplicate their work. No tests, Xmake, commits or runtime claims were added here.
