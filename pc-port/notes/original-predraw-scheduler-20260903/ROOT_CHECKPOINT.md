# Root category remove recovery

Only `src/Game/NameObj/NameObjCategoryList.cpp` changes: add `<algorithm>` and the original missing remove method, which swaps the final element into the removed slot and decrements the count. No invalid-pointer guard was added: callers must remove a registered pointer, matching the original contract.

All 15 category text methods, including new remove at 0x80261E78/0x6C, and two delegator vtables are 100% original compiler matches. Existing NameObjListExecutor forwarding methods are 100%; its existing destructor remains 98.41% due existing draw-holder decompilation. DOL evidence verifies all corresponding retail text functions against the expected DOL SHA1.

`root.patch` and `root/` are frozen independently of native scheduler staging. Parent may commit this one root source while native service integration continues.
