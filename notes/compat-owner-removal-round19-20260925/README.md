# Delete all remaining scene sidecars

Actual Scene owns its native execution, effect initialization and retained heap lifecycle. Actual GameScene and its sequence/control children own native cleanup. The controller already supplies real initialization states, so the unused replacement scope API and its two obsolete fixture targets are deleted. All eight remaining src/scene files and both build globs are removed; the directory no longer exists.

The unused alternate factory catalog generator is also deleted now that the original NameObjFactory table is compiled in full. Source audit no longer classifies the deleted scene path. SceneScheduler::clear retains callback clearing through the actual category owner.

Validation: one integrated build passed in 6.228 seconds; a single fresh-save Gateway opening completed 120 frames and exited 0 in 3.130 seconds, with no remaining process. Binary SHA-256: 0c95eaf9c2ed3226754780e61414952ad8abcf4da117c0e2c3e87a7eb45db120. No fixture suites or gameplay-route tests were run. This does not establish the full wakeup-to-Rosalina demo.

Remaining: 37 compat files. Further compatibility work must remove sidecars through actual owners and general SDK implementations. Existing unrelated staged route notes and editor/history changes are preserved.
