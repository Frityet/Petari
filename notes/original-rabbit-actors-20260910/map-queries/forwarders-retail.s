# getStrikeInfoNumCategory__21@unnamed@MapUtil_cpp@Fl
/* 803E19D0 003DCF10  94 21 FF F0 */	stwu r1, -0x10(r1)
/* 803E19D4 003DCF14  7C 08 02 A6 */	mflr r0
/* 803E19D8 003DCF18  90 01 00 14 */	stw r0, 0x14(r1)
/* 803E19DC 003DCF1C  93 E1 00 0C */	stw r31, 0xc(r1)
/* 803E19E0 003DCF20  7C 7F 1B 78 */	mr r31, r3
/* 803E19E4 003DCF24  4B D9 42 E9 */	bl getCollisionDirector__2MRFv
/* 803E19E8 003DCF28  80 63 00 0C */	lwz r3, 0xc(r3)
/* 803E19EC 003DCF2C  57 E0 10 3A */	slwi r0, r31, 2
/* 803E19F0 003DCF30  83 E1 00 0C */	lwz r31, 0xc(r1)
/* 803E19F4 003DCF34  7C 63 00 2E */	lwzx r3, r3, r0
/* 803E19F8 003DCF38  80 01 00 14 */	lwz r0, 0x14(r1)
/* 803E19FC 003DCF3C  80 63 00 10 */	lwz r3, 0x10(r3)
/* 803E1A00 003DCF40  7C 08 03 A6 */	mtlr r0
/* 803E1A04 003DCF44  38 21 00 10 */	addi r1, r1, 0x10
/* 803E1A08 003DCF48  4E 80 00 20 */	blr

# getFirstPolyOnLineToMap__2MRFPQ29JGeometry8TVec3<f>P8TriangleRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>
/* 803E1F00 003DD440  38 E0 00 00 */	li r7, 0x0
/* 803E1F04 003DD444  39 00 00 00 */	li r8, 0x0
/* 803E1F08 003DD448  39 20 00 00 */	li r9, 0x0
/* 803E1F0C 003DD44C  4B FF FB 00 */	b "getFirstPolyOnLineCategory__21@unnamed@MapUtil_cpp@FPQ29JGeometry8TVec3<f>P8TriangleRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PC18TriangleFilterBasePC24CollisionPartsFilterBasel"

# getFirstPolyOnLineToMap__2MRFPQ29JGeometry8TVec3<f>P8TriangleRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PC24CollisionPartsFilterBasePC18TriangleFilterBase
/* 803E207C 003DD5BC  7C E0 3B 78 */	mr r0, r7
/* 803E2080 003DD5C0  7D 07 43 78 */	mr r7, r8
/* 803E2084 003DD5C4  7C 08 03 78 */	mr r8, r0
/* 803E2088 003DD5C8  39 20 00 00 */	li r9, 0x0
/* 803E208C 003DD5CC  4B FF F9 80 */	b "getFirstPolyOnLineCategory__21@unnamed@MapUtil_cpp@FPQ29JGeometry8TVec3<f>P8TriangleRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PC18TriangleFilterBasePC24CollisionPartsFilterBasel"

# checkStrikeLineToSunshade__9CollisionFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>lPC24CollisionPartsFilterBasePC18TriangleFilterBase
/* 803E3DC8 003DF308  94 21 FF E0 */	stwu r1, -0x20(r1)
/* 803E3DCC 003DF30C  7C 08 02 A6 */	mflr r0
/* 803E3DD0 003DF310  90 01 00 24 */	stw r0, 0x24(r1)
/* 803E3DD4 003DF314  39 61 00 20 */	addi r11, r1, 0x20
/* 803E3DD8 003DF318  48 13 4C 29 */	bl _savegpr_27
/* 803E3DDC 003DF31C  7C 7B 1B 78 */	mr r27, r3
/* 803E3DE0 003DF320  7C 9C 23 78 */	mr r28, r4
/* 803E3DE4 003DF324  7C BD 2B 78 */	mr r29, r5
/* 803E3DE8 003DF328  7C DE 33 78 */	mr r30, r6
/* 803E3DEC 003DF32C  7C FF 3B 78 */	mr r31, r7
/* 803E3DF0 003DF330  4B D9 1E DD */	bl getCollisionDirector__2MRFv
/* 803E3DF4 003DF334  80 63 00 0C */	lwz r3, 0xc(r3)
/* 803E3DF8 003DF338  7F 64 DB 78 */	mr r4, r27
/* 803E3DFC 003DF33C  7F 85 E3 78 */	mr r5, r28
/* 803E3E00 003DF340  7F A6 EB 78 */	mr r6, r29
/* 803E3E04 003DF344  80 63 00 04 */	lwz r3, 0x4(r3)
/* 803E3E08 003DF348  7F C7 F3 78 */	mr r7, r30
/* 803E3E0C 003DF34C  7F E8 FB 78 */	mr r8, r31
/* 803E3E10 003DF350  4B D9 06 5D */	bl "checkStrikeLine__26CollisionCategorizedKeeperFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>lPC24CollisionPartsFilterBasePC18TriangleFilterBase"
/* 803E3E14 003DF354  39 61 00 20 */	addi r11, r1, 0x20
/* 803E3E18 003DF358  48 13 4C 35 */	bl _restgpr_27
/* 803E3E1C 003DF35C  80 01 00 24 */	lwz r0, 0x24(r1)
/* 803E3E20 003DF360  7C 08 03 A6 */	mtlr r0
/* 803E3E24 003DF364  38 21 00 20 */	addi r1, r1, 0x20
/* 803E3E28 003DF368  4E 80 00 20 */	blr

# getStrikeInfoMap__9CollisionFUl
/* 803E3E2C 003DF36C  94 21 FF F0 */	stwu r1, -0x10(r1)
/* 803E3E30 003DF370  7C 08 02 A6 */	mflr r0
/* 803E3E34 003DF374  90 01 00 14 */	stw r0, 0x14(r1)
/* 803E3E38 003DF378  93 E1 00 0C */	stw r31, 0xc(r1)
/* 803E3E3C 003DF37C  7C 7F 1B 78 */	mr r31, r3
/* 803E3E40 003DF380  4B D9 1E 8D */	bl getCollisionDirector__2MRFv
/* 803E3E44 003DF384  80 63 00 0C */	lwz r3, 0xc(r3)
/* 803E3E48 003DF388  7F E4 FB 78 */	mr r4, r31
/* 803E3E4C 003DF38C  80 63 00 00 */	lwz r3, 0x0(r3)
/* 803E3E50 003DF390  4B D9 0C F5 */	bl getStrikeInfo__26CollisionCategorizedKeeperFUl
/* 803E3E54 003DF394  80 01 00 14 */	lwz r0, 0x14(r1)
/* 803E3E58 003DF398  83 E1 00 0C */	lwz r31, 0xc(r1)
/* 803E3E5C 003DF39C  7C 08 03 A6 */	mtlr r0
/* 803E3E60 003DF3A0  38 21 00 10 */	addi r1, r1, 0x10
/* 803E3E64 003DF3A4  4E 80 00 20 */	blr

# getStrikeInfoNumMap__9CollisionFv
/* 803E3E68 003DF3A8  38 60 00 00 */	li r3, 0x0
/* 803E3E6C 003DF3AC  4B FF DB 64 */	b "getStrikeInfoNumCategory__21@unnamed@MapUtil_cpp@Fl"
