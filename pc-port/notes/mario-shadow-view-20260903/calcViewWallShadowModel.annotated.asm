
build/mario-shadow-view-20260903/calcViewWallShadowModel.o:	file format elf32-powerpc

Disassembly of section .data:

802c037c <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start>:
802c037c: 94 21 fe f0  	stwu 1, -272(1)
802c0380: 7c 08 02 a6  	mflr 0
802c0384: 90 01 01 14  	stw 0, 276(1)
802c0388: 39 61 01 10  	addi 11, 1, 272
802c038c: 48 25 86 79  	bl 0x80518a04 _savegpr_28 <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x2582c0>
802c0390: 88 03 0a 08  	lbz 0, 2568(3)
802c0394: 3b c0 00 00  	li 30, 0
802c0398: 9b c3 0a 0c  	stb 30, 2572(3)
802c039c: 7c 7f 1b 78  	mr	31, 3
802c03a0: 54 00 07 bd  	rlwinm. 0, 0, 0, 30, 30
802c03a4: 40 82 03 88  	bf	2, 0x802c072c <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x3b0>
802c03a8: 88 03 09 f1  	lbz 0, 2545(3)
802c03ac: 2c 00 00 00  	cmpwi	0, 0
802c03b0: 40 82 03 7c  	bf	2, 0x802c072c <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x3b0>
802c03b4: 3c 60 80 5c  	lis 3, -32676
802c03b8: 38 9f 02 a0  	addi 4, 31, 672
802c03bc: 38 63 8e 30  	addi 3, 3, -29136
802c03c0: 3b a0 00 00  	li 29, 0
802c03c4: 48 13 fd 25  	bl 0x804000e8 getAreaObj__2MRFPCcRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x13f9a4>
802c03c8: 2c 03 00 00  	cmpwi	3, 0
802c03cc: 7c 7c 1b 78  	mr	28, 3
802c03d0: 41 82 00 28  	bt	2, 0x802c03f8 <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x7c>
802c03d4: 38 81 00 8c  	addi 4, 1, 140
802c03d8: 48 14 04 59  	bl 0x80400830 calcCubeAxisZ__2MRFPC7AreaObjPQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x1400ec>
802c03dc: 7f 83 e3 78  	mr	3, 28
802c03e0: 38 80 00 00  	li 4, 0
802c03e4: 48 13 fd 95  	bl 0x80400178 getAreaObjArg__2MRFPC7AreaObjl <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x13fa34>
802c03e8: 93 9f 02 0c  	stw 28, 524(31)
802c03ec: 7c 7d 1b 78  	mr	29, 3
802c03f0: 9b df 02 10  	stb 30, 528(31)
802c03f4: 48 00 00 48  	b 0x802c043c <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0xc0>
802c03f8: 3c 60 80 5c  	lis 3, -32676
802c03fc: 38 9f 02 a0  	addi 4, 31, 672
802c0400: 38 63 8e 43  	addi 3, 3, -29117
802c0404: 48 13 fc e5  	bl 0x804000e8 getAreaObj__2MRFPCcRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x13f9a4>
802c0408: 2c 03 00 00  	cmpwi	3, 0
802c040c: 7c 7c 1b 78  	mr	28, 3
802c0410: 41 82 00 2c  	bt	2, 0x802c043c <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0xc0>
802c0414: 7f e3 fb 78  	mr	3, 31
802c0418: 7f 84 e3 78  	mr	4, 28
802c041c: 38 a1 00 8c  	addi 5, 1, 140
802c0420: 48 00 09 95  	bl 0x802c0db4 calcCylinderToCenter__10MarioActorFPC7AreaObjPQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x670>
802c0424: fc 00 08 1e  	fctiwz 0, 1
802c0428: 38 00 00 01  	li 0, 1
802c042c: 93 9f 02 0c  	stw 28, 524(31)
802c0430: d8 01 00 f8  	stfd 0, 248(1)
802c0434: 98 1f 02 10  	stb 0, 528(31)
802c0438: 83 a1 00 fc  	lwz 29, 252(1)
802c043c: 2c 1c 00 00  	cmpwi	28, 0
802c0440: 3b 80 00 00  	li 28, 0
802c0444: 41 82 00 30  	bt	2, 0x802c0474 <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0xf8>
802c0448: 6f a3 80 00  	xoris 3, 29, 32768
802c044c: 3c 00 43 30  	lis 0, 17200
802c0450: 90 61 00 fc  	stw 3, 252(1)
802c0454: 3c 60 80 54  	lis 3, -32684
802c0458: c8 43 9a 78  	lfd 2, -25992(3)
802c045c: 90 01 00 f8  	stw 0, 248(1)
802c0460: c0 02 fb 84  	lfs 0, -1148(2)
802c0464: c8 21 00 f8  	lfd 1, 248(1)
802c0468: ec 21 10 28  	fsubs 1, 1, 2
802c046c: fc 01 00 40  	fcmpo 0, 1, 0
802c0470: 40 80 00 90  	bf	0, 0x802c0500 <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x184>
802c0474: c0 5f 02 08  	lfs 2, 520(31)
802c0478: c0 22 fb 98  	lfs 1, -1128(2)
802c047c: fc 02 08 40  	fcmpo 0, 2, 1
802c0480: 4c 41 13 82  	cror 2, 1, 2
802c0484: 41 82 02 a8  	bt	2, 0x802c072c <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x3b0>
802c0488: c0 02 fb 9c  	lfs 0, -1124(2)
802c048c: ec 02 00 2a  	fadds 0, 2, 0
802c0490: fc 00 08 40  	fcmpo 0, 0, 1
802c0494: d0 1f 02 08  	stfs 0, 520(31)
802c0498: 4c 41 13 82  	cror 2, 1, 2
802c049c: 40 82 00 08  	bf	2, 0x802c04a4 <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x128>
802c04a0: d0 3f 02 08  	stfs 1, 520(31)
802c04a4: 88 1f 02 10  	lbz 0, 528(31)
802c04a8: 2c 00 00 01  	cmpwi	0, 1
802c04ac: 41 82 00 34  	bt	2, 0x802c04e0 <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x164>
802c04b0: 40 80 00 4c  	bf	0, 0x802c04fc <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x180>
802c04b4: 2c 00 00 00  	cmpwi	0, 0
802c04b8: 40 80 00 08  	bf	0, 0x802c04c0 <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x144>
802c04bc: 48 00 00 40  	b 0x802c04fc <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x180>
802c04c0: 80 7f 02 0c  	lwz 3, 524(31)
802c04c4: 38 81 00 8c  	addi 4, 1, 140
802c04c8: 48 14 03 69  	bl 0x80400830 calcCubeAxisZ__2MRFPC7AreaObjPQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x1400ec>
802c04cc: 80 7f 02 0c  	lwz 3, 524(31)
802c04d0: 38 80 00 00  	li 4, 0
802c04d4: 48 13 fc a5  	bl 0x80400178 getAreaObjArg__2MRFPC7AreaObjl <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x13fa34>
802c04d8: 7c 7d 1b 78  	mr	29, 3
802c04dc: 48 00 00 20  	b 0x802c04fc <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x180>
802c04e0: 80 9f 02 0c  	lwz 4, 524(31)
802c04e4: 7f e3 fb 78  	mr	3, 31
802c04e8: 38 a1 00 8c  	addi 5, 1, 140
802c04ec: 48 00 08 c9  	bl 0x802c0db4 calcCylinderToCenter__10MarioActorFPC7AreaObjPQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x670>
802c04f0: fc 00 08 1e  	fctiwz 0, 1
802c04f4: d8 01 00 f8  	stfd 0, 248(1)
802c04f8: 83 a1 00 fc  	lwz 29, 252(1)
802c04fc: 3b 80 00 01  	li 28, 1
802c0500: 38 61 00 98  	addi 3, 1, 152
802c0504: 4b ec 24 1d  	bl 0x80182920 __ct__8TriangleFv <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffec21dc>
802c0508: 6f a3 80 00  	xoris 3, 29, 32768
802c050c: 3c 00 43 30  	lis 0, 17200
802c0510: 90 61 00 fc  	stw 3, 252(1)
802c0514: 3c 60 80 54  	lis 3, -32684
802c0518: c8 23 9a 78  	lfd 1, -25992(3)
802c051c: 38 61 00 2c  	addi 3, 1, 44
802c0520: 90 01 00 f8  	stw 0, 248(1)
802c0524: 38 81 00 8c  	addi 4, 1, 140
802c0528: c8 01 00 f8  	lfd 0, 248(1)
802c052c: ec 20 08 28  	fsubs 1, 0, 1
802c0530: 4b d5 89 5d  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd58748>
802c0534: 38 61 00 80  	addi 3, 1, 128
802c0538: 38 81 00 98  	addi 4, 1, 152
802c053c: 38 bf 02 a0  	addi 5, 31, 672
802c0540: 38 c1 00 2c  	addi 6, 1, 44
802c0544: 48 12 19 bd  	bl 0x803e1f00 getFirstPolyOnLineToMap__2MRFPQ29JGeometry8TVec3<f>P8TriangleRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x1217bc>
802c0548: 2c 03 00 00  	cmpwi	3, 0
802c054c: 41 82 01 e0  	bt	2, 0x802c072c <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x3b0>
802c0550: 38 7f 01 f0  	addi 3, 31, 496
802c0554: 38 81 00 80  	addi 4, 1, 128
802c0558: 4b d5 89 21  	bl 0x80018e78 __as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd58734>
802c055c: 38 61 00 98  	addi 3, 1, 152
802c0560: 38 80 00 00  	li 4, 0
802c0564: 4b ec 26 15  	bl 0x80182b78 getNormal__8TriangleCFi <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffec2434>
802c0568: 7c 64 1b 78  	mr	4, 3
802c056c: 38 7f 01 fc  	addi 3, 31, 508
802c0570: 4b d5 89 09  	bl 0x80018e78 __as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd58734>
802c0574: 2c 1c 00 00  	cmpwi	28, 0
802c0578: 40 82 00 40  	bf	2, 0x802c05b8 <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_start+0x23c>
802c057c: 38 61 00 20  	addi 3, 1, 32
802c0580: 38 81 00 80  	addi 4, 1, 128
802c0584: 4b d5 89 6d  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd587ac>
802c0588: 38 61 00 20  	addi 3, 1, 32
802c058c: 38 9f 02 a0  	addi 4, 31, 672
802c0590: 4b d5 fb 69  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd5f9b4>
802c0594: 38 61 00 20  	addi 3, 1, 32
802c0598: 48 1f 8b 41  	bl 0x804b90d8 PSVECMag <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x1f8994>
802c059c: c0 02 fb c0  	lfs 0, -1088(2)
802c05a0: c0 62 fb bc  	lfs 3, -1092(2)
802c05a4: c0 5f 02 08  	lfs 2, 520(31)
802c05a8: ec 00 00 72  	fmuls 0, 0, 1
802c05ac: ec 23 00 b2  	fmuls 1, 3, 2
802c05b0: ec 01 00 2a  	fadds 0, 1, 0
802c05b4: d0 1f 02 08  	stfs 0, 520(31)
802c05b8: c0 3f 02 08  	lfs 1, 520(31)
802c05bc: 7f e3 fb 78  	mr	3, 31
802c05c0: 48 00 26 3d  	bl 0x802c2bfc updateRandomTexture__10MarioActorFf <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x24b8>
802c05c4: 38 61 00 74  	addi 3, 1, 116
802c05c8: 38 81 00 8c  	addi 4, 1, 140
802c05cc: 4b d5 89 25  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd587ac>
802c05d0: 38 61 00 74  	addi 3, 1, 116
802c05d4: 48 12 5f 69  	bl 0x803e653c normalizeOrZero__2MRFPQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x125df8>
802c05d8: 7f e3 fb 78  	mr	3, 31
802c05dc: 4b ff 6c 49  	bl 0x802b7224 getJ3DModel__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffff6ae0>
802c05e0: 7c 7c 1b 78  	mr	28, 3
802c05e4: 7f e3 fb 78  	mr	3, 31
802c05e8: 4b ff 6c 69  	bl 0x802b7250 getSimpleModel__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffff6b0c>
802c05ec: 7c 7d 1b 78  	mr	29, 3
802c05f0: 38 7c 00 24  	addi 3, 28, 36
802c05f4: 38 9d 00 24  	addi 4, 29, 36
802c05f8: 48 1f 7d 95  	bl 0x804b838c PSMTXCopy <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x1f7c48>
802c05fc: 3b dd 00 24  	addi 30, 29, 36
802c0600: 38 81 00 68  	addi 4, 1, 104
802c0604: 7f c3 f3 78  	mr	3, 30
802c0608: 4b d6 02 c5  	bl 0x800208cc getXDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd60188>
802c060c: 38 61 00 68  	addi 3, 1, 104
802c0610: 38 81 00 74  	addi 4, 1, 116
802c0614: 7c 65 1b 78  	mr	5, 3
802c0618: 48 12 6f 1d  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x126df0>
802c061c: 7f c3 f3 78  	mr	3, 30
802c0620: 38 81 00 5c  	addi 4, 1, 92
802c0624: 4b d6 02 c1  	bl 0x800208e4 getYDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd601a0>
802c0628: 38 61 00 5c  	addi 3, 1, 92
802c062c: 38 81 00 74  	addi 4, 1, 116
802c0630: 7c 65 1b 78  	mr	5, 3
802c0634: 48 12 6f 01  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x126df0>
802c0638: 7f c3 f3 78  	mr	3, 30
802c063c: 38 81 00 50  	addi 4, 1, 80
802c0640: 4b d6 02 bd  	bl 0x800208fc getZDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd601b8>
802c0644: 38 61 00 50  	addi 3, 1, 80
802c0648: 38 81 00 74  	addi 4, 1, 116
802c064c: 7c 65 1b 78  	mr	5, 3
802c0650: 48 12 6e e5  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x126df0>
802c0654: 7f c3 f3 78  	mr	3, 30
802c0658: 38 81 00 68  	addi 4, 1, 104
802c065c: 4b db bb 39  	bl 0x8007c194 setXDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffdbba50>
802c0660: 7f c3 f3 78  	mr	3, 30
802c0664: 38 81 00 5c  	addi 4, 1, 92
802c0668: 4b db bb 49  	bl 0x8007c1b0 setYDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffdbba6c>
802c066c: 7f c3 f3 78  	mr	3, 30
802c0670: 38 81 00 50  	addi 4, 1, 80
802c0674: 4b db bb 59  	bl 0x8007c1cc setZDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffdbba88>
802c0678: 38 61 00 14  	addi 3, 1, 20
802c067c: 38 9f 00 0c  	addi 4, 31, 12
802c0680: 4b d5 88 71  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd587ac>
802c0684: 38 61 00 14  	addi 3, 1, 20
802c0688: 38 81 00 80  	addi 4, 1, 128
802c068c: 4b d5 fa 6d  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd5f9b4>
802c0690: 7f e3 fb 78  	mr	3, 31
802c0694: 4b ff 8f 7d  	bl 0x802b9610 getGravityVec__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffff8ecc>
802c0698: 7c 64 1b 78  	mr	4, 3
802c069c: 38 61 00 14  	addi 3, 1, 20
802c06a0: 38 a1 00 44  	addi 5, 1, 68
802c06a4: 48 12 6e 91  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x126df0>
802c06a8: 38 61 00 44  	addi 3, 1, 68
802c06ac: 48 1f 8a 2d  	bl 0x804b90d8 PSVECMag <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x1f8994>
802c06b0: c0 02 fb 9c  	lfs 0, -1124(2)
802c06b4: 38 61 00 08  	addi 3, 1, 8
802c06b8: 38 81 00 74  	addi 4, 1, 116
802c06bc: ec 20 08 2a  	fadds 1, 0, 1
802c06c0: 4b d5 87 cd  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd58748>
802c06c4: 38 61 00 38  	addi 3, 1, 56
802c06c8: 38 9f 00 0c  	addi 4, 31, 12
802c06cc: 4b d5 88 25  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xffffffffffd587ac>
802c06d0: e0 01 00 38  	<unknown>
802c06d4: 7f c3 f3 78  	mr	3, 30
802c06d8: e0 21 00 08  	<unknown>
802c06dc: e0 41 80 40  	lq 2, -32704(1)
802c06e0: 10 00 08 2a  	vsel 0, 0, 1, 0
802c06e4: e0 61 80 10  	lq 3, -32752(1)
802c06e8: 10 22 18 2a  	vsel 1, 2, 3, 0
802c06ec: f0 01 00 38  	xxsel 0, 1, 0, 32
802c06f0: f0 21 80 40  	xssubsp 1, 1, 16
802c06f4: c0 21 00 38  	lfs 1, 56(1)
802c06f8: c0 41 00 3c  	lfs 2, 60(1)
802c06fc: c0 61 00 40  	lfs 3, 64(1)
802c0700: 48 12 c8 b1  	bl 0x803ecfb0 setMtxTrans__2MRFPA4_ffff <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x12c86c>
802c0704: 7f c3 f3 78  	mr	3, 30
802c0708: 7f c5 f3 78  	mr	5, 30
802c070c: 38 9f 0b c8  	addi 4, 31, 3016
802c0710: 48 1f 7c b1  	bl 0x804b83c0 PSMTXConcat <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x1f7c7c>
802c0714: 7f a3 eb 78  	mr	3, 29
802c0718: 7f 85 e3 78  	mr	5, 28
802c071c: 38 80 00 03  	li 4, 3
802c0720: 4b fe 65 35  	bl 0x802a6c54 viewCalcRef__9J3DModelXFUlP8J3DModel <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0xfffffffffffe6510>
802c0724: 38 00 00 01  	li 0, 1
802c0728: 98 1f 0a 0c  	stb 0, 2572(31)
802c072c: 39 61 01 10  	addi 11, 1, 272
802c0730: 48 25 83 21  	bl 0x80518a50 _restgpr_28 <_binary_build_mario_shadow_view_20260903_calcViewWallShadowModel_bin_end+0x25830c>
802c0734: 80 01 01 14  	lwz 0, 276(1)
802c0738: 7c 08 03 a6  	mtlr 0
802c073c: 38 21 01 10  	addi 1, 1, 272
802c0740: 4e 80 00 20  	blr
