# Original map-parts rotation owners

Recovered the complete original MapPartsRailRotator, MapPartsSeesaw1AxisRotator and MapPartsSeesaw2AxisRotator in canonical decomp/src/Game/MapObj with their actual typed layouts, then copied their sources and headers unchanged to the native Game module. These are lower owners required by the original MapObjActor initializer; they implement full state transitions, matrix updates, authored rail settings, player torque and ground-pound behavior.

The rail controller retains the original two matrices and a pointer-width-correct borrowed host matrix. Both seesaws use the real MapPartsRotatorBase virtual interface. Its original empty destructor is defined inline; retail emits that same empty derived body in LavaHomeSeesawRotator (80259798), with normal base destruction. Seesaw1 angular speed and applied force are actual named fields at Wii offsets 0x68/0x6C, replacing callers' former incompatible MapPartsRotator matrix views. Sound thresholds were named according to their real comparisons. Seesaw2 preserves the original rate-minus-previous-field recurrence at 0xC4, including its original sound trigger.

Six indexed MapPartsUtil wrappers were recovered with 100% retail instruction matches: rail rotate speed/time/angle/axis/type and speed-calculation type. They dispatch to the corresponding original rail-point argument accessors. The source/header files are now owned by the parallel full MapPartsUtil recovery and are intentionally excluded from this frozen source manifest.

One missing native JGeometry helper, setRotateDegree(const TVec3f&), was added as the exact existing reference operation, converting degrees then calling the common rotation implementation. It fixes the production compile frontier without altering Game behavior.

## Evidence and limits

The original Metrowerks compiler accepts all three recovered TUs. Objdiff compares each against the extracted retail Game/MapObj object; per-method scores are in method-match-summary.json. Branch and scalar operations were read directly from the corresponding retail assembly in notes/gateway-audit-20260907/restoration/retail/asm/Game/MapObj.

Most principal methods match 90–100%. Rail updateRotateMtx retains the same Rodrigues rotation and matrix concatenation through the canonical SDK implementation, with different inline scheduling (88.45%). Seesaw1 calcRotatedAngle has a lower instruction score because the existing canonical JMAAcosRadian definition inlines its table implementation, whereas retail calls that helper; the gravity fallback, normalization, dot snapping, acos operation and degree conversion are preserved. Seesaw1 clamp differences are register/branch arrangement around the same original ordered comparisons. No register padding, assembly or matching-only behavioral edits were introduced.

All three native TUs compile with the existing production compiler configuration. The shared production build and gameplay integration belong to the parent task. No component tests or gameplay success are claimed by this recovery checkpoint.

source-manifest.json freezes the 12 class source/header files plus the narrow shared base-destructor and native matrix-helper files. Other shared edits and the index were left untouched.
