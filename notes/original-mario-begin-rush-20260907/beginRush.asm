0000 stwu r1, -0x20(r1)
0004 mflr r0
0008 stw r0, 0x24(r1)
000c addi r11, r1, 0x20
0010 bl _savegpr_29
0014 lwz r0, 0x7e4(r3)
0018 lis r30, lbl_805B8A80@ha
001c mr r29, r3
0020 stw r0, 0x924(r3)
0024 addi r30, r30, lbl_805B8A80@l
0028 lwz r3, 0x234(r3)
002c bl clearAllJointTransform__13MarioAnimatorFv
0030 mr r3, r29
0034 bl invalidateHitSensors__2MRFP9LiveActor
0038 mr r3, r29
003c addi r4, r30, 0x56
0040 bl stopEffect__10MarioActorFPCc
0044 mr r3, r29
0048 addi r4, r30, 0x63
004c bl stopEffect__10MarioActorFPCc
0050 lwz r4, 0x924(r29)
0054 mr r3, r29
0058 lwz r4, 0x24(r4)
005c lwz r4, 0x4(r4)
0060 bl selectSpinCatchInRush__10MarioActorCFPCc
0064 lhz r0, 0x3d4(r29)
0068 mr r31, r3
006c cmplwi r0, 0x4
0070 bne +0x90
0074 lwz r4, 0x924(r29)
0078 mr r3, r29
007c bl selectHideFlyMeter__10MarioActorCFPC9HitSensor
0080 cmpwi r3, 0x0
0084 beq +0x90
0088 bl getGameSceneLayoutHolder__2MRFv
008c bl changeLifeMeterModeGround__21GameSceneLayoutHolderFv
0090 lwz r4, 0x924(r29)
0094 mr r3, r29
0098 bl isFixJumpRushSensor__10MarioActorCFPC9HitSensor
009c cmpwi r3, 0x0
00a0 bne +0xac
00a4 cmpwi r31, 0x0
00a8 beq +0xd8
00ac mr r3, r29
00b0 bl settingRush__10MarioActorFv
00b4 mr r3, r29
00b8 li r4, lbl_806B2230@sda21
00bc bl getSensor__9LiveActorCFPCc
00c0 bl validate__9HitSensorFv
00c4 mr r3, r29
00c8 addi r4, r30, 0x70
00cc bl getSensor__9LiveActorCFPCc
00d0 bl validate__9HitSensorFv
00d4 b +0x19c
00d8 mr r3, r29
00dc addi r4, r30, 0x70
00e0 bl getSensor__9LiveActorCFPCc
00e4 bl validate__9HitSensorFv
00e8 lwz r3, 0x924(r29)
00ec li r4, 0x67
00f0 bl isType__9HitSensorCFUl
00f4 cmpwi r3, 0x0
00f8 beq +0x118
00fc mr r3, r29
0100 bl forceDeleteEffectAll__2MRFP9LiveActor
0104 lwz r3, 0x1b8(r29)
0108 lwz r12, 0x0(r3)
010c lwz r12, 0x28(r12)
0110 mtctr r12
0114 bctrl 
0118 lwz r4, 0x924(r29)
011c mr r3, r29
0120 bl selectLandEffect__10MarioActorCFPC9HitSensor
0124 cmpwi r3, 0x0
0128 beq +0x138
012c mr r3, r29
0130 addi r4, r30, 0x75
0134 bl playEffect__10MarioActorFPCc
0138 lwz r3, 0x924(r29)
013c lwz r0, 0x0(r3)
0140 cmpwi r0, 0x67
0144 beq +0x14c
0148 b +0x164
014c mr r3, r29
0150 li r4, 0x0
0154 li r5, 0x0
0158 bl setPlayerMode__10MarioActorFUsb
015c mr r3, r29
0160 bl resetFog__10MarioActorFv
0164 mr r3, r29
0168 bl settingRush__10MarioActorFv
016c lwz r3, 0x924(r29)
0170 li r4, 0x67
0174 bl isType__9HitSensorCFUl
0178 cmpwi r3, 0x0
017c bne +0x19c
0180 lwz r4, 0x23c(r29)
0184 mr r3, r29
0188 lwz r0, 0x8(r4)
018c slwi r0, r0, 2
0190 lwzx r4, r4, r0
0194 lhz r4, 0x7a0(r4)
0198 bl setBlendMtxTimer__10MarioActorFUs
019c addi r11, r1, 0x20
01a0 bl _restgpr_29
01a4 lwz r0, 0x24(r1)
01a8 mtlr r0
01ac addi r1, r1, 0x20
01b0 blr 
