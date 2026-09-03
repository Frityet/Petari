# Original particle emitter lifecycle

Recovered all seven ParticleEmitterHolder methods and both ParticleEmitter
predicates from the retail Game code. With the configured GC 3.0a3 compiler,
all nine methods match at 100%: 1,056 relocated instruction bytes are identical
to the verified RMGK01 DOL. The complete twelve-byte member-function predicate
and its function relocation also match. `verify.py` reproduces these checks.

The holder's previously guessed extra count field was incorrect. The actual
AssignableArray already stores the count at holder offset8. The original
compiler verifies holder size12, emitter size8, array offset4 and initialization
flag offset5. The constructor, first-free search, group-specific updates,
one-time deletion, pause/resume and callback initialization are the original
algorithms. No emitter name, stage name or resource exception is introduced.

The common MSL functional header gains the standard const reference member
adapter and mem_fun_ref overloads used by the original std::find_if predicate.
The existing original SDK callback type is used for its init virtual call.
isContinuousParticle retains an out-of-line call through the project's
NO_INLINE convention; the compiler emits its original weak body.

This root-first checkpoint does not activate the native EffectSystem owner.
Its actual constructors, executors and resource ownership remain separate
integration work. No rendered gameplay or jumping result is claimed here.
