0000: stwu r1, -0x30(r1)
0004: mflr r0
0008: stw r0, 0x34(r1)
000c: stfd f31, 0x20(r1)
0010: psq_st f31, 0x28(r1), 0, qr0
0014: stfd f30, 0x10(r1)
0018: psq_st f30, 0x18(r1), 0, qr0
001c: lfs f1, 0x8(r5)
0020: stw r31, 0xc(r1)
0024: mr r31, r5
0028: stw r30, 0x8(r1)
002c: mr r30, r4
0030: bl JMACosRadian__Ff
0034: fmr f30, f1
0038: lfs f1, 0x4(r31)
003c: bl JMACosRadian__Ff
0040: fmr f31, f1
0044: lfs f1, 0x0(r31)
0048: bl JMACosRadian__Ff
004c: fmr f2, f31
0050: mr r3, r30
0054: fmr f3, f30
0058: bl set<f>__Q29JGeometry8TVec3<f>Ffff_v
005c: psq_l f31, 0x28(r1), 0, qr0
0060: lfd f31, 0x20(r1)
0064: psq_l f30, 0x18(r1), 0, qr0
0068: lfd f30, 0x10(r1)
006c: lwz r31, 0xc(r1)
0070: lwz r0, 0x34(r1)
0074: lwz r30, 0x8(r1)
0078: mtlr r0
007c: addi r1, r1, 0x30
0080: blr 

REFERENCES
