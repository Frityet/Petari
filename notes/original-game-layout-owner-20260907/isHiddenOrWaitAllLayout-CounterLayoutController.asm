0000: stwu r1, -0x20(r1)
0004: mflr r0
0008: stw r0, 0x24(r1)
000c: addi r11, r1, 0x20
0010: bl _savegpr_26
0014: lwz r30, 0x28(r3)
0018: mr r26, r3
001c: li r29, 0x0
0020: li r28, 0x0
0024: mr r3, r30
0028: li r27, 0x0
002c: li r31, 0x0
0030: bl isDead__2MRFPC11LayoutActor
0034: cmpwi r3, 0x0
0038: bne 0x5c
003c: mr r3, r30
0040: bl isHiddenLayout__2MRFPC11LayoutActor
0044: cmpwi r3, 0x0
0048: bne 0x5c
004c: mr r3, r30
0050: bl isWait__11CoinCounterCFv
0054: cmpwi r3, 0x0
0058: beq 0x60
005c: li r31, 0x1
0060: cmpwi r31, 0x0
0064: beq 0xb0
0068: lwz r30, 0x2c(r26)
006c: li r31, 0x0
0070: mr r3, r30
0074: bl isDead__2MRFPC11LayoutActor
0078: cmpwi r3, 0x0
007c: bne 0xa0
0080: mr r3, r30
0084: bl isHiddenLayout__2MRFPC11LayoutActor
0088: cmpwi r3, 0x0
008c: bne 0xa0
0090: mr r3, r30
0094: bl isWait__16StarPieceCounterCFv
0098: cmpwi r3, 0x0
009c: beq 0xa4
00a0: li r31, 0x1
00a4: cmpwi r31, 0x0
00a8: beq 0xb0
00ac: li r27, 0x1
00b0: cmpwi r27, 0x0
00b4: beq 0x100
00b8: lwz r30, 0x30(r26)
00bc: li r31, 0x0
00c0: mr r3, r30
00c4: bl isDead__2MRFPC11LayoutActor
00c8: cmpwi r3, 0x0
00cc: bne 0xf0
00d0: mr r3, r30
00d4: bl isHiddenLayout__2MRFPC11LayoutActor
00d8: cmpwi r3, 0x0
00dc: bne 0xf0
00e0: mr r3, r30
00e4: bl isWait__10PlayerLeftCFv
00e8: cmpwi r3, 0x0
00ec: beq 0xf4
00f0: li r31, 0x1
00f4: cmpwi r31, 0x0
00f8: beq 0x100
00fc: li r28, 0x1
0100: cmpwi r28, 0x0
0104: beq 0x150
0108: lwz r30, 0x34(r26)
010c: li r31, 0x0
0110: mr r3, r30
0114: bl isDead__2MRFPC11LayoutActor
0118: cmpwi r3, 0x0
011c: bne 0x140
0120: mr r3, r30
0124: bl isHiddenLayout__2MRFPC11LayoutActor
0128: cmpwi r3, 0x0
012c: bne 0x140
0130: mr r3, r30
0134: bl isWait__11StarCounterCFv
0138: cmpwi r3, 0x0
013c: beq 0x144
0140: li r31, 0x1
0144: cmpwi r31, 0x0
0148: beq 0x150
014c: li r29, 0x1
0150: addi r11, r1, 0x20
0154: mr r3, r29
0158: bl _restgpr_26
015c: lwz r0, 0x24(r1)
0160: mtlr r0
0164: addi r1, r1, 0x20
0168: blr 

REFERENCES
