# Observed integration fixes

The first build exposed ModelUtil's undeclared createAndAddResourceHolder after removing the forced CollisionPartsCompat include. The collision lane added its actual ObjUtil header and completed native Triangle finalizer/capacity cleanup before retry.

The next build exposed WhirlPool's stale placeholder padding `(0xD8) - sizeof(LiveActor)` becoming a negative array size after native actor ownership members were added. Replaced the placeholder header with the complete existing donor declaration from decomp/include/Game/MapObj/WhirlPool.hpp. Its actual typed members adapt to the host ABI naturally. No new decompilation or guessed padding was added; this does not enable the unlinked actor implementation.
