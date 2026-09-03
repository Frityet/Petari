
build/mario-shadow-view-20260903/calcViewReflectionModel.o:	file format elf32-powerpc

Disassembly of section .data:

802bfc5c <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start>:
802bfc5c: 94 21 fe 00  	stwu 1, -512(1)
802bfc60: 7c 08 02 a6  	mflr 0
802bfc64: 90 01 02 04  	stw 0, 516(1)
802bfc68: db e1 01 f0  	stfd 31, 496(1)
802bfc6c: f3 e1 01 f8  	xxsel 31, 1, 0, 39
802bfc70: db c1 01 e0  	stfd 30, 480(1)
802bfc74: f3 c1 01 e8  	<unknown>
802bfc78: db a1 01 d0  	stfd 29, 464(1)
802bfc7c: f3 a1 01 d8  	<unknown>
802bfc80: db 81 01 c0  	stfd 28, 448(1)
802bfc84: f3 81 01 c8  	xsmsubmdp 28, 1, 0
802bfc88: 39 61 01 c0  	addi 11, 1, 448
802bfc8c: 48 25 8d 7d  	bl 0x80518a08 _savegpr_29 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x25868c>
802bfc90: 7c 7e 1b 78  	mr	30, 3
802bfc94: 4b ff 75 91  	bl 0x802b7224 getJ3DModel__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffff6ea8>
802bfc98: 7c 7f 1b 78  	mr	31, 3
802bfc9c: 7f c3 f3 78  	mr	3, 30
802bfca0: 4b ff 75 85  	bl 0x802b7224 getJ3DModel__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffff6ea8>
802bfca4: 38 63 00 24  	addi 3, 3, 36
802bfca8: 38 81 01 78  	addi 4, 1, 376
802bfcac: 48 1f 86 e1  	bl 0x804b838c PSMTXCopy <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1f8010>
802bfcb0: 38 61 01 48  	addi 3, 1, 328
802bfcb4: 48 1f 86 ad  	bl 0x804b8360 PSMTXIdentity <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1f7fe4>
802bfcb8: 3c 80 80 5c  	lis 4, -32676
802bfcbc: c3 e2 fb 9c  	lfs 31, -1124(2)
802bfcc0: 7f c3 f3 78  	mr	3, 30
802bfcc4: 38 84 8e 18  	addi 4, 4, -29160
802bfcc8: 4b ff 00 11  	bl 0x802afcd8 isAnimationRun__10MarioActorCFPCc <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xfffffffffffef95c>
802bfccc: 2c 03 00 00  	cmpwi	3, 0
802bfcd0: 41 82 00 08  	bt	2, 0x802bfcd8 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x7c>
802bfcd4: c3 e2 fb a0  	lfs 31, -1120(2)
802bfcd8: 88 1e 0a 08  	lbz 0, 2568(30)
802bfcdc: 28 00 00 06  	cmplwi	0, 6
802bfce0: 41 82 00 14  	bt	2, 0x802bfcf4 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x98>
802bfce4: 28 00 00 07  	cmplwi	0, 7
802bfce8: 41 82 00 0c  	bt	2, 0x802bfcf4 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x98>
802bfcec: 38 00 00 00  	li 0, 0
802bfcf0: 98 1e 0a 24  	stb 0, 2596(30)
802bfcf4: 88 1e 0a 08  	lbz 0, 2568(30)
802bfcf8: 28 00 00 03  	cmplwi	0, 3
802bfcfc: 41 82 00 0c  	bt	2, 0x802bfd08 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0xac>
802bfd00: 38 00 00 00  	li 0, 0
802bfd04: 98 1e 0a 25  	stb 0, 2597(30)
802bfd08: 88 1e 0a 08  	lbz 0, 2568(30)
802bfd0c: 2c 00 00 03  	cmpwi	0, 3
802bfd10: 41 82 00 e4  	bt	2, 0x802bfdf4 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x198>
802bfd14: 40 80 00 10  	bf	0, 0x802bfd24 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0xc8>
802bfd18: 2c 00 00 02  	cmpwi	0, 2
802bfd1c: 40 80 00 1c  	bf	0, 0x802bfd38 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0xdc>
802bfd20: 48 00 05 68  	b 0x802c0288 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x62c>
802bfd24: 2c 00 00 08  	cmpwi	0, 8
802bfd28: 40 80 05 60  	bf	0, 0x802c0288 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x62c>
802bfd2c: 2c 00 00 06  	cmpwi	0, 6
802bfd30: 40 80 03 08  	bf	0, 0x802c0038 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x3dc>
802bfd34: 48 00 05 54  	b 0x802c0288 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x62c>
802bfd38: c0 41 01 7c  	lfs 2, 380(1)
802bfd3c: 38 80 00 00  	li 4, 0
802bfd40: c0 21 01 8c  	lfs 1, 396(1)
802bfd44: c0 01 01 9c  	lfs 0, 412(1)
802bfd48: fc 40 10 50  	fneg 2, 2
802bfd4c: fc 20 08 50  	fneg 1, 1
802bfd50: fc 00 00 50  	fneg 0, 0
802bfd54: d0 41 01 7c  	stfs 2, 380(1)
802bfd58: d0 21 01 8c  	stfs 1, 396(1)
802bfd5c: d0 01 01 9c  	stfs 0, 412(1)
802bfd60: 80 7e 02 30  	lwz 3, 560(30)
802bfd64: 80 63 04 5c  	lwz 3, 1116(3)
802bfd68: 4b ec 2e 11  	bl 0x80182b78 getNormal__8TriangleCFi <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffec27fc>
802bfd6c: fc 20 f8 90  	fmr 1, 31
802bfd70: 7c 64 1b 78  	mr	4, 3
802bfd74: 38 61 00 88  	addi 3, 1, 136
802bfd78: 4b d5 91 15  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b10>
802bfd7c: 80 9e 02 30  	lwz 4, 560(30)
802bfd80: 38 61 00 94  	addi 3, 1, 148
802bfd84: 38 84 03 1c  	addi 4, 4, 796
802bfd88: 4b d5 91 69  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b74>
802bfd8c: 38 61 00 94  	addi 3, 1, 148
802bfd90: 38 81 00 88  	addi 4, 1, 136
802bfd94: 4b d6 03 65  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5fd7c>
802bfd98: 38 61 01 0c  	addi 3, 1, 268
802bfd9c: 38 81 00 94  	addi 4, 1, 148
802bfda0: 4b d5 90 d9  	bl 0x80018e78 __as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58afc>
802bfda4: 3c 80 80 5c  	lis 4, -32676
802bfda8: 7f c3 f3 78  	mr	3, 30
802bfdac: 38 84 8e 27  	addi 4, 4, -29145
802bfdb0: 38 a1 01 00  	addi 5, 1, 256
802bfdb4: 4b ff 2c e5  	bl 0x802b2a98 getRealPos__10MarioActorCFPCcPQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffff271c>
802bfdb8: 38 61 00 f4  	addi 3, 1, 244
802bfdbc: 38 9e 00 0c  	addi 4, 30, 12
802bfdc0: 4b d5 91 31  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b74>
802bfdc4: 38 61 00 f4  	addi 3, 1, 244
802bfdc8: 38 81 01 00  	addi 4, 1, 256
802bfdcc: 4b d6 03 2d  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5fd7c>
802bfdd0: c0 21 00 f4  	lfs 1, 244(1)
802bfdd4: 38 61 01 48  	addi 3, 1, 328
802bfdd8: c0 41 00 f8  	lfs 2, 248(1)
802bfddc: c0 61 00 fc  	lfs 3, 252(1)
802bfde0: 48 1f 8a 49  	bl 0x804b8828 PSMTXTrans <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1f84ac>
802bfde4: 88 9e 0a 09  	lbz 4, 2569(30)
802bfde8: 7f c3 f3 78  	mr	3, 30
802bfdec: 48 00 25 bd  	bl 0x802c23a8 updateReflectAlphaDL__10MarioActorFUc <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x202c>
802bfdf0: 48 00 04 98  	b 0x802c0288 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x62c>
802bfdf4: c0 21 01 78  	lfs 1, 376(1)
802bfdf8: 38 61 00 e8  	addi 3, 1, 232
802bfdfc: c0 41 01 88  	lfs 2, 392(1)
802bfe00: c0 61 01 98  	lfs 3, 408(1)
802bfe04: 4b d5 74 e1  	bl 0x800172e4 set<f>__Q29JGeometry8TVec3<f>Ffff_v <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd56f68>
802bfe08: c0 21 01 7c  	lfs 1, 380(1)
802bfe0c: 38 61 00 dc  	addi 3, 1, 220
802bfe10: c0 41 01 8c  	lfs 2, 396(1)
802bfe14: c0 61 01 9c  	lfs 3, 412(1)
802bfe18: 4b d5 74 cd  	bl 0x800172e4 set<f>__Q29JGeometry8TVec3<f>Ffff_v <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd56f68>
802bfe1c: c0 21 01 80  	lfs 1, 384(1)
802bfe20: 38 61 00 d0  	addi 3, 1, 208
802bfe24: c0 41 01 90  	lfs 2, 400(1)
802bfe28: c0 61 01 a0  	lfs 3, 416(1)
802bfe2c: 4b d5 74 b9  	bl 0x800172e4 set<f>__Q29JGeometry8TVec3<f>Ffff_v <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd56f68>
802bfe30: 80 9e 02 30  	lwz 4, 560(30)
802bfe34: 38 61 00 e8  	addi 3, 1, 232
802bfe38: 7c 65 1b 78  	mr	5, 3
802bfe3c: 80 84 08 84  	lwz 4, 2180(4)
802bfe40: 3b a4 01 60  	addi 29, 4, 352
802bfe44: 7f a4 eb 78  	mr	4, 29
802bfe48: 48 12 76 ed  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1271b8>
802bfe4c: ff c0 08 90  	fmr 30, 1
802bfe50: 38 61 00 dc  	addi 3, 1, 220
802bfe54: 7f a4 eb 78  	mr	4, 29
802bfe58: 7c 65 1b 78  	mr	5, 3
802bfe5c: 48 12 76 d9  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1271b8>
802bfe60: ff a0 08 90  	fmr 29, 1
802bfe64: 38 61 00 d0  	addi 3, 1, 208
802bfe68: 7f a4 eb 78  	mr	4, 29
802bfe6c: 7c 65 1b 78  	mr	5, 3
802bfe70: 48 12 76 c5  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1271b8>
802bfe74: ff 80 08 90  	fmr 28, 1
802bfe78: 7f a4 eb 78  	mr	4, 29
802bfe7c: fc 20 f0 90  	fmr 1, 30
802bfe80: 38 61 00 7c  	addi 3, 1, 124
802bfe84: 4b d5 90 09  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b10>
802bfe88: 38 61 00 e8  	addi 3, 1, 232
802bfe8c: 38 81 00 7c  	addi 4, 1, 124
802bfe90: 4b d6 02 69  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5fd7c>
802bfe94: fc 20 e8 90  	fmr 1, 29
802bfe98: 7f a4 eb 78  	mr	4, 29
802bfe9c: 38 61 00 70  	addi 3, 1, 112
802bfea0: 4b d5 8f ed  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b10>
802bfea4: 38 61 00 dc  	addi 3, 1, 220
802bfea8: 38 81 00 70  	addi 4, 1, 112
802bfeac: 4b d6 02 4d  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5fd7c>
802bfeb0: fc 20 e0 90  	fmr 1, 28
802bfeb4: 7f a4 eb 78  	mr	4, 29
802bfeb8: 38 61 00 64  	addi 3, 1, 100
802bfebc: 4b d5 8f d1  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b10>
802bfec0: 38 61 00 d0  	addi 3, 1, 208
802bfec4: 38 81 00 64  	addi 4, 1, 100
802bfec8: 4b d6 02 31  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5fd7c>
802bfecc: c1 21 00 e8  	lfs 9, 232(1)
802bfed0: fc 20 f8 90  	fmr 1, 31
802bfed4: c1 01 00 ec  	lfs 8, 236(1)
802bfed8: 7f a4 eb 78  	mr	4, 29
802bfedc: c0 e1 00 f0  	lfs 7, 240(1)
802bfee0: 38 61 00 4c  	addi 3, 1, 76
802bfee4: c0 c1 00 dc  	lfs 6, 220(1)
802bfee8: c0 a1 00 e0  	lfs 5, 224(1)
802bfeec: c0 81 00 e4  	lfs 4, 228(1)
802bfef0: c0 61 00 d0  	lfs 3, 208(1)
802bfef4: c0 41 00 d4  	lfs 2, 212(1)
802bfef8: c0 01 00 d8  	lfs 0, 216(1)
802bfefc: d1 21 01 78  	stfs 9, 376(1)
802bff00: d1 01 01 88  	stfs 8, 392(1)
802bff04: d0 e1 01 98  	stfs 7, 408(1)
802bff08: d0 c1 01 7c  	stfs 6, 380(1)
802bff0c: d0 a1 01 8c  	stfs 5, 396(1)
802bff10: d0 81 01 9c  	stfs 4, 412(1)
802bff14: d0 61 01 80  	stfs 3, 384(1)
802bff18: d0 41 01 90  	stfs 2, 400(1)
802bff1c: d0 01 01 a0  	stfs 0, 416(1)
802bff20: 4b d5 8f 6d  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b10>
802bff24: 80 9e 02 30  	lwz 4, 560(30)
802bff28: 38 61 00 58  	addi 3, 1, 88
802bff2c: 80 84 08 84  	lwz 4, 2180(4)
802bff30: 38 84 01 6c  	addi 4, 4, 364
802bff34: 4b d5 8f bd  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b74>
802bff38: 38 61 00 58  	addi 3, 1, 88
802bff3c: 38 81 00 4c  	addi 4, 1, 76
802bff40: 4b d6 01 b9  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5fd7c>
802bff44: 38 61 01 0c  	addi 3, 1, 268
802bff48: 38 81 00 58  	addi 4, 1, 88
802bff4c: 4b d5 8f 2d  	bl 0x80018e78 __as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58afc>
802bff50: 80 7e 02 30  	lwz 3, 560(30)
802bff54: c0 22 fb a4  	lfs 1, -1116(2)
802bff58: 80 63 08 84  	lwz 3, 2180(3)
802bff5c: c0 03 01 9c  	lfs 0, 412(3)
802bff60: fc 40 00 50  	fneg 2, 0
802bff64: fc 02 08 40  	fcmpo 0, 2, 1
802bff68: 40 80 00 0c  	bf	0, 0x802bff74 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x318>
802bff6c: c3 e2 fb 84  	lfs 31, -1148(2)
802bff70: 48 00 00 24  	b 0x802bff94 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x338>
802bff74: c0 02 fb ac  	lfs 0, -1108(2)
802bff78: fc 02 00 40  	fcmpo 0, 2, 0
802bff7c: 40 80 00 14  	bf	0, 0x802bff90 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x334>
802bff80: ec 22 08 28  	fsubs 1, 2, 1
802bff84: c0 02 fb a8  	lfs 0, -1112(2)
802bff88: ef e1 00 24  	fdivs 31, 1, 0
802bff8c: 48 00 00 08  	b 0x802bff94 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x338>
802bff90: c3 e2 fb 80  	lfs 31, -1152(2)
802bff94: 7f a4 eb 78  	mr	4, 29
802bff98: 38 7e 0f 9c  	addi 3, 30, 3996
802bff9c: 4b d5 d3 0d  	bl 0x8001d2a8 dot__Q29JGeometry8TVec3<f>CFRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5cf2c>
802bffa0: d0 21 00 0c  	stfs 1, 12(1)
802bffa4: 38 61 00 0c  	addi 3, 1, 12
802bffa8: 4b e7 3f d5  	bl 0x80133f7c clamp01__2MRFPf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffe73c00>
802bffac: c0 21 00 0c  	lfs 1, 12(1)
802bffb0: 38 61 00 0c  	addi 3, 1, 12
802bffb4: c0 02 fb b0  	lfs 0, -1104(2)
802bffb8: ec 01 00 32  	fmuls 0, 1, 0
802bffbc: d0 01 00 0c  	stfs 0, 12(1)
802bffc0: 4b e7 3f bd  	bl 0x80133f7c clamp01__2MRFPf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffe73c00>
802bffc4: c0 21 00 0c  	lfs 1, 12(1)
802bffc8: c0 02 fb b4  	lfs 0, -1100(2)
802bffcc: ec 01 00 32  	fmuls 0, 1, 0
802bffd0: ec 3f 00 32  	fmuls 1, 31, 0
802bffd4: d0 01 00 0c  	stfs 0, 12(1)
802bffd8: 88 7e 0a 25  	lbz 3, 2597(30)
802bffdc: fc 00 08 1e  	fctiwz 0, 1
802bffe0: d8 01 01 a8  	stfd 0, 424(1)
802bffe4: 80 01 01 ac  	lwz 0, 428(1)
802bffe8: 54 00 06 3e  	clrlwi	0, 0, 24
802bffec: 7c 03 00 40  	cmplw	3, 0
802bfff0: 40 81 00 10  	bf	1, 0x802c0000 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x3a4>
802bfff4: 38 03 ff ff  	addi 0, 3, -1
802bfff8: 98 1e 0a 25  	stb 0, 2597(30)
802bfffc: 48 00 00 20  	b 0x802c001c <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x3c0>
802c0000: d8 01 01 a8  	stfd 0, 424(1)
802c0004: 80 01 01 ac  	lwz 0, 428(1)
802c0008: 54 00 06 3e  	clrlwi	0, 0, 24
802c000c: 7c 03 00 40  	cmplw	3, 0
802c0010: 40 80 00 0c  	bf	0, 0x802c001c <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x3c0>
802c0014: 38 03 00 01  	addi 0, 3, 1
802c0018: 98 1e 0a 25  	stb 0, 2597(30)
802c001c: 88 9e 0a 25  	lbz 4, 2597(30)
802c0020: 7f c3 f3 78  	mr	3, 30
802c0024: 48 00 22 b5  	bl 0x802c22d8 updateSimpleAlphaDL__10MarioActorFUc <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1f5c>
802c0028: 38 7e 0b 18  	addi 3, 30, 2840
802c002c: 38 81 01 0c  	addi 4, 1, 268
802c0030: 4b d5 8e 49  	bl 0x80018e78 __as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58afc>
802c0034: 48 00 02 54  	b 0x802c0288 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x62c>
802c0038: c0 21 01 78  	lfs 1, 376(1)
802c003c: 38 61 00 c4  	addi 3, 1, 196
802c0040: c0 41 01 88  	lfs 2, 392(1)
802c0044: c0 61 01 98  	lfs 3, 408(1)
802c0048: 4b d5 72 9d  	bl 0x800172e4 set<f>__Q29JGeometry8TVec3<f>Ffff_v <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd56f68>
802c004c: c0 21 01 7c  	lfs 1, 380(1)
802c0050: 38 61 00 b8  	addi 3, 1, 184
802c0054: c0 41 01 8c  	lfs 2, 396(1)
802c0058: c0 61 01 9c  	lfs 3, 412(1)
802c005c: 4b d5 72 89  	bl 0x800172e4 set<f>__Q29JGeometry8TVec3<f>Ffff_v <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd56f68>
802c0060: c0 21 01 80  	lfs 1, 384(1)
802c0064: 38 61 00 ac  	addi 3, 1, 172
802c0068: c0 41 01 90  	lfs 2, 400(1)
802c006c: c0 61 01 a0  	lfs 3, 416(1)
802c0070: 4b d5 72 75  	bl 0x800172e4 set<f>__Q29JGeometry8TVec3<f>Ffff_v <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd56f68>
802c0074: 80 9e 02 30  	lwz 4, 560(30)
802c0078: 38 61 00 a0  	addi 3, 1, 160
802c007c: 80 84 08 84  	lwz 4, 2180(4)
802c0080: 38 84 01 78  	addi 4, 4, 376
802c0084: 4b d5 8e 6d  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b74>
802c0088: 38 61 00 a0  	addi 3, 1, 160
802c008c: 38 9e 00 0c  	addi 4, 30, 12
802c0090: 4b d6 00 69  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5fd7c>
802c0094: 38 61 00 a0  	addi 3, 1, 160
802c0098: 48 12 63 19  	bl 0x803e63b0 normalize__2MRFPQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x126034>
802c009c: 38 61 00 c4  	addi 3, 1, 196
802c00a0: 38 81 00 a0  	addi 4, 1, 160
802c00a4: 7c 65 1b 78  	mr	5, 3
802c00a8: 48 12 74 8d  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1271b8>
802c00ac: ff 80 08 90  	fmr 28, 1
802c00b0: 38 61 00 b8  	addi 3, 1, 184
802c00b4: 7c 65 1b 78  	mr	5, 3
802c00b8: 38 81 00 a0  	addi 4, 1, 160
802c00bc: 48 12 74 79  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1271b8>
802c00c0: ff a0 08 90  	fmr 29, 1
802c00c4: 38 61 00 ac  	addi 3, 1, 172
802c00c8: 7c 65 1b 78  	mr	5, 3
802c00cc: 38 81 00 a0  	addi 4, 1, 160
802c00d0: 48 12 74 65  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1271b8>
802c00d4: ff c0 08 90  	fmr 30, 1
802c00d8: 38 61 00 40  	addi 3, 1, 64
802c00dc: fc 20 e0 90  	fmr 1, 28
802c00e0: 38 81 00 a0  	addi 4, 1, 160
802c00e4: 4b d5 8d a9  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b10>
802c00e8: 38 61 00 c4  	addi 3, 1, 196
802c00ec: 38 81 00 40  	addi 4, 1, 64
802c00f0: 4b d6 00 09  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5fd7c>
802c00f4: fc 20 e8 90  	fmr 1, 29
802c00f8: 38 61 00 34  	addi 3, 1, 52
802c00fc: 38 81 00 a0  	addi 4, 1, 160
802c0100: 4b d5 8d 8d  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b10>
802c0104: 38 61 00 b8  	addi 3, 1, 184
802c0108: 38 81 00 34  	addi 4, 1, 52
802c010c: 4b d5 ff ed  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5fd7c>
802c0110: fc 20 f0 90  	fmr 1, 30
802c0114: 38 61 00 28  	addi 3, 1, 40
802c0118: 38 81 00 a0  	addi 4, 1, 160
802c011c: 4b d5 8d 71  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b10>
802c0120: 38 61 00 ac  	addi 3, 1, 172
802c0124: 38 81 00 28  	addi 4, 1, 40
802c0128: 4b d5 ff d1  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5fd7c>
802c012c: c1 21 00 c4  	lfs 9, 196(1)
802c0130: fc 20 f8 90  	fmr 1, 31
802c0134: c1 01 00 c8  	lfs 8, 200(1)
802c0138: 38 61 00 10  	addi 3, 1, 16
802c013c: c0 e1 00 cc  	lfs 7, 204(1)
802c0140: 38 81 00 a0  	addi 4, 1, 160
802c0144: c0 c1 00 b8  	lfs 6, 184(1)
802c0148: c0 a1 00 bc  	lfs 5, 188(1)
802c014c: c0 81 00 c0  	lfs 4, 192(1)
802c0150: c0 61 00 ac  	lfs 3, 172(1)
802c0154: c0 41 00 b0  	lfs 2, 176(1)
802c0158: c0 01 00 b4  	lfs 0, 180(1)
802c015c: d1 21 01 78  	stfs 9, 376(1)
802c0160: d1 01 01 88  	stfs 8, 392(1)
802c0164: d0 e1 01 98  	stfs 7, 408(1)
802c0168: d0 c1 01 7c  	stfs 6, 380(1)
802c016c: d0 a1 01 8c  	stfs 5, 396(1)
802c0170: d0 81 01 9c  	stfs 4, 412(1)
802c0174: d0 61 01 80  	stfs 3, 384(1)
802c0178: d0 41 01 90  	stfs 2, 400(1)
802c017c: d0 01 01 a0  	stfs 0, 416(1)
802c0180: 4b d5 8d 0d  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b10>
802c0184: 80 9e 02 30  	lwz 4, 560(30)
802c0188: 38 61 00 1c  	addi 3, 1, 28
802c018c: 80 84 08 84  	lwz 4, 2180(4)
802c0190: 38 84 01 78  	addi 4, 4, 376
802c0194: 4b d5 8d 5d  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58b74>
802c0198: 38 61 00 1c  	addi 3, 1, 28
802c019c: 38 81 00 10  	addi 4, 1, 16
802c01a0: 4b d5 ff 59  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5fd7c>
802c01a4: 38 61 01 0c  	addi 3, 1, 268
802c01a8: 38 81 00 1c  	addi 4, 1, 28
802c01ac: 4b d5 8c cd  	bl 0x80018e78 __as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58afc>
802c01b0: 80 7e 02 30  	lwz 3, 560(30)
802c01b4: 80 63 08 84  	lwz 3, 2180(3)
802c01b8: 48 00 11 75  	bl 0x802c132c getWaterEdgeDist__9MarioSwimCFv <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xfb0>
802c01bc: c3 e2 fb 84  	lfs 31, -1148(2)
802c01c0: fc 01 f8 40  	fcmpo 0, 1, 31
802c01c4: 40 80 00 08  	bf	0, 0x802c01cc <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x570>
802c01c8: 48 00 00 20  	b 0x802c01e8 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x58c>
802c01cc: c0 02 fb 98  	lfs 0, -1128(2)
802c01d0: fc 01 00 40  	fcmpo 0, 1, 0
802c01d4: 40 80 00 14  	bf	0, 0x802c01e8 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x58c>
802c01d8: ec 01 00 24  	fdivs 0, 1, 0
802c01dc: c0 22 fb 88  	lfs 1, -1144(2)
802c01e0: ec 01 00 32  	fmuls 0, 1, 0
802c01e4: ef e1 00 2a  	fadds 31, 1, 0
802c01e8: 38 7e 0f 9c  	addi 3, 30, 3996
802c01ec: 38 81 00 a0  	addi 4, 1, 160
802c01f0: 4b d5 d0 b9  	bl 0x8001d2a8 dot__Q29JGeometry8TVec3<f>CFRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd5cf2c>
802c01f4: d0 21 00 08  	stfs 1, 8(1)
802c01f8: 38 61 00 08  	addi 3, 1, 8
802c01fc: 4b e7 3d 81  	bl 0x80133f7c clamp01__2MRFPf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffe73c00>
802c0200: c0 21 00 08  	lfs 1, 8(1)
802c0204: 38 61 00 08  	addi 3, 1, 8
802c0208: c0 02 fb b0  	lfs 0, -1104(2)
802c020c: ec 01 00 32  	fmuls 0, 1, 0
802c0210: d0 01 00 08  	stfs 0, 8(1)
802c0214: 4b e7 3d 69  	bl 0x80133f7c clamp01__2MRFPf <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffe73c00>
802c0218: c0 21 00 08  	lfs 1, 8(1)
802c021c: c0 02 fb b8  	lfs 0, -1096(2)
802c0220: ec 01 00 32  	fmuls 0, 1, 0
802c0224: ec 3f 00 32  	fmuls 1, 31, 0
802c0228: d0 01 00 08  	stfs 0, 8(1)
802c022c: 88 7e 0a 24  	lbz 3, 2596(30)
802c0230: fc 00 08 1e  	fctiwz 0, 1
802c0234: d8 01 01 a8  	stfd 0, 424(1)
802c0238: 80 01 01 ac  	lwz 0, 428(1)
802c023c: 54 00 06 3e  	clrlwi	0, 0, 24
802c0240: 7c 03 00 40  	cmplw	3, 0
802c0244: 40 81 00 10  	bf	1, 0x802c0254 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x5f8>
802c0248: 38 03 ff ff  	addi 0, 3, -1
802c024c: 98 1e 0a 24  	stb 0, 2596(30)
802c0250: 48 00 00 20  	b 0x802c0270 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x614>
802c0254: d8 01 01 a8  	stfd 0, 424(1)
802c0258: 80 01 01 ac  	lwz 0, 428(1)
802c025c: 54 00 06 3e  	clrlwi	0, 0, 24
802c0260: 7c 03 00 40  	cmplw	3, 0
802c0264: 40 80 00 0c  	bf	0, 0x802c0270 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x614>
802c0268: 38 03 00 01  	addi 0, 3, 1
802c026c: 98 1e 0a 24  	stb 0, 2596(30)
802c0270: 88 9e 0a 24  	lbz 4, 2596(30)
802c0274: 7f c3 f3 78  	mr	3, 30
802c0278: 48 00 20 61  	bl 0x802c22d8 updateSimpleAlphaDL__10MarioActorFUc <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1f5c>
802c027c: 38 7e 0b 18  	addi 3, 30, 2840
802c0280: 38 81 01 0c  	addi 4, 1, 268
802c0284: 4b d5 8b f5  	bl 0x80018e78 __as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffd58afc>
802c0288: c0 41 01 0c  	lfs 2, 268(1)
802c028c: 38 61 01 78  	addi 3, 1, 376
802c0290: c0 21 01 10  	lfs 1, 272(1)
802c0294: 38 9e 0e 0c  	addi 4, 30, 3596
802c0298: c0 01 01 14  	lfs 0, 276(1)
802c029c: d0 41 01 84  	stfs 2, 388(1)
802c02a0: d0 21 01 94  	stfs 1, 404(1)
802c02a4: d0 01 01 a4  	stfs 0, 420(1)
802c02a8: 48 1f 80 e5  	bl 0x804b838c PSMTXCopy <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1f8010>
802c02ac: 7f c3 f3 78  	mr	3, 30
802c02b0: 4b ff c9 99  	bl 0x802bcc48 getCarrySensor__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffffc8cc>
802c02b4: 2c 03 00 00  	cmpwi	3, 0
802c02b8: 41 82 00 44  	bt	2, 0x802c02fc <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_start+0x6a0>
802c02bc: 7f c3 f3 78  	mr	3, 30
802c02c0: 4b ff c9 89  	bl 0x802bcc48 getCarrySensor__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xffffffffffffc8cc>
802c02c4: 80 63 00 24  	lwz 3, 36(3)
802c02c8: 38 80 00 00  	li 4, 0
802c02cc: 48 11 65 b9  	bl 0x803d6884 getJointMtx__2MRFPC9LiveActori <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x116508>
802c02d0: 7c 64 1b 78  	mr	4, 3
802c02d4: 38 7e 0b c8  	addi 3, 30, 3016
802c02d8: 38 a1 01 18  	addi 5, 1, 280
802c02dc: 48 1f 80 e5  	bl 0x804b83c0 PSMTXConcat <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1f8044>
802c02e0: 38 7e 0e 0c  	addi 3, 30, 3596
802c02e4: 38 81 01 18  	addi 4, 1, 280
802c02e8: 7c 65 1b 78  	mr	5, 3
802c02ec: 48 1f 80 d5  	bl 0x804b83c0 PSMTXConcat <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1f8044>
802c02f0: 80 7e 09 a0  	lwz 3, 2464(30)
802c02f4: 38 9e 0e 0c  	addi 4, 30, 3596
802c02f8: 4b fe 84 b1  	bl 0x802a87a8 calcType0__15JetTurtleShadowFPA4_f <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xfffffffffffe842c>
802c02fc: 38 81 01 48  	addi 4, 1, 328
802c0300: 38 7e 0b c8  	addi 3, 30, 3016
802c0304: 7c 85 23 78  	mr	5, 4
802c0308: 48 1f 80 b9  	bl 0x804b83c0 PSMTXConcat <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1f8044>
802c030c: 38 61 01 78  	addi 3, 1, 376
802c0310: 38 81 01 48  	addi 4, 1, 328
802c0314: 7c 65 1b 78  	mr	5, 3
802c0318: 48 1f 80 a9  	bl 0x804b83c0 PSMTXConcat <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1f8044>
802c031c: 3c 60 80 61  	lis 3, -32671
802c0320: 38 81 01 78  	addi 4, 1, 376
802c0324: 38 63 d5 00  	addi 3, 3, -11008
802c0328: 7c 65 1b 78  	mr	5, 3
802c032c: 48 1f 80 95  	bl 0x804b83c0 PSMTXConcat <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x1f8044>
802c0330: 7f e3 fb 78  	mr	3, 31
802c0334: 38 80 00 03  	li 4, 3
802c0338: 38 a0 00 00  	li 5, 0
802c033c: 4b fe 68 39  	bl 0x802a6b74 viewCalc3__9J3DModelXFUlPA4_f <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0xfffffffffffe67f8>
802c0340: 48 10 86 99  	bl 0x803c89d8 loadViewMtx__2MRFv <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x10865c>
802c0344: e3 e1 01 f8  	<unknown>
802c0348: cb e1 01 f0  	lfd 31, 496(1)
802c034c: e3 c1 01 e8  	<unknown>
802c0350: cb c1 01 e0  	lfd 30, 480(1)
802c0354: e3 a1 01 d8  	<unknown>
802c0358: cb a1 01 d0  	lfd 29, 464(1)
802c035c: e3 81 01 c8  	<unknown>
802c0360: 39 61 01 c0  	addi 11, 1, 448
802c0364: cb 81 01 c0  	lfd 28, 448(1)
802c0368: 48 25 86 ed  	bl 0x80518a54 _restgpr_29 <_binary_build_mario_shadow_view_20260903_calcViewReflectionModel_bin_end+0x2586d8>
802c036c: 80 01 02 04  	lwz 0, 516(1)
802c0370: 7c 08 03 a6  	mtlr 0
802c0374: 38 21 02 00  	addi 1, 1, 512
802c0378: 4e 80 00 20  	blr
