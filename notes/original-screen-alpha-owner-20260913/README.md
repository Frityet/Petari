# Original scene screen-alpha owner

The thirteenth actual `--original` Gateway run reached GameScene initialization after Logo retirement, then failed because the old screen-alpha implementation required RuntimeContext.

Activated the existing original ScreenAlphaCapture source/header byte-identically, including its actual SceneObj factory. This restores five authored slots, the scene GDDR heap selection, GX_TF_I8 storage, original copy-filter restoration and texture-cache reset. Deleted the four-slot lazy RuntimeContext service and its eager process-global creation. The existing SDK JUTTexture heap finalizer releases GX/mapped backing before the owning original heap retires; no second Game texture owner is introduced.

The obsolete RuntimeContext service-publication assertion was removed from its existing construction fixture; its actual context, heap, texture, and NameObj retirement assertions remain. No component test campaign was run. Production build/run receipts are under ../gateway-wakeup-demo-20260912/.

Twenty-first production build passed. Fourteenth real-disc run passed original ScreenAlphaCapture creation and reached StageFileLoader's archive receive. The next failure is the old mountAsyncArchive path bypassing FileLoader, not a screen texture failure. Exact crash stack: ../gateway-wakeup-demo-20260912/original-app-fourteenth-backtrace.log.
