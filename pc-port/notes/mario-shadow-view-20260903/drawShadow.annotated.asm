
build/mario-shadow-view-20260903/drawShadow.o:	file format elf32-powerpc

Disassembly of section .data:

802c0744 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start>:
802c0744: 94 21 ff b0  	stwu 1, -80(1)
802c0748: 7c 08 02 a6  	mflr 0
802c074c: 90 01 00 54  	stw 0, 84(1)
802c0750: 39 61 00 50  	addi 11, 1, 80
802c0754: 48 25 82 b5  	bl 0x80518a08 _savegpr_29 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x258094>
802c0758: 88 03 04 82  	lbz 0, 1154(3)
802c075c: 3f e0 80 5c  	lis 31, -32676
802c0760: 7c 7e 1b 78  	mr	30, 3
802c0764: 38 80 00 00  	li 4, 0
802c0768: 2c 00 00 00  	cmpwi	0, 0
802c076c: 3b ff 8e 00  	addi 31, 31, -29184
802c0770: 40 82 00 10  	bf	2, 0x802c0780 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x3c>
802c0774: 88 03 04 81  	lbz 0, 1153(3)
802c0778: 2c 00 00 00  	cmpwi	0, 0
802c077c: 41 82 00 08  	bt	2, 0x802c0784 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x40>
802c0780: 38 80 00 01  	li 4, 1
802c0784: 2c 04 00 00  	cmpwi	4, 0
802c0788: 40 82 01 d4  	bf	2, 0x802c095c <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x218>
802c078c: 88 03 0a 08  	lbz 0, 2568(3)
802c0790: 54 00 07 ff  	clrlwi.	0, 0, 31
802c0794: 41 82 01 c8  	bt	2, 0x802c095c <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x218>
802c0798: a0 03 03 d4  	lhz 0, 980(3)
802c079c: 28 00 00 06  	cmplwi	0, 6
802c07a0: 41 82 01 bc  	bt	2, 0x802c095c <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x218>
802c07a4: 88 03 09 f1  	lbz 0, 2545(3)
802c07a8: 2c 00 00 00  	cmpwi	0, 0
802c07ac: 41 82 00 14  	bt	2, 0x802c07c0 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x7c>
802c07b0: 80 83 02 30  	lwz 4, 560(3)
802c07b4: 80 04 00 08  	lwz 0, 8(4)
802c07b8: 54 00 17 ff  	rlwinm. 0, 0, 2, 31, 31
802c07bc: 40 82 01 a0  	bf	2, 0x802c095c <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x218>
802c07c0: 80 63 02 30  	lwz 3, 560(3)
802c07c4: 38 80 00 12  	li 4, 18
802c07c8: 48 03 3f 09  	bl 0x802f46d0 isStatusActive__5MarioCFUl <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x33d5c>
802c07cc: 2c 03 00 00  	cmpwi	3, 0
802c07d0: 41 82 00 10  	bt	2, 0x802c07e0 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x9c>
802c07d4: 88 1e 01 a1  	lbz 0, 417(30)
802c07d8: 2c 00 00 00  	cmpwi	0, 0
802c07dc: 40 82 01 80  	bf	2, 0x802c095c <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x218>
802c07e0: 80 9e 09 24  	lwz 4, 2340(30)
802c07e4: 2c 04 00 00  	cmpwi	4, 0
802c07e8: 41 82 00 14  	bt	2, 0x802c07fc <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0xb8>
802c07ec: 7f c3 f3 78  	mr	3, 30
802c07f0: 48 00 3f e5  	bl 0x802c47d4 selectNoShadow__10MarioActorCFPC9HitSensor <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x3e60>
802c07f4: 2c 03 00 00  	cmpwi	3, 0
802c07f8: 40 82 01 64  	bf	2, 0x802c095c <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x218>
802c07fc: 80 7e 02 30  	lwz 3, 560(30)
802c0800: 80 03 00 08  	lwz 0, 8(3)
802c0804: 54 00 87 ff  	rlwinm. 0, 0, 16, 31, 31
802c0808: 41 82 00 10  	bt	2, 0x802c0818 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0xd4>
802c080c: a0 03 05 44  	lhz 0, 1348(3)
802c0810: 28 00 00 01  	cmplwi	0, 1
802c0814: 41 81 01 48  	bt	1, 0x802c095c <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x218>
802c0818: 7f c3 f3 78  	mr	3, 30
802c081c: 4b ff 6a 35  	bl 0x802b7250 getSimpleModel__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0xffffffffffff68dc>
802c0820: c0 3e 00 24  	lfs 1, 36(30)
802c0824: 7c 7d 1b 78  	mr	29, 3
802c0828: c0 5e 00 28  	lfs 2, 40(30)
802c082c: 38 61 00 08  	addi 3, 1, 8
802c0830: c0 7e 00 2c  	lfs 3, 44(30)
802c0834: 48 1f 80 75  	bl 0x804b88a8 PSMTXScale <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x1f7f34>
802c0838: 38 61 00 08  	addi 3, 1, 8
802c083c: 38 9e 0b c8  	addi 4, 30, 3016
802c0840: 7c 65 1b 78  	mr	5, 3
802c0844: 48 1f 7b 7d  	bl 0x804b83c0 PSMTXConcat <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x1f7a4c>
802c0848: 7f c3 f3 78  	mr	3, 30
802c084c: 4b ff 69 d9  	bl 0x802b7224 getJ3DModel__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0xffffffffffff68b0>
802c0850: 38 63 00 24  	addi 3, 3, 36
802c0854: 38 81 00 08  	addi 4, 1, 8
802c0858: 38 bd 00 24  	addi 5, 29, 36
802c085c: 48 1f 7b 65  	bl 0x804b83c0 PSMTXConcat <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x1f7a4c>
802c0860: 7f c3 f3 78  	mr	3, 30
802c0864: 4b ff 69 c1  	bl 0x802b7224 getJ3DModel__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0xffffffffffff68b0>
802c0868: 7c 66 1b 78  	mr	6, 3
802c086c: 80 7e 02 14  	lwz 3, 532(30)
802c0870: 7f a4 eb 78  	mr	4, 29
802c0874: 38 a0 00 02  	li 5, 2
802c0878: 48 05 37 95  	bl 0x8031400c calcView__15CollisionShadowFP9J3DModelXUlP9J3DModelX <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x53698>
802c087c: 80 7e 02 30  	lwz 3, 560(30)
802c0880: 88 03 07 35  	lbz 0, 1845(3)
802c0884: 28 00 00 b4  	cmplwi	0, 180
802c0888: 41 81 00 d4  	bt	1, 0x802c095c <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x218>
802c088c: 28 00 00 80  	cmplwi	0, 128
802c0890: 40 81 00 10  	bf	1, 0x802c08a0 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x15c>
802c0894: 7f a3 eb 78  	mr	3, 29
802c0898: 38 8d 8c 20  	addi 4, 13, -29664
802c089c: 48 11 63 8d  	bl 0x803d6c28 hideJointAndChildren__2MRFP8J3DModelPCc <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x1162b4>
802c08a0: 80 7e 02 30  	lwz 3, 560(30)
802c08a4: 88 03 07 35  	lbz 0, 1845(3)
802c08a8: 28 00 00 96  	cmplwi	0, 150
802c08ac: 40 81 00 1c  	bf	1, 0x802c08c8 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x184>
802c08b0: 7f a3 eb 78  	mr	3, 29
802c08b4: 38 9f 00 5a  	addi 4, 31, 90
802c08b8: 48 11 63 71  	bl 0x803d6c28 hideJointAndChildren__2MRFP8J3DModelPCc <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x1162b4>
802c08bc: 7f a3 eb 78  	mr	3, 29
802c08c0: 38 9f 00 64  	addi 4, 31, 100
802c08c4: 48 11 63 65  	bl 0x803d6c28 hideJointAndChildren__2MRFP8J3DModelPCc <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x1162b4>
802c08c8: 88 1e 0a 58  	lbz 0, 2648(30)
802c08cc: 2c 00 00 00  	cmpwi	0, 0
802c08d0: 40 82 00 14  	bf	2, 0x802c08e4 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x1a0>
802c08d4: 80 7e 02 30  	lwz 3, 560(30)
802c08d8: 88 03 07 35  	lbz 0, 1845(3)
802c08dc: 28 00 00 78  	cmplwi	0, 120
802c08e0: 40 81 00 1c  	bf	1, 0x802c08fc <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x1b8>
802c08e4: 7f a3 eb 78  	mr	3, 29
802c08e8: 38 9f 00 6e  	addi 4, 31, 110
802c08ec: 48 11 62 7d  	bl 0x803d6b68 hideJoint__2MRFP8J3DModelPCc <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x1161f4>
802c08f0: 7f a3 eb 78  	mr	3, 29
802c08f4: 38 9f 00 75  	addi 4, 31, 117
802c08f8: 48 11 62 71  	bl 0x803d6b68 hideJoint__2MRFP8J3DModelPCc <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x1161f4>
802c08fc: 80 7e 02 14  	lwz 3, 532(30)
802c0900: 7f a4 eb 78  	mr	4, 29
802c0904: 38 be 00 0c  	addi 5, 30, 12
802c0908: 48 05 37 e1  	bl 0x803140e8 drawAndCaptureTex__15CollisionShadowFP9J3DModelXRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x53774>
802c090c: 80 7e 02 14  	lwz 3, 532(30)
802c0910: 48 05 3b 41  	bl 0x80314450 clearAlphaBuffer__15CollisionShadowFv <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x53adc>
802c0914: 88 1e 09 f1  	lbz 0, 2545(30)
802c0918: 2c 00 00 00  	cmpwi	0, 0
802c091c: 41 82 00 14  	bt	2, 0x802c0930 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x1ec>
802c0920: 80 7e 02 14  	lwz 3, 532(30)
802c0924: 38 00 00 00  	li 0, 0
802c0928: 98 03 03 07  	stb 0, 775(3)
802c092c: 48 00 00 10  	b 0x802c093c <_binary_build_mario_shadow_view_20260903_drawShadow_bin_start+0x1f8>
802c0930: 80 7e 02 14  	lwz 3, 532(30)
802c0934: 38 00 00 01  	li 0, 1
802c0938: 98 03 03 07  	stb 0, 775(3)
802c093c: 80 7e 02 14  	lwz 3, 532(30)
802c0940: 81 83 00 00  	lwz 12, 0(3)
802c0944: 81 8c 00 18  	lwz 12, 24(12)
802c0948: 7d 89 03 a6  	mtctr 12
802c094c: 4e 80 04 21  	bctrl
802c0950: 7f a3 eb 78  	mr	3, 29
802c0954: 38 9f 00 7c  	addi 4, 31, 124
802c0958: 48 11 64 15  	bl 0x803d6d6c showJointAndChildren__2MRFP8J3DModelPCc <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x1163f8>
802c095c: 39 61 00 50  	addi 11, 1, 80
802c0960: 48 25 80 f5  	bl 0x80518a54 _restgpr_29 <_binary_build_mario_shadow_view_20260903_drawShadow_bin_end+0x2580e0>
802c0964: 80 01 00 54  	lwz 0, 84(1)
802c0968: 7c 08 03 a6  	mtlr 0
802c096c: 38 21 00 50  	addi 1, 1, 80
802c0970: 4e 80 00 20  	blr
