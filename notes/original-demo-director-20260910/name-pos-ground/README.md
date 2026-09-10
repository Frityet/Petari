# Original named placement on ground — 2026-09-10

Recovered the missing `MR::findNamePosOnGround` in the original reference ObjUtil first, then copied the exact body into the current native provider `OriginalNamePosUtil.cpp`. Genuine HitInfo/GravityUtil/MapUtil includes expose its actual dependencies.

Retail `ObjUtil.s` 803F2CD8–803F2E0C: resolve the authored named matrix; retain its front vector and position; calculate gravity at that position; trace from `position - gravity * 100` along `gravity * 1000`. On a hit rebuild the matrix using up=-gravity, authored front and the hit position. On a miss copy the authored matrix unchanged. Constants @65643/@65644 are exactly 100 and 1000. The retail routine neither checks the original name lookup result nor returns a bool; the paired headers therefore correct the old placeholder return declaration to void. Both existing PlayerUtil call sites already discard the result.

One complete RMGK01 Wii ObjUtil compilation succeeds. The recovered 312-byte function is **99.78205%** against the original; differences are local stack slots. One native provider syntax compilation succeeds. No extra runtime/test cycle or score optimization was performed. Parent owns the combined link and smoke run. Reference changes remain uncommitted for the coordinated checkpoint.
