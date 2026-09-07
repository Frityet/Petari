# Original MarioActor initialization and movement audit

The five requested reference methods are complete and freshly Wii-compiled against current headers. The constructor matches retail 99.98%, init2 99.67%, initAfterPlacement 99.93%, movement 99.88%, and control 99.53%. control2, controlMain and the actor vtable match 100%. These scores cover the named methods; unrelated existing full-TU warnings and low-score methods are not validated by this audit.

A complete native MarioActor.cpp probe substitutes exactly these five reference method bodies into the current native TU. LLVM23 object compilation succeeds with only reference `_1D8`/`_1DC` assignments mapped to existing native `mRasterBuffers[0]`/`[1]`. No production source was changed.

## Substantive current native divergences

- Constructor omits original Luigi and dark-comet health conditions.
- init2 rejects authored initial-animation variants and forces Mario identity. It omits joint matrix storage, original MarioEffect and CollisionShadow owners, action matrices, sensor setup, MarioMessenger, morph names, event camera declarations/target binding, FootPrint, hand transforms, raster buffers and movement-history storage. It sets gravity ratio to 1 instead of original 0; the historical retail-source else branch also contains this wrong constant. Restore current reference bodies, not that historical branch.
- initAfterPlacement manually constructs a new gravity/front basis and camera state, omitting original gravity update, hit-sensor synchronization and camera-info calls.
- movement replaces the original matrix history, collision/press/moving-platform correction, movement-relative recording, camera synchronization and animation gating with direct field copies and unconditional animator update.
- control directly calls Mario::update and sets several fields, skipping the original power-up/demo/transform logic and control2/controlMain chain. Restoring only Mario::update does not restore that actor control chain.

## Provider closure

`native-provider-closure.json` records LLVM IR references for each method and a timestamped archive-symbol availability snapshot. Immediate missing init2 providers are setupSensors, initMorphStringTable, MarioMessenger construction, five camera declaration/target helpers. control immediately needs requestPowerUpHPMeter. Its existing control2/controlMain path reaches additional original action/rush/throw, UI meter and vertical-press methods; the constructor also retains actor virtual-method dependencies. The full local-TU dependency report includes those newly reachable references and existing archive providers. This is an exact reference inventory within this TU, not proof of transitive archive link closure or runtime initialization.

No native Game edits, walking tuning, guards, root build, or decomp recovery were necessary for this bounded audit. Parent coordinates activation after restoring the Mario owner graph.
