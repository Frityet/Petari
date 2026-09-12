# Original pause menu startup owner

Restored missing `PauseMenu::exeSelecting` and `exeDecided` in the reference from retail `Game/Screen/PauseMenu.s` addresses 0x80375F54–0x80376354, then imported the complete PauseMenu TU unchanged. The handlers retain pointer selection priority, optional/hidden letter guards, animation gating of Plus/Minus dismissal, selected-button sound timing, save/confirmation transitions, and the Luigi-letter branch.

Imported the complete existing original LuigiLetter source/header as PauseMenu's absent direct class dependency. ButtonPaneController, SysInfoWindow and IconAButton already exist. Named sound requests continue through the existing disabled object-audio boundary; no fake audio members or extra per-menu policy were added. This is the normal in-game pause menu, separate from the removed Wii HOME menu.

Both native TUs compile; the recovered PauseMenu also compiles with the original Wii compiler. Receipts: native-compile.json and reference-compile.json. No component tests or shared builds were run. Root owns the next actual startup build/run. Source manifest lists the four owned paths; native PauseMenu/LuigiLetter copies are byte-identical to the reference.
