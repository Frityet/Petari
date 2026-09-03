# Original J3D model dispatcher semantics

Recovered the two missing root `J3DModelLoaderDataBase` dispatchers in `src/JSystem/J3DGraphLoader/J3DModelLoader.cpp`. The original J3DJoint `addMesh` body was already present inline in `libs/JSystem/include/JSystem/J3DGraphAnimator/J3DJoint.hpp`; it was verified rather than replaced. No native source or build integration was changed by this task.

## Reproduction and evidence

From the repository root:

```sh
python3 pc-port/notes/original-sdk-model-dispatch-20260903/verify-source.py
```

This uses the actual GC3.0a3 compiler with the `configure.py` JSystem flags and Shift-JIS wrapper. The verified RMGK01 rev0 DOL SHA-1 is `25c5959534b3c21246c6c7e42021b916b41fb578`. The verifier performs original-object objdiff and applies real compiled relocations before checking every byte against the DOL. Commands, objects, disassembly, compiler logs, and diff output are retained under ignored `build/original-sdk-model-dispatch-20260903/`; the checked-in evidence is `source-evidence.json`.

| Recovered method | Address | Bytes | Objdiff | Relocated compiled bytes |
| --- | --- | --- | --- | --- |
| `J3DModelLoaderDataBase::loadMaterialTable` | `0x8043DDBC` | 128 | 100% | Equal retail |
| `J3DModelLoaderDataBase::loadBinaryDisplayList` | `0x8043DE3C` | 148 | 100% | Equal retail |

The two dispatchers cover 276 bytes / 69 retail instructions, with no normalization of algorithmic instructions. Both call the original v26 constructor at `0x8043F9EC` and then the original virtual loader slot on the local loader instance.

## SDK entrypoint contract

- `loadMaterialTable` returns null immediately for a null pointer. It accepts exactly the two-word header `J3D2` / `bmt3`, creates a `J3DModelLoader_v26`, and returns that instance's `loadMaterialTable` result. Every other version/type returns null. In particular, the dispatcher's acceptance is narrower than the existence of v21 material-reader methods elsewhere in the SDK.
- `loadBinaryDisplayList` likewise handles null first, accepts exactly `J3D2` / `bdl3` or `J3D2` / `bdl4`, and invokes v26 `loadBinaryDisplayList` with the original data pointer and flags unchanged. The retail compiler folds the two adjacent tags into an unsigned range comparison; the restored source expresses the same two tags.
- These methods do not examine file length, block contents, scene names, or model flags beyond forwarding the caller's flags. The original nonnull-pointer precondition includes two readable header words. A native retained resource owner can validate resource memory at its decoding boundary without broadening these dispatch predicates or treating arbitrary parsed summaries as fully constructed SDK objects.
- The existing ordinary `load` dispatcher remains unchanged: `J3D2/bmd2` selects v21 and `J3D2/bmd3` selects v26; it rejects `J3D1/bmd1` and other tags.

## Existing `J3DJoint::addMesh`

The root inline body is:

```cpp
if (mMesh != NULL) {
    pMesh->setNext(mMesh);
}
mMesh = pMesh;
```

The original `makeHierarchy` contains these five instructions at `0x804317DC` through `0x804317EC`, using `r23` as the joint and `r28` as the material:

```text
lwz   r0, 0x58(r23)
cmpwi r0, 0
beq   next_head_store
stw   r0, 0x4(r28)
stw   r28, 0x58(r23)
```

The verifier checks those exact 20 retail bytes, the current original compiler's complete 24-byte weak method (same five operations plus return), and the caller's joint/material argument identity. The current `makeHierarchy` context scores 95.73034% because this compiler emits that helper out of line. This note does not claim every unrelated hierarchy instruction has been verified here.

Insertion prepends to a nonempty list. If the joint's old head is null, the incoming material's existing next pointer is preserved. `addMesh` itself does not assign the material's joint pointer: the following original `makeHierarchy` instruction does that separately. Original material construction initializes its next pointer; native backing must preserve the real material lifecycle and should import the existing body rather than clear the chain or append to its end.

This is a root source/evidence checkpoint. The complete native retained model/material owner, SDK bridge, and runtime validation remain parent-owned integration work.
