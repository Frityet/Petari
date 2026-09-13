# DVD archive identity

The twenty-fourth original application run passed collision setup and reached Tico's original middle-distance model. `LodCtrl::initLodModel` requested `ticoMiddle.arc`; the original FileLoader had already mounted `TicoMiddle.arc`. FileLoader's original case-insensitive lookup found the request, but native archive caches used case-sensitive filesystem strings and could not receive it.

The shared `DvdFileSystemService::resolve` boundary now ASCII case-folds normalized virtual DVD keys. Mounted archives, layouts, model holders and decoded archive caches use the same identity, including embedded mounts absent from the FST. Requested path trace strings and directory entry names retain their authored spelling. No Game source, actor name exception, extra read or readiness substitute was added.

Reference: `decomp/src/RVL_SDK/dvd/dvdfs.c` path-name comparison applies the SDK lowercase map; the ordinary ASCII disc names now have the same equality in native caches. Evidence: `../gateway-wakeup-demo-20260912/original-app-twenty-fourth-exception.log`. The thirty-fourth ordinary production build passed. The twenty-fifth real run passed the mixed-case model lookup and reached a separate pending FileLoader request blocked by an obsolete whole-model scheduler scope.
