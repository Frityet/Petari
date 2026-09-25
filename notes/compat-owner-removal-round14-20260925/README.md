# Compat owner removal, round 14

Removed nine more compat files (60 to 51): collision, gravity and effect ownership pairs, J3dCommandScope pair and SceneJ3dScope header. Actual Game objects now destroy their own allocation graphs; actual J3DSys owns command serialization and nested shared-context restoration. Scene construction uses the actual owners with its existing general NameObj transaction rather than three subsystem capture helpers.

Collision keepers retain category publication services for surviving parts. Gravity fields unregister from their manager and followers clear retired target borrows. Effect executors own their callback adaptors, keeper destruction retires callback-bearing emitters, and the secondary ScenarioSelectScene effect system is destroyed before process resources. Actual process particle metadata remains authoritative. No new decompilation, game encounter hacks or renamed catch-all sidecar was introduced.

Validation followed the requested reduced scope: one `xmake build smg-pc` completed successfully and one fresh-save Metal Gateway opening run completed 120 frames with exit 0, unchanged binary SHA256, no timeout and no remaining process. This exercises startup/opening/shutdown, not the rabbit route or Rosalina appearance. Individual fixtures were not run. The obsolete standalone sidecar effect fixture was removed; surviving tests received only required API/ownership adaptations.

Initially dirty production/test edits and the six preexisting staged route notes are preserved with a separate Git index. Lane manifests/snapshots record ownership; dirty files are staged as scoped deltas or direct API adaptations on the committed baseline. The initially dirty GlobalGravityOwnership header is intentionally removed with its entire obsolete service. No Aurora or decomp changes are part of this batch.

See collision/README.md, gravity/README.md, effects/implementation.md, j3d/README.md, scene/README.md, validation.json and next-removal.md for details.
