# Original placement grouping, ordering and construction

The complete PlacementInfoOrdered source is recovered. It closes the missing
ordering object called by actual StageDataHolder initialization, and preserves
original grouping, resource priority, shell sort, archive requests and actor
creation. No native scene/camera activation is claimed here.

`verify-original.py` compiles with the configured original GC3.0a3 toolchain
and checks all 16 retail methods/helpers (2,152 bytes). Twelve methods totaling
1,592 bytes are identical after relocation, including the full 408-byte sort,
attach, insertion, lookup, group/index allocation and both constructors.
All four other methods have equal canonical instructions with these explicit,
bounded differences:

* requestFileLoad, 98.21429%: one bijective callee-saved register permutation.
* initPlacement, 98.22034%: verified register live ranges; the compiler reuses
  the retired group register for the object name and another for the new actor.
* getUsedArrayNum, 98%: three independent integer increments are reordered.
* Index destructor, 95.652176%: the compiler omits one unreachable duplicate
  null-this branch immediately after an exit using that same condition.

The verifier rejects any other instruction, branch, argument or call change.
Function-pointer relocations are resolved by exact function identity, never
by a coincidentally equal instruction prefix. The ordinary empty string is
verified against its actual r13 SDA target. compiler-evidence.json records all
method scores, canonical instructions, references and source hashes.

SameIdSet now uses its actual Identifier base (the first name/shape fields),
so shared creator lookup uses ordinary typed conversion instead of fabricated
layout casts. Identifier initialization precedes list construction exactly as
retail. The unchanged BothDirPtrList(bool) body moves inline into its header,
allowing the proven original initializer to call initiate directly. The source
includes its own header instead of the unrelated complete Game utility umbrella.
`verify-list.py` proves all seven other list/link methods remain identical:
624 bytes and every relocation. Original SameIdSet constructor is byte-exact.

The algorithm groups by case-sensitive name plus shape ID and preserves each
row's JMapInfoIter in its original linked list. Player archive loaders sort
first; other groups use original DVD-resource queries and then descending group
size. Shell gaps and insertion behavior are literal retail. Archive requests
retain per-row traversal for ordinary objects. Actor initialization preserves
original zone publication and both original initLiveActorSystemInfo calls.
The existing fixed capacities are retained, without host growth or truncation.

Root checkpoint files:

* src/Game/Scene/PlacementInfoOrdered.cpp
* include/Game/Scene/PlacementInfoOrdered.hpp
* src/Game/Util/BothDirList.cpp
* include/Game/Util/BothDirList.hpp

## Native staging and remaining ownership

native.patch contains six staged files. Three complete translation units
compile with native flags: original PlacementInfoOrdered and BothDirList, and
OriginalPlacementJMap containing the complete original shape-ID helper. The
native JMapUtil header receives only its existing missing declaration.
native-build.json records actual commands/results; native-manifest.json lists
exact source hashes. Staging is under build/original-placement-info-ordered-20260903.
No shared build, GPU session or production native mutation was performed.

Runtime remains gated on actual StageDataHolder raw table address identity,
its authored archive leases, and complete original NameObjFactory/resource
queries. A compile result is not a working placement or Mario/camera runtime.
The source-recovery evidence validates the ordering algorithm without supplying
fake factory answers or substituting a host container/sort.
