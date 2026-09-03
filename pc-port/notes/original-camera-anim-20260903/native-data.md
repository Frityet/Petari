# Native CANM/CKAN ownership

`CameraAnimation::from_bytes` now validates disc-format input and constructs an
owned native representation for unchanged `CameraAnim::setParam`, `loadBin`, and
the original linear/key accessors. This change is limited to
`src/camera/CameraAnimation.{hpp,cpp}`; controller integration and regression
tests are owned by the coordinating task.

## Ownership and representation

`CameraAnimation::native_data()` returns a copyable `NativeCameraAnimationData`
handle. Its `bytes()` method exposes `span<const uint8_t>`, while the handle
retains shared ownership independently of the source animation or archive.
The original non-const pointer signatures require a narrow `const_cast` at the
caller: the inspected `loadBin`, `set`, and sampling bodies only read the block.
A default or moved-from owner can be empty; the controller must reject that
before calling `loadBin`.

The allocation is aligned to `max_align_t`. It contains actual typed
`CanmFileHeader`, `CanmFrameInfo`/`CanmKeyFrameInfo`, value-byte-count, and a float
array. Placement construction establishes those lifetimes; the float values
form one array for the original pointer arithmetic. All these types are trivial,
so the storage owner can retire the raw allocation directly.

Conversion preserves:

- Original block size, tags, table locations, opaque gaps, and trailing bytes.
- Every header word, including `_8`, `_C`, `_10`, `_14`, frame count, and value
  offset. Signed opaque fields retain their original bit patterns.
- Every component's count, float offset, and CKAN type, without deduplication or
  rewritten counts.
- Value byte count and all finite float values, including unused entries and
  signed zero.

Only the known numeric fields change byte order. Type zero selects three-float
keys; **every nonzero type selects four-float keys**, exactly as the original
`KeyCamAnmDataAccessor::get` does. Constant components consume one float even
when their type indicates a keyed layout.

## Reader preconditions and lookahead

Input must have an ANDO/CANM or ANDO/CKAN header, nonzero version and frame count,
aligned non-overlapping component/value tables, and sufficient finite values for
the original reads. The zero-frame case is rejected because `CameraAnim::calc`
still evaluates twist/FOV at unsigned `mNrFrames - 1`. CANM also rejects an end
frame which rounds outside the original `u32` frame-conversion range.

The relevant root and RMGK01 rev0 readers are:

| Reader | Address | Behavior |
| --- | --- | --- |
| `searchKeyFrameIndex` | `0x800945d4` | Upper-bound search, then returns `low - 1` |
| `get3f` | `0x80094630` | Reads current key and the following three values |
| `get4f` | `0x800946b4` | Reads current outgoing tangent and following time/value/incoming tangent |

The DOL confirms these bodies and their unconditional following-record reads.
There is no lower or upper clamp in the original keyed accessor.

For a multi-key component, the first key must be at or before frame zero and the
searched key times must be nondecreasing. Duplicate times are supported: the
upper-bound search chooses the last matching key, so the next searched key has
a greater time. This preserves authored discontinuities.

If the last searched key can be selected during playback, validation requires
three readable values at `component.offset + component.count * stride`. Those
values may be a real lookahead record outside the declared search count, in the
same global value table. The record's time must exceed the last searched time.
It is never inserted into the search or counted as another key. When playback
passes that lookahead time, the original Hermite routine extrapolates using the
same final segment; validation does not add a time clamp. A type-four terminal
record used only as lookahead needs three values, because its outgoing tangent
is not read.

Reachability covers every active frame below `float(mNrFrames)` and the final
twist/FOV query at `float(mNrFrames - 1)`. The latter is checked independently
because large integer frame counts can round to the same float. Positive finite
playback speed and phase scheduling remain the controller owner's responsibility.

`CameraAnimation::sample` retains its small diagnostic API but dispatches to the
actual `CamAnmDataAccessor` or `KeyCamAnmDataAccessor` against the retained native
block. It requires a finite frame in `[0, float(frame_count))`. The host Hermite,
linear interpolation, endpoint clamps, and duplicate component/value storage
have been removed.

## Real resource evidence

A small standalone inspector used the installed `encounter-nod` C API and the
existing `RarcArchive`/Yaz0 code to read the supplied RMGK01 rev0 disc. It extracted
`ObjectData/TicoBaby.arc:/demomeettico.canm` into ignored
`build/compat-camera-animation/DemoMeetTico.canm` without a full disc conversion.
The inspector source and executable are in that same ignored directory.

The resource is 3196 bytes. Its SHA-256 is
`750aa371bfd4af394cda8886da8f76e5bab324ed44c1e99bbc75c879bf66ae62`.
The six header words beginning at `0x08` are
`1, 0, 1, 4, 1199, 96`. Its global value table contains 765 floats.

| Component | Count | Float offset | Type | First key | Last searched key |
| --- | ---: | ---: | ---: | ---: | ---: |
| Position X | 29 | 0 | 0 | 0 | 1199 |
| Position Y | 23 | 87 | 0 | 0 | 1199 |
| Position Z | 29 | 156 | 0 | 0 | 1199 |
| Watch X | 55 | 243 | 0 | 0 | 1199 |
| Watch Y | 49 | 408 | 0 | 0 | 1199 |
| Watch Z | 57 | 555 | 0 | 0 | 1199 |
| Twist | 12 | 726 | 0 | 0 | 1199 |
| FOV | 1 | 762 | 0 | Constant | Constant |

This asset stores its terminal keys inside the search count and does not need
outside-count lookahead. That observed convention is not imposed on other
resources whose stored data satisfies the original accessor's actual reads.

Source review and `git diff --check` completed. This subtask did not run xmake or
the regression executables; the coordinating task runs the rebuilt controller
tests and real-disc checks. The standalone resource inspector was compiled and
run only to establish the authored data bounds above.
