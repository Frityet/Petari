0000: stwu r1, -0x60(r1)
0004: mflr r0
0008: stw r0, 0x64(r1)
000c: addi r11, r1, 0x60
0010: bl _savegpr_23
0014: lwz r31, 0x14(r4)
0018: mr r23, r3
001c: addi r30, r4, 0x14
0020: lis r25, lbl_805B6E02@ha
0024: lis r26, lbl_805B6DE0@ha
0028: lis r27, lbl_805D65BC@ha
002c: lis r28, lbl_805D659A@ha
0030: li r29, 0x6
0034: b 0xb4
0038: cmpwi r31, 0x0
003c: bne 0x54
0040: addi r3, r25, lbl_805B6E02@l
0044: addi r5, r26, lbl_805B6DE0@l
0048: li r4, 0x23d
004c: crclr cr1eq
0050: bl Panic__Q24nw4r2dbFPCciPCce
0054: subic. r24, r31, 0x4
0058: bne 0x70
005c: addi r3, r27, lbl_805D65BC@l
0060: addi r5, r28, lbl_805D659A@l
0064: li r4, 0x193
0068: crclr cr1eq
006c: bl Panic__Q24nw4r2dbFPCciPCce
0070: lwz r3, 0xc(r24)
0074: addi r4, r24, 0x54
0078: addi r5, r1, 0x8
007c: addi r3, r3, 0x84
0080: bl PSMTXConcat
0084: addi r5, r24, 0x80
0088: addi r4, r1, 0x4
008c: mtctr r29
0090: lwz r3, 0x4(r4)
0094: lwzu r0, 0x8(r4)
0098: stw r3, 0x4(r5)
009c: stwu r0, 0x8(r5)
00a0: bdnz 0x90
00a4: mr r3, r23
00a8: mr r4, r24
00ac: bl recalcChildGlobalMtx__14LayoutPaneCtrlFPQ34nw4r3lyt4Pane
00b0: lwz r31, 0x0(r31)
00b4: cmplw r31, r30
00b8: bne 0x38
00bc: addi r11, r1, 0x60
00c0: bl _restgpr_23
00c4: lwz r0, 0x64(r1)
00c8: mtlr r0
00cc: addi r1, r1, 0x60
00d0: blr 

REFERENCES
{'offset': '0x22', 'kind': 6, 'symbol': 'lbl_805B6E02', 'addend': 0}
{'offset': '0x26', 'kind': 6, 'symbol': 'lbl_805B6DE0', 'addend': 0}
{'offset': '0x2a', 'kind': 6, 'symbol': 'lbl_805D65BC', 'addend': 0}
{'offset': '0x2e', 'kind': 6, 'symbol': 'lbl_805D659A', 'addend': 0}
{'offset': '0x42', 'kind': 4, 'symbol': 'lbl_805B6E02', 'addend': 0}
{'offset': '0x46', 'kind': 4, 'symbol': 'lbl_805B6DE0', 'addend': 0}
{'offset': '0x5e', 'kind': 4, 'symbol': 'lbl_805D65BC', 'addend': 0}
{'offset': '0x62', 'kind': 4, 'symbol': 'lbl_805D659A', 'addend': 0}
