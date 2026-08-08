# RMGK02 target excerpts from build/RMGK02/asm/Game/Map/FileSelectItem.s
# Full target SHA-1: 83ad9cab285f84baf80688f652a5d9b65ef82abc

# control(): update, retail matrix choice, and target transform construction
.fn control__14FileSelectItemFv, global
/* 80178EA4 001743E4  48 00 04 31 */ bl updatePointing__14FileSelectItemFv
/* 80178EA8 001743E8  7F A3 EB 78 */ mr r3, r29
/* 80178EAC 001743EC  48 00 05 21 */ bl updateRotate__14FileSelectItemFv
/* 80178EB0 001743F0  C0 22 C2 8C */ lfs f1, "@62893"@sda21(r0)
/* 80178EB4 001743F4  C0 1D 00 18 */ lfs f0, 0x18(r29)
/* 80178EB8 001743F8  FC 01 00 00 */ fcmpu cr0, f1, f0
/* 80178EBC 001743FC  40 82 00 20 */ bne .L_80178EDC
/* 80178EC0 00174400  C0 1D 00 20 */ lfs f0, 0x20(r29)
/* 80178EC4 00174404  FC 01 00 00 */ fcmpu cr0, f1, f0
/* 80178EC8 00174408  40 82 00 14 */ bne .L_80178EDC
/* 80178ECC 0017440C  7F A4 EB 78 */ mr r4, r29
/* 80178ED0 00174410  38 61 00 68 */ addi r3, r1, 0x68
/* 80178ED4 00174414  48 24 6C 49 */ bl makeMtxTransRotateY__2MRFPA4_fPC9LiveActor
/* 80178ED8 00174418  48 00 00 10 */ b .L_80178EE8
.L_80178EDC:
/* 80178EDC 0017441C  7F A4 EB 78 */ mr r4, r29
/* 80178EE0 00174420  38 61 00 68 */ addi r3, r1, 0x68
/* 80178EE4 00174424  48 24 6B D5 */ bl makeMtxTR__2MRFPA4_fPC9LiveActor

# control(): scale controller drives planet, five fellow models, and Mii
/* 80178F98 001744D8  80 7D 01 48 */ lwz r3, 0x148(r29)
/* 80178F9C 001744DC  48 22 AF E1 */ bl updateNerve__13NerveExecutorFv
/* 80178FA0 001744E0  80 9D 01 48 */ lwz r4, 0x148(r29)
/* 80178FA4 001744E4  80 7D 00 90 */ lwz r3, 0x90(r29)
/* 80178FA8 001744E8  C0 24 00 08 */ lfs f1, 0x8(r4)
/* 80178FAC 001744EC  C0 02 C2 A0 */ lfs f0, "@64341"@sda21(r0)
/* 80178FB0 001744F0  38 63 00 24 */ addi r3, r3, 0x24
/* 80178FB4 001744F4  EC 20 00 72 */ fmuls f1, f0, f1
/* 80178FB8 001744F8  4B ED 5D 9D */ bl "setAll<f>__Q29JGeometry8TVec3<f>Ff_v"
/* 80178FE0 00174520  4B ED 5D 75 */ bl "setAll<f>__Q29JGeometry8TVec3<f>Ff_v"
/* 80178FEC 0017452C  2C 1E 00 05 */ cmpwi r30, 0x5
/* 80178FF0 00174530  41 80 FF D8 */ blt .L_80178FC8
/* 80178FF4 00174534  80 9D 01 48 */ lwz r4, 0x148(r29)
/* 80178FF8 00174538  80 7D 00 9C */ lwz r3, 0x9c(r29)
/* 80179008 00174548  EC 20 00 72 */ fmuls f1, f0, f1
/* 8017900C 0017454C  4B ED 5D 49 */ bl "setAll<f>__Q29JGeometry8TVec3<f>Ff_v"

# updateRotate(): squared turn-to-front easing and early exit
/* 80179404 00174944  80 A3 01 6C */ lwz r5, 0x16c(r3)
/* 80179414 00174954  38 E5 00 01 */ addi r7, r5, 0x1
/* 80179450 00174990  EF E2 08 24 */ fdivs f31, f2, f1
/* 80179454 00174994  EF FF 07 F2 */ fmuls f31, f31, f31
/* 80179474 001749B4  7C 07 40 00 */ cmpw r7, r8
/* 8017947C 001749BC  38 00 00 00 */ li r0, 0x0
/* 80179480 001749C0  90 03 01 6C */ stw r0, 0x16c(r3)
/* 80179484 001749C4  90 03 01 68 */ stw r0, 0x168(r3)
/* 801794B8 001749F8  7F C3 F3 78 */ mr r3, r30
/* 801794BC 001749FC  EC 01 07 F2 */ fmuls f0, f1, f31
/* 801794C0 00174A00  EC 01 00 28 */ fsubs f0, f1, f0
/* 801794C4 00174A04  D0 1F 00 1C */ stfs f0, 0x1c(r31)
/* 801794C8 00174A08  48 00 0F C9 */ bl open__Q217FileSelectItemSub15BlinkControllerFv
/* 801794D4 00174A14  48 22 AA BD */ bl setNerve__13NerveExecutorFPC5Nerve
/* 801794D8 00174A18  48 00 05 4C */ b .L_80179A24

# updateRotate(): retail pointer gain and clamp
/* 80179900 00174E40  C0 42 C2 B0 */ lfs f2, "@64523"@sda21(r0)  # 0.03f
/* 8017992C 00174E6C  EC 42 00 F2 */ fmuls f2, f2, f3
/* 80179938 00174E78  EC 22 08 24 */ fdivs f1, f2, f1
/* 80179948 00174E88  EC 00 08 2A */ fadds f0, f0, f1
/* 8017994C 00174E8C  FC 00 28 40 */ fcmpo cr0, f0, f5
/* 80179950 00174E90  D0 1F 01 60 */ stfs f0, 0x160(r31)
/* 80179954 00174E94  40 80 00 08 */ bge .L_8017995C
/* 8017995C 00174E9C  C0 A2 C2 B8 */ lfs f5, "@64525"@sda21(r0)  # 25.0f
/* 80179960 00174EA0  FC 00 28 40 */ fcmpo cr0, f0, f5
/* 80179964 00174EA4  40 81 00 08 */ ble .L_8017996C
/* 8017996C 00174EAC  FC A0 00 90 */ fmr f5, f0
/* 80179970 00174EB0  D0 BF 01 60 */ stfs f5, 0x160(r31)

# Recovered visibility/effect symbols and target spans
# appearFellowModel:   80179B7C..80179BCC (0x50)
# killAllModels:       80179BCC..80179C54 (0x88)
# emitOpen:            80179C54..80179CFC (0xA8)
# emitVanish:          80179CFC..80179DA4 (0xA8)
# emitCopy:            80179DA4..80179E4C (0xA8)
# emitCompleteEffect:  80179E4C..80179EE0 (0x94)
# deleteCompleteEffect:80179EE0..80179F44 (0x64)
