# MarioAnimator and Xanime ownership — round 13

Removed `src/compat/MarioAnimatorLifetime.cpp/.hpp`. Ownership now belongs to the objects that allocate the state; there is no replacement capture service or global registry.

- `XanimeCore` owns its track array and shares ownership of joint/transform arrays with the original upper/lower core constructors and `shareJointTransform`. Shared-pointer bookkeeping is host allocated; the actual arrays use the caller's original selected heap. Construction failure releases completed allocations.
- `XanimePlayer` owns its constructed core and private duplicated simple group. Model/resource pointers remain borrowed, and mutable public pointers do not decide which allocations are deleted.
- `XanimeResourceTable` owns its hash table and allocated simple-group array. Original static Mario tables remain borrowed. The ordinary constructor now initializes the optional simple-group view to null.
- `MarioAnimator` owns its two players, callback table and resource table, retaining the actual ResourceHolder token until those children retire. Reverse member destruction releases the upper/lower cores before the animator matrices they borrow.
- `MarioAnimator::createNative` retains the actor's actual ModelManager, constructs under its real heap domain, and gives the completed animator to `ModelManager::retainNativeDependency`. Its deleter retains the domain through operator delete. This replaces the single `new MarioAnimator(this)` call in MarioActor; adoption occurs only after construction, so an allocation failure cannot double-delete a partially constructed `this`.
- The constructor retains the existing `J3dCommandScope` behavior (recursive heap/J3D mutex recovery, GD/interrupt restoration), and restores `j3dSys` on success or exception. The factory restores the prior ModelManager player on any failure, including shared-owner allocation or dependency-vector growth.
- ModelManager now deletes the original player once (the player deletes its core) and owns its original resource table explicitly. It still retires authored animator dependencies before its original model and resource leases.

The joint-tree bridge and four existing fixtures no longer independently delete Xanime-owned arrays/core/hash/simple-group storage. Their existing behavior assertions remain unchanged; no cases were added.

## Integration

18 existing paths changed/deleted, all clean in the parent's round-13 baseline. Exact pre-edits are under `before/`, the scoped delta is `scoped.patch`, and the file/hash manifest is `owned-manifest.json`.

No explicit build-file change is needed. Regenerate the compat source glob for the deleted pair. No builds, tests or Git operations were run by this agent, following the requested reduced verification. A source search found no remaining MarioAnimatorLifetime/MarioAnimatorConstructionScope/createdCore references or independent manual Xanime array owners under src/tests.
