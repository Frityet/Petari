# Upstream build integration

Upstream f1189541 replaces gfx/common with frame, recording, encoding and
resource-cache modules, and adds GX register storage, shared thread/time and
atomic IO services. CMake and xmake include those real files. Our existing
acyclic base/platform/core split is preserved: IO, thread and time belong to
base alongside existing device/input/logging, window remains in platform,
and core retains the frontend. Existing GX destruction-state support remains
alongside new register storage. The removed common.cpp is not built.

Upstream requires Dawn v20260807.225922 for the new immediate-data APIs.
The xmake defaults and parent Dawn recipe are updated to that release.
All eight platform asset hashes come from the GitHub release API digests;
see dawn-release-digests.json. Existing older package hashes remain available
for explicit historical configurations. The local build will be configured
with the new version, retaining other selected options.

No production Game code is changed by this build adaptation.
