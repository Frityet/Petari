# Integrated compile corrections

Removing the forced prefix exposes real include boundaries. First diagnostics and fixes:

- Aurora MSL printf implementation explicitly includes cstddef for std::ptrdiff_t; its public C API correctly uses stddef.h instead.
- DemoExecutor.cpp includes LiveActor.hpp before comparing actor and NameObj addresses.
- ScenarioStarter.hpp directly includes TQuat.hpp for its stored quaternion.
- The NWC24 umbrella no longer wraps dependency includes in extern C. Each actual NWC24 declaration header already supplies its own C linkage; the outer block incorrectly pulled C++ host standard-library templates into C linkage.
- The prefix lane follows up on concrete quaternion/type dependencies surfaced by the build. No umbrella header or forced Game type include is restored.

Build attempts are logged independently, preserving failures. No runtime check will run until the app links; no fixture suites are being run.

Further concrete diagnostics were resolved through the real defining headers: direct legacy functional-adapter callers include functional.hpp, and J3dMaterialTableData includes JKRHeap.hpp for integer-aligned allocation. Aurora GXEnum.h now declares the original _GXTlutFmt enum tag, replacing its incidental alias in the Revolution umbrella; J3DGD already includes the correct GX header.

J2DPane includes the canonical degree macros, and JUTFont includes the Revolution type/attribute declarations it uses. Final build-retry10.log links successfully.
