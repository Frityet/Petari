# Original warp pair query and draw owner

Restored WarpCubeMgr::getPairCube from the retail DOL: skip the source, skip candidates with no group identity, compare original group and zone IDs, and return the first matching registered cube. It does not filter by activity; the caller owns that decision. Fresh full Wii compile passes; the 136-byte query matches100%.

Also recovered WarpCube::draw to complete the original actor vtable before enabling its whole native translation unit. It uses the actual authored rotation and position, 120-unit local-up offset, original validity colors (0x1FFF0080 and0x1FCF0010), and original GX draw path. The364-byte function matches99.94505%; every compiled WarpCube function is>=99.82%, with constructors/camera/init/query methods100%. Full native source mirrors reference and object-compiles. No invented manager or pairing table is supplied.
