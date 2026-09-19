# Live recorded controller replay

`SMGPC_DEBUG_WPAD_INPUT_FILE` is an opt-in debug-only input source shared by the
original application and RuntimeContext. The file contains exactly three JSON
string fields: `buttons`, `pointer`, and `stick`, using the existing inclusive
frame-span syntax. Empty strings disable that scripted channel. External tools
must atomically replace the complete file. No actor, scene, or story fields are
accepted.

Each actual frame reads this explicitly configured file, with a 64 KiB limit.
Changed, valid content is parsed as a complete replacement before publication.
Every accepted revision logs its exact frame, revision number and all effective
scripts. Missing files, incomplete/invalid JSON, unknown fields, nonstring fields
and invalid scripts fail explicitly. An absent environment variable performs no
file access. Physical buttons remain ORed with scripted buttons; the live source
is applied after the fixed environment scripts, so its active pointer/stick
spans take precedence. Disabling a live span returns to ordinary physical/fixed
input at the next original sample; no synthetic held state is retained.

The shared parser now rejects malformed button and pointer entries just as it
already rejected malformed stick entries. Unknown buttons and nonfinite pointer
coordinates can no longer silently disappear or propagate invalid values. Valid
existing syntax and normal KPAD/WPad hold/trigger/release behavior are unchanged.

FrameButtonStateTests adds atomic replacement, explicit neutral stick, physical
input preservation, revision identity and failed-update rollback coverage. The
existing actual WPad edge tests remain in the same suite. Build/runtime results
are recorded below. A completed debug replay must not be reported as a physical
keyboard-input test.

Validation: the shared FrameButtonStateTests suite passes, including actual WPad
hold/trigger/release and live file atomic replacement, unchanged revision,
neutral stick, all three script parsers, rejected schema/oversize/missing-file
updates, and rollback. `frame-button-tests.log.gz` preserves that run. The
original application and two original-process probes linked successfully in the
serialized shared build (9.372 seconds; `main-shared-build.log.gz`).

The read-only original-process trace additionally captures Mario's current
movement-up vector and actual camera position/basis. External diagnostic drivers
can derive controller directions without invoking mutation-bearing movement
methods or changing actors. The runner now accepts the explicit live input file
and clears any inherited NAND import override when creating its recorded fresh
save directory. A 12,000-frame controller-only chase is running separately; its
result is not asserted by this compilation and focused-test checkpoint.
