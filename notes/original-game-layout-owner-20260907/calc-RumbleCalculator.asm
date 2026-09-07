0000: stwu r1, -0x40(r1)
0004: mflr r0
0008: stw r0, 0x44(r1)
000c: stfd f31, 0x30(r1)
0010: psq_st f31, 0x38(r1), 0, qr0
0014: stw r31, 0x2c(r1)
0018: mr r31, r3
001c: lwz r4, 0x4(r3)
0020: lwz r0, 0x8(r3)
0024: cmplw r4, r0
0028: blt 0x38
002c: addi r3, r3, 0xc
0030: bl zero__Q29JGeometry8TVec3<f>Fv
0034: b 0xf8
0038: lwz r4, 0x4(r3)
003c: lis r6, 0x4330
0040: lwz r0, 0x8(r3)
0044: lis r5, lbl_8053F5C8@ha
0048: stw r4, 0x1c(r1)
004c: addi r4, r3, 0xc
0050: lfd f5, lbl_8053F5C8@l(r5)
0054: addi r5, r1, 0x8
0058: stw r6, 0x18(r1)
005c: lfs f1, @54035@sda21
0060: lfd f0, 0x18(r1)
0064: stw r0, 0x24(r1)
0068: fsubs f4, f0, f5
006c: lfs f0, 0x18(r3)
0070: stw r6, 0x20(r1)
0074: lfs f2, @52824@sda21
0078: lfd f3, 0x20(r1)
007c: fsubs f3, f3, f5
0080: fdivs f3, f4, f3
0084: fmuls f1, f3, f1
0088: fneg f3, f3
008c: fmuls f1, f0, f1
0090: fadds f31, f2, f3
0094: stfs f1, 0x8(r1)
0098: lfs f0, 0x1c(r3)
009c: fadds f1, f1, f0
00a0: stfs f1, 0xc(r1)
00a4: lfs f0, 0x1c(r3)
00a8: fadds f0, f1, f0
00ac: stfs f0, 0x10(r1)
00b0: lwz r12, 0x0(r3)
00b4: lwz r12, 0x8(r12)
00b8: mtctr r12
00bc: bctrl 
00c0: lfs f0, 0x20(r31)
00c4: lwz r3, 0x4(r31)
00c8: fmuls f3, f31, f0
00cc: lfs f2, 0xc(r31)
00d0: lfs f1, 0x10(r31)
00d4: addi r0, r3, 0x1
00d8: lfs f0, 0x14(r31)
00dc: fmuls f2, f2, f3
00e0: fmuls f1, f1, f3
00e4: stw r0, 0x4(r31)
00e8: fmuls f0, f0, f3
00ec: stfs f2, 0xc(r31)
00f0: stfs f1, 0x10(r31)
00f4: stfs f0, 0x14(r31)
00f8: psq_l f31, 0x38(r1), 0, qr0
00fc: lwz r0, 0x44(r1)
0100: lfd f31, 0x30(r1)
0104: lwz r31, 0x2c(r1)
0108: mtlr r0
010c: addi r1, r1, 0x40
0110: blr 

REFERENCES
{'offset': '0x46', 'kind': 6, 'symbol': 'lbl_8053F5C8', 'addend': 0}
{'offset': '0x52', 'kind': 4, 'symbol': 'lbl_8053F5C8', 'addend': 0}
{'offset': '0x5c', 'kind': 109, 'symbol': '@54035', 'addend': 0, 'value_hex': '40c90fdb'}
{'offset': '0x74', 'kind': 109, 'symbol': '@52824', 'addend': 0, 'value_hex': '3f800000'}
