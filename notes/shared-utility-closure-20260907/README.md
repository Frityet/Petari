# Shared original utility closure — 2026-09-07

Original StringUtil, ModelUtil, MtxUtil, MathUtil, ActorSensorUtil, HitInfo, PlayerUtil and sound forwarding bodies were copied into their existing native providers. The JSON manifests record reference paths and body hashes. The original unclamped perpendicular-foot helper is retained, including its division behavior; water queries use the same geometric operation.

MR::vecScaleAdd now translates the original MathUtil paired-single helper into portable fused operations. It loads both complete vectors before writes and uses one fused multiply/add per component. The existing math test distinguishes fused and unfused results with an exactly representable -2^-46 residual and checks self-aliasing. This permits the original MarioModule scaled-velocity helper to replace its earlier multiply-then-add shortcut.

MR::invalidateCollisionParts(LiveActor*) disables the real primary KCL registration. Existing built-BVH query filters immediately observe that membership change, and actor appearance does not silently undo it. The real FileSelect collision fixture covers repeated invalidation and dead/reappear persistence. This change does not claim original validateCollisionParts matrix/radius update support.

Original shadow cylinder start/end offsets and volume-cut flag now update the actual native shadow controller. Signed offsets are retained. The repaired real Tico fixture validates joint/model lifetime separately.

These are provider restorations and bounded compatibility changes. Application linking and full Mario movement/camera execution remain under validation.
