
build/mario-shadow-view-20260903/decideShadowMode.o:	file format elf32-powerpc

Disassembly of section .data:

802c09e0 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start>:
802c09e0: 94 21 ff d0  	stwu 1, -48(1)
802c09e4: 7c 08 02 a6  	mflr 0
802c09e8: 90 01 00 34  	stw 0, 52(1)
802c09ec: 93 e1 00 2c  	stw 31, 44(1)
802c09f0: 7c 7f 1b 78  	mr	31, 3
802c09f4: 80 83 02 30  	lwz 4, 560(3)
802c09f8: 80 04 00 08  	lwz 0, 8(4)
802c09fc: 54 00 1f ff  	rlwinm. 0, 0, 3, 31, 31
802c0a00: 41 82 00 dc  	bt	2, 0x802c0adc <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0xfc>
802c0a04: 80 63 02 30  	lwz 3, 560(3)
802c0a08: 48 03 27 05  	bl 0x802f310c isNotReflectGlassGround__5MarioCFv <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_end+0x324ec>
802c0a0c: 2c 03 00 00  	cmpwi	3, 0
802c0a10: 40 82 00 cc  	bf	2, 0x802c0adc <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0xfc>
802c0a14: 80 7f 02 30  	lwz 3, 560(31)
802c0a18: 38 80 00 12  	li 4, 18
802c0a1c: 48 03 3c b5  	bl 0x802f46d0 isStatusActive__5MarioCFUl <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_end+0x33ab0>
802c0a20: 2c 03 00 00  	cmpwi	3, 0
802c0a24: 40 82 00 b8  	bf	2, 0x802c0adc <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0xfc>
802c0a28: 80 7f 02 30  	lwz 3, 560(31)
802c0a2c: a0 03 09 62  	lhz 0, 2402(3)
802c0a30: 28 00 00 0e  	cmplwi	0, 14
802c0a34: 41 82 00 0c  	bt	2, 0x802c0a40 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x60>
802c0a38: 28 00 00 21  	cmplwi	0, 33
802c0a3c: 40 82 00 a0  	bf	2, 0x802c0adc <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0xfc>
802c0a40: 38 00 00 02  	li 0, 2
802c0a44: 80 7f 02 30  	lwz 3, 560(31)
802c0a48: 98 1f 0a 08  	stb 0, 2568(31)
802c0a4c: c0 22 fb 90  	lfs 1, -1136(2)
802c0a50: c0 43 04 88  	lfs 2, 1160(3)
802c0a54: c0 02 fb 94  	lfs 0, -1132(2)
802c0a58: ec 42 08 24  	fdivs 2, 2, 1
802c0a5c: fc 02 00 40  	fcmpo 0, 2, 0
802c0a60: 4c 41 13 82  	cror 2, 1, 2
802c0a64: 40 82 00 08  	bf	2, 0x802c0a6c <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x8c>
802c0a68: fc 40 00 90  	fmr 2, 0
802c0a6c: 38 00 00 40  	li 0, 64
802c0a70: 3c 60 43 30  	lis 3, 17200
802c0a74: 90 01 00 1c  	stw 0, 28(1)
802c0a78: 3c 80 80 54  	lis 4, -32684
802c0a7c: c0 02 fb 80  	lfs 0, -1152(2)
802c0a80: 38 00 00 04  	li 0, 4
802c0a84: 90 61 00 18  	stw 3, 24(1)
802c0a88: ec 40 10 28  	fsubs 2, 0, 2
802c0a8c: c8 24 9a 70  	lfd 1, -26000(4)
802c0a90: c8 01 00 18  	lfd 0, 24(1)
802c0a94: ec 00 08 28  	fsubs 0, 0, 1
802c0a98: ec 00 00 b2  	fmuls 0, 0, 2
802c0a9c: fc 00 00 1e  	fctiwz 0, 0
802c0aa0: d8 01 00 20  	stfd 0, 32(1)
802c0aa4: 80 61 00 24  	lwz 3, 36(1)
802c0aa8: 54 63 06 3e  	clrlwi	3, 3, 24
802c0aac: 7c 09 03 a6  	mtctr 0
802c0ab0: 88 9f 0a 09  	lbz 4, 2569(31)
802c0ab4: 7c 04 18 40  	cmplw	4, 3
802c0ab8: 40 80 00 10  	bf	0, 0x802c0ac8 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0xe8>
802c0abc: 38 04 00 01  	addi 0, 4, 1
802c0ac0: 98 1f 0a 09  	stb 0, 2569(31)
802c0ac4: 48 00 00 10  	b 0x802c0ad4 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0xf4>
802c0ac8: 40 81 00 0c  	bf	1, 0x802c0ad4 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0xf4>
802c0acc: 38 04 ff ff  	addi 0, 4, -1
802c0ad0: 98 1f 0a 09  	stb 0, 2569(31)
802c0ad4: 42 00 ff dc  	bdnz 0x802c0ab0 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0xd0>
802c0ad8: 48 00 00 7c  	b 0x802c0b54 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x174>
802c0adc: 88 7f 0a 09  	lbz 3, 2569(31)
802c0ae0: 2c 03 00 00  	cmpwi	3, 0
802c0ae4: 41 82 00 68  	bt	2, 0x802c0b4c <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x16c>
802c0ae8: 38 03 ff ff  	addi 0, 3, -1
802c0aec: 54 03 06 3f  	clrlwi.	3, 0, 24
802c0af0: 98 1f 0a 09  	stb 0, 2569(31)
802c0af4: 41 82 00 0c  	bt	2, 0x802c0b00 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x120>
802c0af8: 38 03 ff ff  	addi 0, 3, -1
802c0afc: 98 1f 0a 09  	stb 0, 2569(31)
802c0b00: 88 7f 0a 09  	lbz 3, 2569(31)
802c0b04: 2c 03 00 00  	cmpwi	3, 0
802c0b08: 41 82 00 0c  	bt	2, 0x802c0b14 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x134>
802c0b0c: 38 03 ff ff  	addi 0, 3, -1
802c0b10: 98 1f 0a 09  	stb 0, 2569(31)
802c0b14: 88 7f 0a 09  	lbz 3, 2569(31)
802c0b18: 2c 03 00 00  	cmpwi	3, 0
802c0b1c: 41 82 00 0c  	bt	2, 0x802c0b28 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x148>
802c0b20: 38 03 ff ff  	addi 0, 3, -1
802c0b24: 98 1f 0a 09  	stb 0, 2569(31)
802c0b28: 88 1f 0a 09  	lbz 0, 2569(31)
802c0b2c: 2c 00 00 00  	cmpwi	0, 0
802c0b30: 40 82 00 10  	bf	2, 0x802c0b40 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x160>
802c0b34: 38 00 00 01  	li 0, 1
802c0b38: 98 1f 0a 08  	stb 0, 2568(31)
802c0b3c: 48 00 00 18  	b 0x802c0b54 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x174>
802c0b40: 38 00 00 02  	li 0, 2
802c0b44: 98 1f 0a 08  	stb 0, 2568(31)
802c0b48: 48 00 00 0c  	b 0x802c0b54 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x174>
802c0b4c: 38 00 00 01  	li 0, 1
802c0b50: 98 1f 0a 08  	stb 0, 2568(31)
802c0b54: 80 7f 02 30  	lwz 3, 560(31)
802c0b58: 48 03 94 cd  	bl 0x802fa024 isSwimming__5MarioCFv <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_end+0x39404>
802c0b5c: 2c 03 00 00  	cmpwi	3, 0
802c0b60: 41 82 00 a4  	bt	2, 0x802c0c04 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x224>
802c0b64: 38 7f 0f 78  	addi 3, 31, 3960
802c0b68: 48 12 f9 69  	bl 0x803f04d0 isInWater__2MRFRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_end+0x12f8b0>
802c0b6c: 2c 03 00 00  	cmpwi	3, 0
802c0b70: 41 82 00 94  	bt	2, 0x802c0c04 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x224>
802c0b74: 88 1f 0a 25  	lbz 0, 2597(31)
802c0b78: 2c 00 00 00  	cmpwi	0, 0
802c0b7c: 40 82 00 3c  	bf	2, 0x802c0bb8 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x1d8>
802c0b80: 7f e3 fb 78  	mr	3, 31
802c0b84: 4b ff 8a 95  	bl 0x802b9618 getGravityVector__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_end+0xffffffffffff89f8>
802c0b88: e0 03 00 00  	lq 0, 0(3)
802c0b8c: 38 81 00 08  	addi 4, 1, 8
802c0b90: 10 00 00 50  	<unknown>
802c0b94: f0 04 00 00  	xsaddsp 0, 4, 0
802c0b98: c0 03 00 08  	lfs 0, 8(3)
802c0b9c: 38 7f 0f 9c  	addi 3, 31, 3996
802c0ba0: fc 00 00 50  	fneg 0, 0
802c0ba4: d0 01 00 10  	stfs 0, 16(1)
802c0ba8: 4b d5 c7 01  	bl 0x8001d2a8 dot__Q29JGeometry8TVec3<f>CFRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_end+0xffffffffffd5c688>
802c0bac: c0 02 fb 84  	lfs 0, -1148(2)
802c0bb0: fc 01 00 40  	fcmpo 0, 1, 0
802c0bb4: 40 81 00 0c  	bf	1, 0x802c0bc0 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x1e0>
802c0bb8: 38 00 00 03  	li 0, 3
802c0bbc: 98 1f 0a 08  	stb 0, 2568(31)
802c0bc0: 88 1f 0a 24  	lbz 0, 2596(31)
802c0bc4: 2c 00 00 00  	cmpwi	0, 0
802c0bc8: 40 82 00 34  	bf	2, 0x802c0bfc <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x21c>
802c0bcc: 80 7f 02 30  	lwz 3, 560(31)
802c0bd0: 80 63 08 84  	lwz 3, 2180(3)
802c0bd4: 48 00 07 59  	bl 0x802c132c getWaterEdgeDist__9MarioSwimCFv <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_end+0x70c>
802c0bd8: c0 02 fb 84  	lfs 0, -1148(2)
802c0bdc: fc 01 00 40  	fcmpo 0, 1, 0
802c0be0: 40 81 00 24  	bf	1, 0x802c0c04 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x224>
802c0be4: 80 7f 02 30  	lwz 3, 560(31)
802c0be8: 80 63 08 84  	lwz 3, 2180(3)
802c0bec: 48 00 07 41  	bl 0x802c132c getWaterEdgeDist__9MarioSwimCFv <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_end+0x70c>
802c0bf0: c0 02 fb 98  	lfs 0, -1128(2)
802c0bf4: fc 01 00 40  	fcmpo 0, 1, 0
802c0bf8: 40 80 00 0c  	bf	0, 0x802c0c04 <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_start+0x224>
802c0bfc: 38 00 00 07  	li 0, 7
802c0c00: 98 1f 0a 08  	stb 0, 2568(31)
802c0c04: 80 7f 02 14  	lwz 3, 532(31)
802c0c08: 48 05 33 c1  	bl 0x80313fc8 setUpdateFlag__15CollisionShadowFv <_binary_build_mario_shadow_view_20260903_decideShadowMode_bin_end+0x533a8>
802c0c0c: 80 01 00 34  	lwz 0, 52(1)
802c0c10: 83 e1 00 2c  	lwz 31, 44(1)
802c0c14: 7c 08 03 a6  	mtlr 0
802c0c18: 38 21 00 30  	addi 1, 1, 48
802c0c1c: 4e 80 00 20  	blr
