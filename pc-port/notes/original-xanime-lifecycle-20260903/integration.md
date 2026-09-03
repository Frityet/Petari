# Resource-table foundation and current movement validation

The PC Game archive now builds the complete original `ResourceInfo.cpp` and
`XanimeResource.cpp`. The extracted ResTable/XanimeResource query definitions
have been retired. Complete original HashSortTable methods and the unsigned
sortSmall overload provide the group-index construction dependency.

ResourceInfo has two compile-only adjustments: explicit HashUtil inclusion
because the PC umbrella header does not expose it, and standard cstddef in
place of the Metrowerks size_t.h include. Every function body is unchanged.
Original J3DAnmBase/J3DAnmTransform declarations and the original transform
constructor are supplied at the JSystem compatibility boundary. Their native
pointer/vtable sizes can differ from Wii sizes; the imported Xanime code reads
metadata through the named fields/accessors rather than fixed byte offsets.

`verify-resource-tables.py` checks these source correspondences.
`verify-resource-import.py` checks the Xanime import using the original
compiler: all 18 function bodies produce the same 885 PPC instructions and 81
relocations as the current root source. That comparison verifies the port
adaptations, and is not a claim that all of root Xanime matches retail.

## Native validation

Built `smg-pc-showcase`, `smg-pc-mario-gateway-walk-tests`,
`smg-pc-resource-table-tests`, and `smg-pc-hash-sort-table-tests` successfully
on macOS arm64. The native tests passed:

- Three ResourceInfo groups: owned/extension-stripped names, raw versus loaded
  resource metadata, case-insensitive lookup, registered null versus absence,
  reverse pointer aliases, original hash-collision precedence, and high-byte
  resource-name hashing.
- Two HashSortTable groups: unsigned bucket ordering and payload preservation,
  missing queries, duplicate handling, composite-name lookup, and re-sorting
  after rename/insertion.
- Real-disc Mario stand/walk/release proof, including the corrected retail
  init2 gravity multiplier of 1.0. Walking remains 325.684 units, with
  Wait -> Run -> Wait and retained live collision/lifetime checks. The ratio
  correction's retail evidence belongs to the separate gravity-restoration
  note. Jumping is not exercised or enabled by this fixture.

The tests construct actual ResTable/HashSortTable objects and retain their
original allocated children. They do not fabricate a ResourceHolder, loaded
animation, or player. Logs stay ignored.

## Next original movement dependency

Full original animation execution remains unactivated. The current native
archive wrapper conflicts in global name/layout with the original
ResourceHolder; it must be replaced with a properly constructed original
holder and real typed resource tables. Motion entries need stable
J3DAnmTransform objects backed by converted BCK/BCA arrays. Model/material
entries likewise need actual loaded J3D objects, not raw archive pointers.
Those resources must outlive the groups and players that borrow them.

The transform declarations here do not provide transform sampling, animation
loaders, XanimeCore joint/blend routines, or original MarioAnimator construction.
Completing that shared resource/J3D/Xanime ownership chain is required before
the recovered Mario update/jump pipeline replaces the current PC walk path.
