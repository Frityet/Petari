# Original FlipPanel owner activation

The retained `notes/demo-scene-definitions-20260807T010317Z/route-smoke/gateway-handoff-placement-report.md` has 16 FlipPanelReverse placements, the largest unsupported ordinary actor group in authored HeavensDoorGalaxy scenario 1. This is retained source evidence, not a current completed live placement run.

Imported the complete original FlipPanel and FlipPanelObserver translation unit and header byte-for-byte from the existing canonical reference. The actor preserves its original joint control, player-ground contact checks, front/back animation, bloom synchronization, group messaging, completion demo and switch/Power Star requests. The observer's original stage-specific sound selection remains original Game behavior; no host stage-specific branch was added.

Registered the exact original FlipPanel, FlipPanelObserver and FlipPanelReverse creator/archive rows in the shared factory. Its existing full archive table already supplies FlipPanelBloom and FlipPanelReverseBloom, so it needed no new resource rule. Added the original JointController include missing from the native Game/Util.hpp umbrella, retaining an unchanged actor source.

Both native translation units compile. The only Game symbols missing when comparing the new actor object against the previously built production archives are the four original group-operation helpers separately restored by root in `notes/original-group-actor-operations-20260913/`. No new tests or shared application build were run in this lane. Actual runtime remains for the next production run.

Owned native files are recorded in `source-manifest.json`; factory ownership is only the FlipPanel include and three rows. No reference recovery or reference edits were required, and the shared index was not changed.

Thirty-fourth integrated production build passed with this cohort. Twenty-fifth original run reached a pending TicoBaby archive request; it does not establish completed placement of every imported actor or working gameplay.
