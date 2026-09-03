# Original collision query CPU runtime — 2026-09-03

The complete original KCollisionServer point and area routines pass 25 new
query cases in normal and AddressSanitizer/UndefinedBehaviorSanitizer runs.
The existing ten raw-resource test groups also pass in both runs. No sanitizer
error was reported. Exact commands, binary hashes and source hashes are in
`runtime-evidence.json`; `normal.log` and `asan.log` retain the results.

The fixture encodes bounded big-endian KCL/PA data and registers its actual raw
byte identity with KCollisionSourceRegistration. Both original KCollisionServer
objects and their JMapInfo attachments are allocated in an actual JKR scene
heap. Both servers resolve to the same decoded resource, including its mutable
prism state. The tests execute the complete staged KCollision.cpp and original
KCollisionPlus.cpp, rather than a replacement query algorithm.

The 14 point cases cover stored leaf order, octant traversal, face and thickness
boundaries, edge rejection/inclusion, masks, scaled thickness, disabled prisms
and untouched output on a miss. The ten area cases cover repeated prism and
leaf identities, reversed endpoints, one-result capacity, zero-width expansion,
triangle bounding boxes, touching boundaries, outside-volume rejection and
nonpositive prism heights. One further query through the second server proves
that shared prism initialization state affects both real servers. The test also
checks archive immutability and original JKR disposer cleanup with a retained
PA table.

`verify.py` imports the existing heap runtime/link recipe and compiles a literal
complete MR::createBoundingBox extraction. It asserts that this body is identical
in root and native MathUtil before compiling it. This CPU-only extraction avoids
pulling unrelated unresolved quaternion methods from the full native MathUtil TU;
it is not an alternate bounding algorithm or a production source-selection
change. Current native headers take precedence; only the reviewed staged raw KCL
resource-registration header is overlaid.

The remaining integration frontier is explicit: this fixture executes resource
and server queries, not CollisionParts::init, keeper registration, original
Binder or Mario. Actual parts initialization always enters the original camera
code collector and needs the actual CameraDirector plus scenario zone catalog.
The camera package currently has isolated compile closure, while its full
runtime owner transaction is pending. There is no null collector or synthetic
part/actor in this fixture.

When the actual camera/scene transaction is available, the owner test must create
placed parts from retained archive resources and real HitSensors, register them
in the actual keepers, run current/previous transform and query cases, then
quiesce scheduler/query borrowers before retiring actors and resources. Native
registry ownership must retire while JKRDisposers are live; a post-disposer heap
finalizer is too late for audio/model ownership and can form a retained-domain
cycle. Parent-owned native activation remains gated on that graph. No shared
build, GPU runtime or production-native mutation occurred here.
