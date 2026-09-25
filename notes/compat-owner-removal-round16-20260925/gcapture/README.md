# Restore complete GCapture

Removed the query-only compatibility translation unit and imported the complete existing donor GCapture, GCaptureRibbon and SpringValue implementations. The real GCaptureTargetable interface replaces the former forward declaration and the original SceneObjHolder constructor is enabled. Japanese literals use the existing CP932 boundary; no placeholder actor or fabricated geometry is introduced. This activates the original capture controller and ribbon through normal scene construction. Gateway opening validation does not exercise blue-star gameplay.
