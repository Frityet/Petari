# Talk queries: real or explicitly absent

The partial Talk runtime has no real `TalkDirector`, `TalkNodeCtrl` tree, or
scene-owned `MessageArea` data. Its request methods previously returned
`false`, which was indistinguishable from a real director declining a valid
request.

`TalkMessageCtrl::requestTalk()`, `requestTalkForce()`, and `inMessageArea()`
now reject explicitly while those owners are absent. Consequently the `tryTalk`
helpers also expose the missing runtime instead of silently reporting an
ordinary negative result. Null helper arguments retain their normal defensive
`false` result; a constructed control with missing required owners does not.

Verification on 2026-08-07:

```text
Talk real-or-absent tests passed: 3/3
```
