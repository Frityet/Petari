# Original map-object source checkpoint

Restored exact original factory registrations for five RotateMoveObj placements and three KoopaJrNormalShipA SimpleMapObj placements. Native RotateMoveObj source/header are byte-identical to canonical decomp; the shared class-based unique-planet creator also makes all authored rows for RotateMoveObj available. Game/Util.hpp gains the canonical MapPartsUtil include needed to compile the import. No replacement rotation, collision or LOD algorithm is introduced.

The existing original MapObjActor/MapPartsRotator/LodCtrl pipeline constructs real main/Low archives and main/category3 MoveLimit collision. Canonical LodCtrl::update is now recovered and independently MWCC-checked at90.2381% function /98.64883% full text; native LodCtrl behavior was already present. Decomp commit3c8ca69ba1d0c8367f983e30ce6182799a9694a3 was published first and remote-verified.

registration-build2 passes after the original aggregate header include repair. registration-smoke completes360 original GameSystem frames, exits0, process reaped, no timeout, binary unchanged02ff5353a63e2fe6d6f856dcfb5653279a982a8032d74a0ebfb6030cf5635e28. Coverage187supported/51known-unlinked/0unknown/5metadata, up from179supported. This is a source/startup checkpoint, not a full gameplay claim. Detailed real resources and remaining51scope are included under rotating/, ship/ and scope/.

Additional ownership probes and a native controller run are still being finalized. The first long run reproduced a separate general Aurora display-copy lifetime failure during resizing; that fix belongs to the next checkpoint.
