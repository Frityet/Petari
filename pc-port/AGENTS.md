# Super Mario Galaxy PC-Port

This is the pc-port of the decompilation of Super Mario Galaxy. The decompilation is located at the root of this repo.

## Goal

The decompilation is split into multiple "Modules", the actual game sitting in the Game/ module. Our goal and idea is to provide and reimplement as much of the regular Wii APIs and pipelines as possible so that the `Game/` code can be recompiled and ran as unmodified as possible. We are essentially creating as large and extensive as possible Wii compatability layer to make this work, of which we use Aurora (and we have our own fork of aurora which we will continually update and adapt for what we need).

## Restrictions

1. **WE WANT TO MODIFY `Game/` AS LITTLE AS POSSIBLE!!! ONLY MODIFICATIONS ALLOWED ARE TO ADD IN (through #if !defined(NDEBUG)) ARE FOR DEBUGGING!!!**
2. We do not want code in compat to create workarounds/"easy solutions" for specific parts of the game, instead we need to make general expansions to our compatability layer (in aurora) so that the stuff works without direct specific code
3. Only the stuff in `Game/` uses the clang format spec in `Game/.clang-format`; the rest of the code uses the repo infrastructure style.

## Design decisions

- We want to move as much of the "compatability layer" stuff into Aurora as possible.
- WE DO NOT CARE ABOUT API/ABI STABILITY. DO NOT UNDER ANY CIRCUMSTANCES KEEP AROUND OLD PATHS.
- We are only caring about KB+M for now
- Audio can be ignored for now

## Resources

- You have access to up to 16 subagents.
- The source code for Dolphin is in `dolphin`. This build of dolphin has been modified in order to help with comparison and debugging of our port. We also can use the dolphin src for reference and research
- The source code for Aurora is in `aurora`
- We are able to decompile more of the games code if needed, provided you MUST adhere to these rules STRICTLY:
    - New decompiled code must be put into `../src/` and then copied to `pc-port`,
    - We only care about getting a functional match/high fuzzy match, not perfect match
    - DECOMPILED CODE MUST USE THE CONVENTIONS OF THE OTHER DECOMPILED CODE!!! MAKE SURE IT LOOKS LIKE OTHER DECOMPILED CODE.
- xvfb is installed

## Operation instructions

As you work, add in notes in `pc-port/notes/<task>-<timestamp>/`. inside you can put WHATEVER you want, but most importantly we want what changed, why, and supporting evidence and details including dumps, images, etc. You may refer to past notes to see things, however note that past notes may not be representitive of the current status of the repo.
