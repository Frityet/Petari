# RMGK02 archive and actor-name corrections

Date: 2026-08-07

## Scope

This note records two literal-only corrections in the root decompilation:

- `StageDataHolder::initTableData()` requests the real object-name archive.
- `RosettaDemoHeavensDoor1` gives its two parts the original Japanese runtime
  names while retaining the English model/archive names.

No PC-only actor was added and no compatibility behavior was changed.

## Target provenance

The checked RMGK02 executable is the configured revision:

```text
54b71431af0d509097bfdef4ec28617afc487e89  orig/RMGK02/sys/main.dol
```

That SHA-1 is identical to `config/RMGK02/config.yml`.

## `ObjNameTable` archive evidence

The original split assembly for `StageDataHolder::initTableData()` loads
`lbl_805D26AA` before calling `MR::receiveArchive`:

```text
803483E8  lis   r4, lbl_805D26AA@ha
803483FC  addi  r3, r4, lbl_805D26AA@l
80348400  bl    receiveArchive__2MRFPCc
...
805D26AA  "/StageData/ObjNameTable.arc"
805D26C6  "ObjNameTable.tbl"
```

Sources: `build/RMGK02/asm/Game/Scene/StageDataHolder.s:1855-1889` and
`:2742-2751`. The same strings occur in the original DOL at file offsets
`0x5CE7AA` and `0x5CE7C6` (decimal 6088618 and 6088646).

The extracted disc also supplies exactly that archive and resource:

```text
orig/RMGK02/files/StageData/ObjNameTable.arc  38656 bytes
RARC entry: objnametable.tbl                  77120 bytes
orig/RMGK02/files/StageData/ObjTableTable.arc absent
```

Therefore the former `/StageData/ObjTableTable.arc` literal was a decomp typo,
not an alternate resource name.

## Rosetta part-name evidence

The original constructor uses four distinct string offsets for the two calls:

```text
80283EF4  addi r4, r31, 0x70
80283EF8  addi r5, r31, 0x7D
80283F00  bl createPartsModelNpc__2MRFP9LiveActorPCcPCcPA4_f

80283FB4  addi r4, r31, 0x95
80283FB8  addi r5, r31, 0xA0
80283FC0  bl createPartsModelNpc__2MRFP9LiveActorPCcPCcPA4_f
```

`r31` is `@60347` at `0x805B2BD0`. Decoding the target's CP932 data gives:

| Offset | Target bytes | Decoded value | Role |
|---:|---|---|---|
| `+0x70` | `83 89 83 43 83 67 83 68 81 5B 83 80 00` | `ライトドーム` | runtime actor name |
| `+0x7D` | ASCII | `LightDome` | model/archive name |
| `+0x95` | `83 89 83 43 83 67 8C E3 8C F5 00` | `ライト後光` | runtime actor name |
| `+0xA0` | ASCII | `DomeHalo` | model/archive name |

Sources: `build/RMGK02/asm/Game/NPC/RosettaDemoHeavensDoor.s:47-51`,
`:114-118`, and `:1010-1038`. The contiguous original bytes are also visible
in `orig/RMGK02/sys/main.dol` at file offset `0x5AED40`:

```text
005aed40: 83 89 83 43 83 67 83 68 81 5b 83 80 00 4c 69 67
005aed50: 68 74 44 6f 6d 65 00 43 65 6e 74 65 72 00 41 70
005aed60: 70 65 61 72 00 83 89 83 43 83 67 8c e3 8c f5 00
005aed70: 44 6f 6d 65 48 61 6c 6f
```

## Verification

RMGK02 configuration and the affected `StageDataHolder.o` target build
normally:

```text
python3 configure.py --version RMGK02              PASS
ninja build/RMGK02/src/Game/Scene/StageDataHolder.o
                                                     PASS
StageDataHolder::initTableData machine code: byte-identical (0x80 bytes)
```

The normal direct Rosetta target fails on an unrelated, pre-existing
translation-unit issue:

```text
ninja build/RMGK02/src/Game/NPC/RosettaDemoHeavensDoor.o
src/Game/NPC/RosettaDemoHeavensDoor.cpp:12: undefined identifier 'NEW_NERVE'
                                                     FAIL (exit 1)
```

To verify this literal change without expanding the patch, the exact RMGK02
MWCC command was rerun with `-include Game/LiveActor/Nerve.hpp`; it compiled
successfully. Against the extracted target object, objdiff constructor
similarity improved from `93.73972%` for the existing stale object to
`97.75799%` for the corrected temporary object.

Full authored-source status is separately not clean. `ninja -k 0` exits 1 with
17 pre-existing source compilation failures, including `DemoDirector`,
`DemoTimeKeeper`, `DemoExecutor`, `CameraDirector`, several Mario units,
`RunawayRabbit`, and the Rosetta TU above. The build nevertheless links the
available objects and reports `build/RMGK02/main.dol: OK`; that successful hash
check does not mean every authored source target compiled.

```text
0988110454414fbc954ee00f9f4c6b42076af9d209c7d8a51dd3dc14e1911199  src/Game/Scene/StageDataHolder.cpp
8a3e3d93d2cf2670a4de08d3bd0b99a83cfd7a1c6232f6d3a924d96cdbf51a58  src/Game/NPC/RosettaDemoHeavensDoor.cpp
fa8a87d5edd65a19a9fbd1e935c679b5ba1725b676f9fae92e4149c0bc8c6edf  build/RMGK02/src/Game/Scene/StageDataHolder.o
```

`git diff --check` passes for both edited sources.
