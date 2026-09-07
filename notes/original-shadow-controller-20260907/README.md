# Original shadow projection recovery

Recovered the missing ShadowController constructor, gravity and collision updates, projected distance query, collision/gravity mode queries, matrix binding, holder registration, and actor lifecycle requests in decomp first. The complete reference TU compiles with the configured Wii compiler. The native reference/header-overlay syntax probe also passes; native owner activation and gameplay validation are still pending.

The projection update is a 100% comparison match and uses the original map/water-surface segment queries, start offset, collision filter, actual triangle sensor and normal, and one-shot count. Gravity fallback restores the old direction only when both original gravity queries fail. Constructor: 90.23729%; direction: 94.411766%; distance: 91.75676%. Differences are helper inlining and return-register scheduling. Exact values and call effects were inspected against the retail object.

The retail isCalcGravity repeats its mode-0/mode-3 rejection condition in its later one-shot branch. Preserving that source pattern matches 100%, including the unreachable count branch. This is intentionally retained for accuracy. isCalcCollision accepts one-shot mode only while its unsigned count is below one. The private-gravity predicate uses an explicit u8 wrap before comparing.

Retail: RMGK01 DOL SHA1 25c5959534b3c21246c6c7e42021b916b41fb578. Machine-readable per-symbol comparison: reference-proof.json. No original game assets or objects are included in this checkpoint.
