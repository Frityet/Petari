# Native SDK and owner integration

- Moved Binder's necessary native plane-array destruction into actual Binder.cpp; restored offset setters through complete LiveActorUtil and deleted BinderCompat.
- Removed NativeJkrDisposer. Actual JKRDisposer copy/move construction now registers the destination identity; assignment preserves the existing intrusive link. JMapInfo directly inherits JKRDisposer, and the existing SDK identity test now names that actual class.
- Replaced J3DModelLoaderCompat with canonical J3DModelLoader.cpp and a native finalization entry point on the actual SDK loader. Model resource decoding calls that owner; hierarchy/billboard algorithm is unchanged. Original J3D material factory methods now live in their actual SDK source file, deleting another compat provider.
- Enabled full LiveActorUtil, ActorShadowUtil/LocalUtil, and MapUtil with scalar floating point flags. Root supplied existing donor ShadowSurfaceOval/Box and ShadowVolumeBox/FlatModel sources/headers for the complete shadow utility closure; only native includes, CP932 literals, an inline declaration correction, and GX matrix backing pointers differ.
- Corrected the LP64 getResName(0UL) overload ambiguity to original 32-bit 0U in LiveActorUtil.
- Retired the obsolete synthetic shadow-owner target. Focused fixtures are not run or expanded under the user's fast iteration instruction.

- Integration restored original JMapInfo::begin; updated provider records to canonical source paths.
