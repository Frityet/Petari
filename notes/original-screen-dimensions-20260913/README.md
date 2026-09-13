# Original screen dimensions

Restored the missing original MR::getScreenHeight and MR::getFrameBufferHeight definitions in their reference CameraContext and BloomEffect translation units, then copied them into the native source. Each original function loads JUTVideo::sManager, its render-mode pointer and the u16 EFB height (retail addresses 800979DC and 8034B8E4). The existing original MarioActor definition supplies framebuffer width in the same way.

Deleted ScreenUtilCompat.cpp, including its RuntimeContext dependency, fixed-size fallbacks and weak duplicate width definition. The original process now uses its actual JUT video manager for every dimension query, including projection, pointer depth and effects. This does not change screen sizing policy or introduce physical-window pixels into the Wii logical framebuffer.

The next production build checks integration with the full original owner graph. No new component tests are needed for these direct original accessors. CameraContext projection math is unchanged.
