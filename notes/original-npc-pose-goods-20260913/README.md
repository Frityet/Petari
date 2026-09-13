# Original NPC goods, pose and Tico floating helpers

Recovered six missing NPCUtil methods in reference first, copied the complete
reference TU unchanged to native, and removed their duplicate bodies plus the
shared goods constructor from NPCActorRuntimeCompat.

- `isNPCItemFileExist` formats the original `/ObjectData/%s.arc` and calls the
  normal file service. The old RuntimeContext-only lookup silently suppressed
  NPC goods under the actual original process.
- Both goods constructors require the real authored joint, use the original
  fixed/direct PartsModel constructors, appear the model, and initialize lighting
  when its actual model uses lights. The former helper omitted appearance and
  lighting and used a different attachment route.
- `initDefaultPosAndQuat` restores original placement/quaternion/setInitPose
  sequencing, without the compatibility implementation's extra `_CC` write.
- `calcFloatOffset`, used by Tico, restores the original actor-position minus
  player-position direction. The previous compatibility body reversed that
  direction and therefore applied its height test on the wrong side. The
  original talk gates, 0.5 decay, 200 distance, and capped increase remain exact.
- `calcAndSetFloatBaseMtx` uses the original quaternion Y direction and temporary
  position adjustment around the base NPC matrix calculation.

Owned production paths: `decomp/src/Game/Util/NPCUtil.cpp`,
`src/Game/Util/NPCUtil.cpp`, `src/compat/NPCActorRuntimeCompat.cpp`.
No build-list change, new dependency provider, test, or full application run.
Existing original FileUtil, PartsModel constructors, Joint/Model/LiveActor
utilities and player/talk owners provide the lower calls.

Original compiler passes; retail object comparisons: file existence and initial
pose 100%; ordinary goods 99.81%; indirect goods 99.82%; float offset 99.39%; float
matrix 99.81%. `object-diff.json` retains the comparison against retail NPCUtil.
Both modified native TUs compile; logs are alongside this note. Actual NPC
behavior still awaits the parent application's stage run.

## Follow-up: original time-keep demo fade duration

After publishing the six-method reference as `bce4e6089`, recovered the two
remaining `timeKeepDemoFadeIn/Out` wrappers in NPCUtil. Retail passes `-1` to
`openWipeFade` / `closeWipeFade`, allowing original WipeFade to choose its
30-frame default. The old DemoCompat implementation forced 60 frames, doubling
DemoRabbit and Rosetta's original transition duration. Both wrappers match retail
100%, compile in reference and native, and their duplicate DemoCompat bodies
are removed. DemoCompat's unrelated diagnostic helpers remain intact.

Exact follow-up paths: `decomp/src/Game/Util/NPCUtil.cpp`, native mirror
`src/Game/Util/NPCUtil.cpp`, and `src/compat/DemoCompat.cpp`. No extra owner,
Game-specific timing substitute, build change, or test was added. The
`fade-*` comparison/compile logs retain this delta's evidence.
