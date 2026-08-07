# Talk and demo real-or-absent cleanup

## Removed fabricated success

- `TalkMessageCtrl` no longer accepts normal or forced talk requests when no `TalkDirector`/message-node runtime exists.
- Talk start/end and node traversal now fail explicitly instead of mutating a private host-only state machine.
- Missing placement message data no longer aliases message ID zero.
- Non-time-keep demo start now succeeds only through the active, scene-owned `DemoSceneRuntime` and a real named definition.
- Void time-keep starts without a scene demo runtime now fail explicitly instead of creating process-global programmable demo state.
- Demo end still follows the retail `DemoDirector::endDemo` behavior of ending the active demo irrespective of the informational owner/name arguments.

## Verification

Run from `pc-port/`:

```text
xmake build smg-pc-talk-real-or-absent-tests
xmake run smg-pc-talk-real-or-absent-tests
Talk real-or-absent tests passed: 3/3

xmake build smg-pc-stage-player-runtime-tests
xmake run smg-pc-stage-player-runtime-tests
8 stage-player runtime test(s) passed

xmake build smg-pc-demo-scene-runtime-tests
xmake run smg-pc-demo-scene-runtime-tests
16/16 tests passed
```

The scene-owned DemoSheet path remains operational. Only the former no-director/no-definition success paths were removed.
