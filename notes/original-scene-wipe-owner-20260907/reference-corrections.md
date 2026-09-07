# Scene wipe reference corrections

Recovered before native activation from the retail Screen objects under notes/gateway-audit-20260907/restoration/retail.

- SceneWipeHolder black fade name is フェードワイプ; white is 白フェードワイプ. Retail .data pool offsets0x2d and0x3c are distinct strings. Historical source used the white label twice; objdiff relocation matching alone had not exposed the wrong data value. Corrected constructor99.695%.
- WipeRing::getMarioCenterPos originally centers on Mario when Mario exists, except the original authored IceVolcanoGalaxy/scenario1/600-unit proximity exclusion and SuddenDeathDodoryu. Historical source reversed the stage/scenario condition and SuddenDeath predicate and left the distance branch empty. The exact original predicates/call ordering now match99.5% (216bytes). No stage-specific logic was invented in compatibility.
- WipeRing::calcMaxRadius returns MR::sqrt(900160.0f), preserving the original fastSqrtf path. Historical reciprocal square root was a behavior error; recovery now98.75% (64bytes).

Six reference TUs compile. Native temporary overlay compiles five; WipeGameOver is blocked only by native MR::isAnimStopped(LayoutActor*) having the wrong const qualification. No native wipe files or factories have been saved yet during the Group build/fix window.
