# Original ParticleResourceHolder through mounted archives

ArchiveMountService now recognizes JPAC resources by their format signature
and retains the bounded typed JPC registration with each mount. This applies
to every mounted archive/resource name. Version and structure validation remain
inside the shared JPC decoder; there is no effect-name or stage special case.

ParticleResourceHolder.cpp and its header are literal copies of the restored
root Game source. The regular manager fixture now constructs this actual Game
holder, which mounts its archive and constructs the real JPAResourceManager and
both JMapInfo tables. It checks every authored particle name through the
original binary-search routine, then removes the mount publication before
running the complete emitter simulation. Managers and attached tables retain
their resources and release them at actual JKR heap retirement.

The regular macOS target rebuild and actual-disc run pass: all3,327 names,
45,386 function entries,52,571 frames and352,896 particle observations, with
the same pool and retirement results as before. See holder-build.log and
holder-runtime.log. The fixture remains CPU-only; Game EffectSystem, drawing
and Mario activation are still separate work.
