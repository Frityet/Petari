# Mapped capture texture and runtime lifetime validation

The mapped owned-JUTTexture service and RuntimeContext capture owners are activated. The original JUTTexture constructor assignments and CaptureScreenDirector remain in use. The runtime registration and callback scheduler now survive dependent destruction, including constructor failure; owned mapped storage is retired before graphics teardown. Detailed design, source comparisons, and reproducible ModelX preparation are in `../original-modelx-native-20260903/README.md`.

The Aurora gitlink in this checkpoint is `df4e279178a58fc8ba2d9c7a90c447a6c51d5d3e`: viewport/scissor state can be written and drained before a frame recording exists. A separate follow-up covers render-uniform replay when the next target changes size.

Validation on macOS ARM64 with LLVM 23:

- `smg-pc-jut-texture-ownership-tests`: three groups passed, including mapped ownership, borrowed resources, and retirement.
- `smg-pc-runtime-context-construction-tests`: three groups passed with the real RMGK01 disc. This includes mapped OOM before the first frame, an ILogger exception after both capture callbacks register, and two successful reconstructions. Globals, callbacks, capture ownership, and MEM1 capacity recover.
- `smg-pc-showcase` rebuilt successfully. Real-disc title smoke passed two rendered frames and Gateway smoke passed five rendered frames, including real animated model packet submission and planet KCL contact.
- `smg-pc-original-resource-holder-tests`: four groups passed with the real Mario archive (nine materials, thirty joints).

Build/run transcripts remain local in this directory. The smoke's `--max-frames 600` is only a safety cap; the explicit smoke criteria terminate after two/five rendered frames. These checks do not establish full original actor animation activation, working jumping, or the bunny-chase demo. Original ModelManager/ModelX/DrawBuffer integration is still staged.
