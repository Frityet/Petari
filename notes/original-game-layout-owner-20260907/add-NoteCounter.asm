0000: stwu r1, -0x10(r1)
0004: mflr r0
0008: stw r0, 0x14(r1)
000c: stw r31, 0xc(r1)
0010: stw r30, 0x8(r1)
0014: mr r30, r3
0018: lwz r0, 0x2c(r3)
001c: cmpwi r0, -0x2
0020: bne 0x6c
0024: lwz r4, 0x24(r3)
0028: subi r0, r4, 0x5c
002c: srwi r0, r0, 2
0030: mtctr r0
0034: cmpwi r4, 0x60
0038: blt 0x44
003c: subi r4, r4, 0x4
0040: bdnz 0x3c
0044: srawi r0, r4, 2
0048: lis r3, lbl_805D79F8@ha
004c: addze r6, r0
0050: li r5, -0x1
0054: slwi r0, r6, 2
0058: addi r3, r3, lbl_805D79F8@l
005c: subf r0, r0, r4
0060: add r4, r6, r0
0064: bl startSystemSE__2MRFPCcll
0068: b 0xc8
006c: cmpwi r0, -0x1
0070: bne 0xb0
0074: lwz r3, 0x24(r3)
0078: li r0, 0xc
007c: addi r4, r3, 0x5
0080: subi r3, r4, 0x19
0084: divwu r3, r3, r0
0088: mtctr r3
008c: cmpwi r4, 0x24
0090: ble 0x9c
0094: subi r4, r4, 0xc
0098: bdnz 0x94
009c: lis r3, lbl_805D79F8@ha
00a0: li r5, -0x1
00a4: addi r3, r3, lbl_805D79F8@l
00a8: bl startSystemSE__2MRFPCcll
00ac: b 0xc8
00b0: cmpwi r0, 0x0
00b4: blt 0xc8
00b8: lwz r4, 0x24(r30)
00bc: mr r3, r0
00c0: lfs f1, 0x30(r30)
00c4: bl startRemixSound__2MRFllf
00c8: lwz r3, 0x24(r30)
00cc: lwz r0, 0x20(r30)
00d0: addi r3, r3, 0x1
00d4: cmpw r3, r0
00d8: stw r3, 0x24(r30)
00dc: blt 0x10c
00e0: bl getMessageSensor__2MRFv
00e4: mr r31, r3
00e8: bl getMessageSensor__2MRFv
00ec: mr r5, r3
00f0: lwz r3, 0x34(r30)
00f4: mr r6, r31
00f8: li r4, 0x66
00fc: lwz r12, 0x0(r3)
0100: lwz r12, 0x34(r12)
0104: mtctr r12
0108: bctrl 
010c: mr r3, r30
0110: bl tryEndDisp__11NoteCounterFv
0114: lwz r0, 0x14(r1)
0118: lwz r31, 0xc(r1)
011c: lwz r30, 0x8(r1)
0120: mtlr r0
0124: addi r1, r1, 0x10
0128: blr 

REFERENCES
{'offset': '0x4a', 'kind': 6, 'symbol': 'lbl_805D79F8', 'addend': 0, 'value_hex': '53455f53595f464c4f5745525f4745545f434f4d424f00'}
{'offset': '0x5a', 'kind': 4, 'symbol': 'lbl_805D79F8', 'addend': 0, 'value_hex': '53455f53595f464c4f5745525f4745545f434f4d424f00'}
{'offset': '0x9e', 'kind': 6, 'symbol': 'lbl_805D79F8', 'addend': 0, 'value_hex': '53455f53595f464c4f5745525f4745545f434f4d424f00'}
{'offset': '0xa6', 'kind': 4, 'symbol': 'lbl_805D79F8', 'addend': 0, 'value_hex': '53455f53595f464c4f5745525f4745545f434f4d424f00'}
