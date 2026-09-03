> Historical design snapshot from before the heap/model owner implementation.
> See README.md and the current evidence files for completed work and remaining integration.

# Complete original model owner (work in progress)

Baseline checkpoint is7250fc59d (Aurora fd632f5). All component tests and existing
title/Gateway smokes pass, but the actual ResourceHolder constructor and original
Mario jumping are not yet activated.

The owner will combine retained INF/JNT/EVP/DRW, VTX/SHP, MAT/MDL and TEX components
into an actual J3DModelData. It must validate cross-block material/shape/joint and
vertex/matrix references before invoking the original hierarchy/finalization.
The existing original J3DJointTree::makeHierarchy body was missing from native
providers; J3DHierarchyCompat.cpp now imports it unchanged and syntax-compiles.
No complete model is published by that import alone.

The configured original compiler compares makeHierarchy at95.73034% (356 retail
bytes,348 compiled), with native isolated compilation passing. Scratch compiler
and objdiff evidence is under build/original-j3d-model-resource-20260903.

Material construction must preserve original BMD3/BDL4 flags and block order:
normal, patched, locked, existing-material MDL attachment, unique materials, and
the distinct material-table-only path. Original SDK constructors and factories
perform the work. Native ownership must account for allocation failure and later
allocations from tex-matrix animation/display-list conversion, not merely the
initial table pointers. General JKR heap/allocation closure is being audited
alongside the OS mutex implementation before choosing that lifetime boundary.

The original material loader derives mDiffFlag values from allocation addresses;
native pointers cannot be truncated. J3dAllocationIdentity reserves disjoint,
32-aligned original-width address intervals between0x80000000 and0xC0000000. It
is an identity allocation only, not a physical-memory mapping. Shifted addresses
leave both state bits clear; unshifted BMT addresses retain the original high-bit
behavior. The material owner must reserve enough identity range for every ID it
adds to the allocation base, retain it for the material lifetime, and use original
object strides when emulating pointer arithmetic. The reservation map coalesces
released ranges and reuses preallocated nodes during retirement, including an
exception path. This helper is compiled but not yet integrated or runtime-tested.

Animation resources expose explicit RAII aliases from archive byte identity to
retained decoded owners. The model owner should use the same pattern: register
its owned data by default, and register actual archive getResource pointers only
through a lifetime-scoped, extent/byte-checked alias. ResourceHolder migration
must be atomic with the existing global archive-wrapper type/service rename.

Do not publish a joint-only model or create a partially initialized holder to
enable jumping. The original animation actors and renderer must share the same
complete model and authored Xanime resources. Collision recovery proceeds
independently before restoring original floor/writeback/action logic.

The agreed allocation API is declared in compat/JkrAllocationDomain.hpp (no
implementation yet): JkrHeapRuntime owns a caller-budgeted host ExpHeap root;
JkrAllocationDomain owns an actual SolidHeap child. JkrAllocationScope retains
and selects the domain for original code; JkrHostAllocationScope keeps native
parsers/registries/caches outside it. current_jkr_allocation_domain() observes
the retained selected domain even inside a host escape. Model and animation
owners retain that domain until actual object destruction. The model-resource
public API is drafted in resource/J3dModelResource.hpp; its implementation and
original SDK entrypoint bridge remain to be written.

Initial native construction components already own their typed STL backing and
SDK objects explicitly. Keep their allocations outside the Game domain unless
their deleters are changed to use the provenance-aware allocator. In particular,
J3dGeometryData currently frees its aligned initial command allocation with
std::free. Later original SDK command-buffer replacement allocations may belong
to the retained Game domain and be reclaimed at its final retirement. Do not
accidentally feed a Game-heap allocation to the component's host-only deleter.
