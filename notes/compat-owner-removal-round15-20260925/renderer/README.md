# Remove unused alternate J3D renderer

Source searches of both HEAD and the worktree found no production instances or calls of J3dModelRenderer; only its own implementation, an obsolete trace record type, and standalone fixtures remained. Deleted that separate rendering pipeline and its OriginalJ3dJointTree compat pair. The game already uses actual J3DModel/J3DJointTree/Xanime owners.

Removed the orphan renderer packet producer/API/serialization while retaining actual Aurora GX trace and layout records. Removed three standalone fixtures tied to the retired renderer/compat model bootstrap; the GX parsing fixture retains its independent matrix/selector/resource assertions, dropping only the retired renderer light helper test. No replacement test framework was added or executed.

Root coordinates RuntimeContext edits after the NAND lane freezes those shared files. Before snapshots and manifest preserve dirty work for isolated staging.
