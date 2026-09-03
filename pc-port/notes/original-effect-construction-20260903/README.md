# Actual effect construction closure

This tranche stages the original construction graph from LiveActor to EffectKeeper, MultiEmitter and the particle resource/manager classes. Thirteen native translation units compile in isolation. Thirteen missing original methods were recovered and checked with the original compiler. No production native header, effect helper, runtime service or build selection was changed, and no partial/fabricated object graph was executed.

## Concrete files

* `native/` is the frozen native staging tree, copied from `build/original-effect-construction-20260903/staged/`. EffectKeeper, MultiEmitter, their callbacks, SingleEmitter, ParticleResourceHolder, EffectSystemUtil and the inspected JPA classes retain complete original method bodies. The two `Original*.cpp` files contain complete literal method extractions from LiveActor and Overwrite.
* `liveactor-owner-proposal.patch` replaces the facade-only native LiveActor::initEffectKeeper with the complete original body and adds its actual EffectKeeper include. This patch is deliberately gated on the resource/service closure below. It is not an independently runnable feature patch.
* `general-header-proposal.patch` restores original JMapInfoIter equality/inequality and TMatrix::setEulerZ and fixes FixedArray scalar instantiation. These three small general native header changes are separate from the effect API and owner activation. `general-header-manifest.json` records their baseline and resulting hashes.
* `root-recovery.patch` and `root/` record the root changes, already present in the shared tree. They are independent of the previous MarioEffect flag edit and the committed HashSortTable payload work.

## Recovered original behavior

`MultiEmitter::allocateEmitter` resolves a base name into numbered resource names, or tokenizes explicit names separated by spaces, then constructs and initializes the actual SingleEmitter array. It preserves the original 32-entry temporary collection, 40-byte token buffer, lookup order, 16-bit resource indices and base-name hash. Construction does not manufacture a JPABaseEmitter; original playback does that through EffectSystem and JPAEmitterManager later.

ParticleResourceHolder now has its complete original archive constructor, binary name lookup, auto-effect group counting, texture swap and query methods. The constructor mounts the archive, constructs the actual JPAResourceManager from `Particles.jpc`, attaches `ParticleNames.bcsv` and `AutoEffectList.bcsv`, then builds the group counts. Retail establishes that its cached groups are **1,024 Particle pointers**, with separately allocated name/count records. The previous header incorrectly described 512 inline records. Both layouts happened to occupy the same Wii byte span, so only following the pointer loads revealed the error.

EffectSystemUtil's four recovered resource wrappers call the actual ParticleResourceHolder. The numbered variant uses the original `%s%02d` formatting. They do not return constant availability or bypass resource lookup.

The complete SMG JPABaseEmitter::init override is appended to its actual split owner, root `Game/System/Overwrite.cpp`, preserving that file's earlier methods. It reads emitter scale/translation/direction, normalized direction, rotation, timing, rate, volume and velocity parameters from the actual resource; advances the manager random seed; resets transforms/colors, user work and lifecycle flags; and obtains resource colors. Its staged native provider is a literal extraction. The original JPAEmitterManager constructor/createSimpleEmitterID remain the owners of actual emitter pool objects and resource association.

Two general header corrections are required by these bodies:

* FixedArray::callAllFunc now deduces its member-function class as a member template. The old `Base::*` declaration was ill-formed when FixedArray was instantiated for a scalar such as u16, even though callAllFunc was never used. Storage and array algorithms are unchanged.
* JPABaseEmitter's user-work accessors already accepted/returned uintptr_t, but its storage remained s32. The field is now uintptr_t too, retaining Wii size/offset while allowing the original SingleEmitter backlink to carry a native pointer without truncation.

## Validation

`verify-retail.py` compiles the four complete root units and compares the recovered methods to the RMGK01 retail split from the verified DOL (SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`). `retail-evidence.json` records commands, source hashes, sizes and scores:

| Method group | Comparison |
| --- | --- |
| MultiEmitter::allocateEmitter | 99.861115% |
| ParticleResourceHolder constructor | 99.86667% |
| ParticleResourceHolder::getUserIndex | 97.58904% |
| ParticleResourceHolder::countAutoEffectNum | 90.65958% |
| Four ParticleResourceHolder query/swap methods | 100% |
| Three EffectSystemUtil resource queries | 100% |
| Numbered resource query | 99.545456% |
| JPABaseEmitter::init | 99.625% |

Compile-time original layout checks confirm ParticleResourceHolder size 0x1010 and group-count offset 0x100c, and JPABaseEmitter size 0x114 and user-work offset 0xc0. These are original-compiler/fuzzy comparisons, not claims of fully relocated byte equality for every method.

`stage.py` stages sources, compiles all thirteen native units and records `native-compiles.json`. It does not invoke Xmake, link the game or launch a GPU process. `direct-undefined.txt` captures only those inspected objects. Reproduction:

```sh
python3 pc-port/notes/original-effect-construction-20260903/verify-retail.py
python3 pc-port/notes/original-effect-construction-20260903/stage.py
```

## Smallest honest resource/emitter boundary

The native facade already simulates its own JpcEffectEmitterInstance objects. Those instances cannot be cast to JPABaseEmitter, ParticleEmitter or MultiEmitter. The smallest general boundary that preserves the original Game code is **JPA resource decoding and actual JPA manager ownership**:

1. Process/stationed resource ownership retains the actual ParticleResourceHolder, mounted Effect.arc and native JMapInfo attachments. The original GameSystemObjHolder owns this resource pointer. A native host owner must use the retained process allocation domain and publish this actual object through MR::getParticleResourceHolder; that getter currently has no native provider. Strings and decoded JPA data must outlive every scene emitter.
2. The original JPAResourceManager constructor calls JPAResourceLoader. That loader and every JPA block currently read native integers/floats through pointers into raw JPC bytes. The original JPC bytes are big-endian. Simply selecting the compiled original loader on a little-endian host is incorrect, including its initial version check. A general native loader must validate lengths and construct host-order instances of the original JPA block data, resources, arrays, key/color tables and textures. Retain those backing allocations with the process resource owner. Reuse existing EffectResource parsing where it covers the exact fields; its metadata currently omits fields needed by the full original JPA classes, so it cannot be treated as a complete decoded resource by assumption.
3. A scene-owned actual EffectSystem owns JPAEmitterManager and ParticleEmitterHolder plus calculation/drawing registration. The original manager creates real JPABaseEmitter pool entries, initializes them with the recovered override, links them into group lists, and assigns callbacks. ParticleEmitter wrappers and SingleEmitter user-work backlinks refer to those same actual objects. `EffectSystem::{constructor,entry,createEmitter}` and ParticleEmitterHolder allocation/find/update methods still need their original bodies; compiling the existing partial sources cannot replace that work.
4. Original AutoEffectInfo / AutoEffectGroup / AutoEffectGroupHolder and the registration/setup methods in EffectSystemUtil must populate the actual keeper before MarioActor::initEffect performs renames and joint-bound registration. These sources remain missing. Keeper construction cannot honestly skip this for a nonempty group. Their BCK setup also reaches missing original MultiEmitter sync methods.
5. The complete original LiveActor::initEffectKeeper body can then replace facade registration. The actor's retained model matrices, Binder and animation owner must already exist. Original EffectKeeper update/clipping/clear and scene emitter retirement must run before actor/model memory is reclaimed. Its full TU overlaps OriginalEffectBckNotification::changeBck, so select one provider.
6. Publish the original EffectUtil header and original pointer-returning helpers together with that real owner graph. Keep the existing native void MR::emitEffect provider until replacement is coherent; C++ does not encode its return type in the symbol, so a header-only change would silently link the incompatible facade.

The staged JPA constructors and manager compile successfully, but this report does **not** claim resource decoding, effect playback, or the complete constructor chain has been linked or executed. No null service, fabricated resource, substitute emitter or actor-specific exception was added to make such a claim.
