#if defined(PROBE_NATIVE_RTTI)
#include <nw4r/ut/RuntimeTypeInfo.h>
#else
#include "decomp/libs/nw4r/include/nw4r/ut/RuntimeTypeInfo.h"
#endif

namespace rtti_probe {
struct Root {
    NW4R_UT_RUNTIME_TYPEINFO;
};
struct MutableChild : Root {
    NW4R_UT_RTTI_DECL(MutableChild)
};
struct ConstGrandchild : MutableChild {
    NW4R_UT_RUNTIME_TYPEINFO;
};
struct OtherChild : Root {
    NW4R_UT_RTTI_DECL(OtherChild)
};
NW4R_UT_RUNTIME_TYPEINFO_ROOT_DEFINITION(Root);
NW4R_UT_RTTI_DEF_DERIVED(MutableChild, Root);
NW4R_UT_RUNTIME_TYPEINFO_DEFINITION(ConstGrandchild, MutableChild);
NW4R_UT_RTTI_DEF_DERIVED(OtherChild, Root);

int run() {
    Root root;
    ConstGrandchild child;
    Root* base = &child;
    const Root* constBase = &child;
    int failed = 0;
    failed += nw4r::ut::DynamicCast<Root*>(base) != base;
    failed += nw4r::ut::DynamicCast<MutableChild*>(base) != &child;
    failed += nw4r::ut::DynamicCast<ConstGrandchild*>(base) != &child;
    failed += nw4r::ut::DynamicCast<OtherChild*>(base) != NULL;
    failed += nw4r::ut::DynamicCast<MutableChild*>(&root) != NULL;
    failed += nw4r::ut::DynamicCast<MutableChild*>(static_cast<Root*>(NULL)) != NULL;
    failed += nw4r::ut::DynamicCast<const MutableChild*>(constBase) != &child;
    failed += nw4r::ut::DynamicCast<const ConstGrandchild*>(constBase) != &child;
    failed += nw4r::ut::DynamicCast<const OtherChild*>(constBase) != NULL;
    failed += !child.GetRuntimeTypeInfo()->IsDerivedFrom(&Root::typeInfo);
    failed += Root::typeInfo.IsDerivedFrom(&MutableChild::typeInfo);
    failed += child.GetRuntimeTypeInfo()->IsDerivedFrom(NULL);
    return failed;
}
}
#if defined(PROBE_HOST_MAIN)
#include <cstdio>
int main() {
    const int failed = rtti_probe::run();
    std::printf("RuntimeTypeInfo mixed const/mutable macros: %d/12 passed\n", 12 - failed);
    return failed == 0 ? 0 : 1;
}
#endif
