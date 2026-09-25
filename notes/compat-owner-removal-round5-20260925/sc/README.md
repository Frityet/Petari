# Wii system configuration belongs to Aurora

The complete existing native SYSCONF catalog, original SDK accessors and original encrypted product-setting reader now live in Aurora's OS library. This removes three compat providers, the port's SystemConfigService class, and the port-local revolution/sc.h. Aurora exposes SystemConfiguration and the SDK header directly; there are no forwarding headers or old type aliases.

The host supplies the actual NAND filesystem and constructs the owner only after OS memory initialization. The decoded catalog and borrowed product-memory range retain their existing lifetime and interrupt serialization. All six C entry points lock before resolving the current owner as well as inside the catalog methods. Host allocation is the same Aurora allocation scope previously reached through the JKR alias. Application-specific initial language policy remains with the application.

Xmake and CMake both include the three SC sources. CMake also gains nand.cpp, which Xmake already included and the catalog requires. The integrated Xmake application build passes, as do OriginalSystemConfig, ConsoleNandImport, OriginalViRenderMode and RuntimeContextFailure. CMake wiring was reviewed but a CMake build was not run. The independent source review in review/ passes 29 checks, including complete accessor/product body equivalence and consumer lifetime order.

Aurora was clean at edffbd164d373e48de30d4be9997d952cdea7800 before this change. The root's prior gitlink was 8c19ab45d233eb43d0c817256e8a348cc98746cf. Advancing the root gitlink therefore includes the already published GX resize commit edffbd1 as well as this new SDK work. The new Aurora commit is published before the root reference.
