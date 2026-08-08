# Post-merge Game source mirror sync

## Outcome

The existing PC mirrors for FileSelectInfo and the Player/Sequence/System utility boundary are byte-identical to the resolved root decomp sources. The exact Game translation units remain excluded from the host build; real host services continue to enter through compatibility providers.

The upstream `PlayerUtil.hpp` declaration now uses `NO_INLINE`. Because it already includes Aurora's low-level `revolution/types.h`, the guarded host definition was generalized at that Aurora boundary instead of changing the exact Game header or force-including a broad PC-only header into consumers.

Aurora commit: `4a21d57c0012943aca1c895c4fd88dc20a478c0c` (`Expose NO_INLINE from Revolution types`).

## Exactness evidence

Each row passed `cmp -s`; the digest is shared by both files.

| SHA-256 | Root decomp | PC mirror |
| --- | --- | --- |
| `0b9cdf2db96c25b984920761d5c4c9fa87d14acf217f50997fc012449a2ef82c` | `include/Game/Screen/FileSelectInfo.hpp` | `pc-port/src/Game/Screen/FileSelectInfo.hpp` |
| `a96cd9ff44b160c7c1ff3ef5b16dd56d496d4d439e2d8d81dfc52db1a830e622` | `src/Game/Screen/FileSelectInfo.cpp` | `pc-port/src/Game/Screen/FileSelectInfo.cpp` |
| `9d621ed9d3a4407a38709a5ec1141b02d30a40e1b0614009b3b039a21a4e4ee1` | `include/Game/Util/PlayerUtil.hpp` | `pc-port/src/Game/Util/PlayerUtil.hpp` |
| `d4af270f76a553682fe2e601b76c612fd267e96db9de8427850fdef0b9f05fe9` | `src/Game/Util/PlayerUtil.cpp` | `pc-port/src/Game/Util/PlayerUtil.cpp` |
| `5528893121755f41670af6729884edd8bc15d1b70fba34f828dbcf7c263043c6` | `include/Game/Util/SequenceUtil.hpp` | `pc-port/src/Game/Util/SequenceUtil.hpp` |
| `e25e94f7c0648c3a3ab8b61521899fbb0c412bb53943f88cba814d5a34a9741c` | `src/Game/Util/SequenceUtil.cpp` | `pc-port/src/Game/Util/SequenceUtil.cpp` |
| `ae84fb160f3fabce7a2f4c616d6845ab93b1e718855d05c2248a828991c3216d` | `include/Game/Util/SystemUtil.hpp` | `pc-port/src/Game/Util/SystemUtil.hpp` |
| `9b8972121c04a5e7117163928d184dd1c23f6984a8e69a5d48d832d1350ceefa` | `src/Game/Util/SystemUtil.cpp` | `pc-port/src/Game/Util/SystemUtil.cpp` |

## Focused verification

From `pc-port/`:

```text
$ xmake build smg-pc-game-source-mirror-tests
build ok
$ xmake run smg-pc-game-source-mirror-tests
[ok] src/Game/Screen/FileSelectInfo.hpp
[ok] src/Game/Screen/FileSelectInfo.cpp
[ok] src/Game/Util/PlayerUtil.hpp
[ok] src/Game/Util/PlayerUtil.cpp
[ok] src/Game/Util/SequenceUtil.hpp
[ok] src/Game/Util/SequenceUtil.cpp
[ok] src/Game/Util/SystemUtil.hpp
[ok] src/Game/Util/SystemUtil.cpp

$ xmake build smg-pc-player-util-real-or-absent-tests
build ok
$ xmake run smg-pc-player-util-real-or-absent-tests
Player real-or-absent tests passed: 4/4
```

`git diff --check` also passed. The existing xmake exclusions for FileSelectInfo.cpp, PlayerUtil.cpp, SequenceUtil.cpp, and SystemUtil.cpp were retained. Protected SaveIcon and TriggerChecker working-tree changes were not touched.
