# Live UI and controller provenance

The game ran once, from a fresh save, using the current signed application bundle. Bundle UUID 401E42BD-9488-3014-9A5A-882EAA74A61C matched the standalone executable before launch. The runner records the bundle SHA and verifies it did not change during execution. No rebuild occurred until the game was terminal.

`supervise.py` ran the existing three-rabbit operator, then `climb_tower.py` using geometry-derived ordinary stick waypoints and A presses for risers/dialogue. The climber was terminated after Rosalina disappeared and normal control returned; `climb-stop.json` records the actual PID and controller-writer handoff. This was a notes-only process termination, not game state modification. Subsequent `follow_actor.py` commands selected current-run CrystalCage 1013 and1016 positions through read-only traces.

Computer-use actions used the installed skill through node_repl and @oai/sky. Screenshots and normal app-targeted keyboard inputs were used; no other GUI-control mechanism or memory writes were used. The inspected skill was `/Users/frityet/.codex/plugins/cache/openai-bundled/computer-use/1.0.1001093/skills/computer-use/SKILL.md`.

- A normal X press during the Luma crystal-instruction conversation did not break a crystal. Return/Space/focus attempts did not visibly advance that prompt by themselves. This does not establish a keyboard or gameplay defect.
- Explicit ordinary A spans at29460–29490 and29590–29620 closed the prompt; accepted controller-file revisions are in `route.log` and `crystal-prompt-input.json`.
- After walking to the crystal, a normal X key press caused Cage 1013's Break state at 33200, then death. `crystal-x-ui.jpg` was captured immediately afterward and shows shards.
- The next approach targeted 1016. A second normal X press near the end of the run broke the remaining cluster and visually revealed the orange launch star. The exact actor transitions are in the trace/route evidence. Its UI image appeared in the conversation, but the temporary screenshot was removed when the game exited before it could be copied into notes; there is no committed screenshot artifact for that reveal.

The game stopped normally at36000 frames. The launch ride, later planets and Grand Star were not reached. All source fixes in this task were made while the baseline binary ran and built only afterward, so the route proves the preceding checkpoint, not the new fixes.
