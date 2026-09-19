# Preserve native keyboard and mouse samples

The original Gateway demo reaches its first rabbit conversation after the
output-control work, but short Return presses during the live run did not
advance it. A source audit found that `AuroraWindow::Impl::process_sdl_event`
stored only one Boolean per logical action: a down followed by up in the same
`aurora_update` event batch left the action false before the Game sample. Return,
Space and left mouse all map to A, so releasing any one source also erased
another source's held state. Debug actions used the same lossy sampling.

`render/core/FrameButtonState.hpp` now owns two separate pieces of input state:
the held physical sources for each logical action and presses observed during
the current event poll. A short tap remains down for one sampled frame; a held
source remains down across polls. Multiple physical sources retain independent
ownership. Repeated or duplicate down events cannot create a new source/press.
The renderer uses SDL device ID plus keyboard scancode or mouse-button ID as
the source identity, with separate keyboard and mouse namespaces. Release uses
the stored source identity, so a changed keycode cannot strand the original
binding. The entire native event poll has an explicit HostAllocationScope;
retained physical-source records cannot be allocated from a transient Game heap.

The poll edge clears only the press latch. Focus loss clears both held sources
and pending presses, and the renderer ignores new down events while unfocused.
Late releases or keyboard repeats cannot restore a cleared hold. Existing game
and debug mappings are preserved. No actor, dialogue or Game input logic is
modified.

`smg-pc-frame-button-state-tests` publishes the resulting sampled masks through
the existing Aurora WPAD/KPAD service and an actual original WPad owner. It
checks the original WPadButton hold/trigger accessors and sampled KPAD release for quick key/mouse
taps, held repeats, aliases, two keyboard devices, independent A/B actions,
focus reset and changed source bindings. Debug actions use the same general
state class and have a quick-tap regression. The test does not require a window,
disc, actor or scripted gameplay input.

The native build and focused test pass (`build2.log`, `test.log`). The test
uses public original WPadButton queries without changing private Game fields or
headers. A live conversation confirmation remains pending root's serialized
runtime validation.

Validation: twelfth full native build passes. The focused input target builds and passes (test.log), using the original WPadButton public hold/trigger/change queries and actual KPAD release samples. Live LLDB input tracing also observed SDL Return down/up, published WPAD mask2048 and original KPAD trig2048. The original TalkStateEvent::talk requires a held A sample after its trigger sample; a one-frame automated tap does not establish dialogue advancement, so that remains a separate live check. No Game debounce rule was changed.
