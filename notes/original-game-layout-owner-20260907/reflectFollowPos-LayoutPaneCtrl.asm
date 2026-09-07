0000: stwu r1, -0x110(r1)
0004: mflr r0
0008: stw r0, 0x114(r1)
000c: stw r31, 0x10c(r1)
0010: mr r31, r3
0014: lwz r0, 0x18(r3)
0018: cmpwi r0, 0x0
001c: beq 0x208
0020: lwz r3, 0x4(r3)
0024: addi r4, r1, 0xd4
0028: addi r3, r3, 0x84
002c: bl PSMTXCopy
0030: lwz r0, 0x14(r31)
0034: cmpwi r0, 0x2
0038: beq 0x108
003c: bge 0x50
0040: cmpwi r0, 0x0
0044: beq 0x5c
0048: bge 0x7c
004c: b 0x1d4
0050: cmpwi r0, 0x4
0054: bge 0x1d4
0058: b 0x184
005c: lwz r4, 0x18(r31)
0060: addi r3, r1, 0x30
0064: bl convertScreenPosToLayoutPos__2MRFPQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>
0068: lfs f1, 0x30(r1)
006c: lfs f0, 0x34(r1)
0070: stfs f1, 0xe0(r1)
0074: stfs f0, 0xf0(r1)
0078: b 0x1d4
007c: lfs f0, @61176@sda21
0080: addi r3, r1, 0x28
0084: addi r4, r1, 0x18
0088: stfs f0, 0x18(r1)
008c: stfs f0, 0x1c(r1)
0090: bl convertLayoutPosToScreenPos__2MRFPQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>
0094: lwz r5, 0x18(r31)
0098: addi r3, r1, 0x20
009c: lfs f3, 0x28(r1)
00a0: mr r4, r3
00a4: lfs f1, 0x0(r5)
00a8: lfs f2, 0x2c(r1)
00ac: lfs f0, 0x4(r5)
00b0: fadds f1, f3, f1
00b4: fadds f0, f2, f0
00b8: stfs f1, 0x8(r1)
00bc: stfs f0, 0xc(r1)
00c0: lwz r5, 0x8(r1)
00c4: lwz r0, 0xc(r1)
00c8: stw r5, 0x10(r1)
00cc: stw r0, 0x14(r1)
00d0: lfs f1, 0x10(r1)
00d4: lfs f0, 0x14(r1)
00d8: stfs f1, 0x20(r1)
00dc: stfs f0, 0x24(r1)
00e0: bl convertScreenPosToLayoutPos__2MRFPQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>
00e4: lfs f3, 0xe0(r1)
00e8: lfs f2, 0x20(r1)
00ec: lfs f1, 0xf0(r1)
00f0: lfs f0, 0x24(r1)
00f4: fadds f2, f3, f2
00f8: fadds f0, f1, f0
00fc: stfs f2, 0xe0(r1)
0100: stfs f0, 0xf0(r1)
0104: b 0x1d4
0108: lwz r3, 0x4(r31)
010c: li r0, 0x6
0110: addi r5, r1, 0xa0
0114: addi r4, r3, 0x50
0118: mtctr r0
011c: lwz r3, 0x4(r4)
0120: lwzu r0, 0x8(r4)
0124: stw r3, 0x4(r5)
0128: stwu r0, 0x8(r5)
012c: bdnz 0x11c
0130: addi r3, r1, 0xa4
0134: addi r4, r1, 0x74
0138: bl PSMTXInverse
013c: addi r3, r1, 0xd4
0140: addi r4, r1, 0x74
0144: mr r5, r3
0148: bl PSMTXConcat
014c: addi r3, r1, 0xa4
0150: addi r4, r1, 0x44
0154: bl PSMTXCopy
0158: lwz r5, 0x18(r31)
015c: addi r3, r1, 0xd4
0160: addi r4, r1, 0x44
0164: lfs f0, 0x0(r5)
0168: mr r5, r3
016c: stfs f0, 0x50(r1)
0170: lwz r6, 0x18(r31)
0174: lfs f0, 0x4(r6)
0178: stfs f0, 0x60(r1)
017c: bl PSMTXConcat
0180: b 0x1d4
0184: lwz r4, 0x18(r31)
0188: addi r3, r1, 0x38
018c: lfs f0, @61176@sda21
0190: mr r5, r3
0194: lfs f2, 0x4(r4)
0198: lfs f1, 0x0(r4)
019c: stfs f1, 0x38(r1)
01a0: stfs f2, 0x3c(r1)
01a4: stfs f0, 0x40(r1)
01a8: lwz r4, 0x4(r31)
01ac: addi r4, r4, 0x54
01b0: bl VEC3TransformNormal__Q24nw4r4mathFPQ34nw4r4math4VEC3PCQ34nw4r4math5MTX34PCQ34nw4r4math4VEC3
01b4: lfs f3, 0xe0(r1)
01b8: lfs f2, 0x38(r1)
01bc: lfs f1, 0xf0(r1)
01c0: lfs f0, 0x3c(r1)
01c4: fadds f2, f3, f2
01c8: fadds f0, f1, f0
01cc: stfs f2, 0xe0(r1)
01d0: stfs f0, 0xf0(r1)
01d4: lwz r3, 0x4(r31)
01d8: li r0, 0x6
01dc: addi r4, r1, 0xd0
01e0: addi r5, r3, 0x80
01e4: mtctr r0
01e8: lwz r3, 0x4(r4)
01ec: lwzu r0, 0x8(r4)
01f0: stw r3, 0x4(r5)
01f4: stwu r0, 0x8(r5)
01f8: bdnz 0x1e8
01fc: lwz r4, 0x4(r31)
0200: mr r3, r31
0204: bl recalcChildGlobalMtx__14LayoutPaneCtrlFPQ34nw4r3lyt4Pane
0208: lwz r0, 0x114(r1)
020c: lwz r31, 0x10c(r1)
0210: mtlr r0
0214: addi r1, r1, 0x110
0218: blr 

REFERENCES
{'offset': '0x7c', 'kind': 109, 'symbol': '@61176', 'addend': 0, 'value_hex': '00000000'}
{'offset': '0x18c', 'kind': 109, 'symbol': '@61176', 'addend': 0, 'value_hex': '00000000'}
