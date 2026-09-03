
build/original-jpa-overwrite-20260903/DrawRotYBillboard.o:	file format elf32-powerpc

Disassembly of section .data:

803a67e0 <_binary_build_original_jpa_overwrite_20260903_DrawRotYBillboard_bin_start>:
803a67e0: 94 21 ff a0  	stwu 1, -96(1)
803a67e4: 7c 08 02 a6  	mflr 0
803a67e8: 90 01 00 64  	stw 0, 100(1)
803a67ec: 93 e1 00 5c  	stw 31, 92(1)
803a67f0: 7c 9f 23 78  	mr	31, 4
803a67f4: 93 c1 00 58  	stw 30, 88(1)
803a67f8: 7c 7e 1b 78  	mr	30, 3
803a67fc: 80 04 00 7c  	lwz 0, 124(4)
803a6800: 54 00 07 39  	rlwinm. 0, 0, 0, 28, 28
803a6804: 40 82 01 20  	bf	2, 0x803a6924 <_binary_build_original_jpa_overwrite_20260903_DrawRotYBillboard_bin_start+0x144>
803a6808: c0 22 18 2c  	lfs 1, 6188(2)
803a680c: 38 61 00 14  	addi 3, 1, 20
803a6810: c0 5e 01 98  	lfs 2, 408(30)
803a6814: c0 7e 01 a8  	lfs 3, 424(30)
803a6818: 4b c7 27 11  	bl 0x80018f28 <_binary_build_original_jpa_overwrite_20260903_DrawRotYBillboard_bin_end+0xffffffffffc725ec>
803a681c: c0 22 18 48  	lfs 1, 6216(2)
803a6820: 38 61 00 14  	addi 3, 1, 20
803a6824: 48 04 07 c5  	bl 0x803e6fe8 <_binary_build_original_jpa_overwrite_20260903_DrawRotYBillboard_bin_end+0x406ac>
803a6828: 2c 03 00 00  	cmpwi	3, 0
803a682c: 40 82 00 f8  	bf	2, 0x803a6924 <_binary_build_original_jpa_overwrite_20260903_DrawRotYBillboard_bin_start+0x144>
803a6830: 7f e4 fb 78  	mr	4, 31
803a6834: 38 7e 01 84  	addi 3, 30, 388
803a6838: 38 a1 00 08  	addi 5, 1, 8
803a683c: 48 11 24 85  	bl 0x804b8cc0 <_binary_build_original_jpa_overwrite_20260903_DrawRotYBillboard_bin_end+0x112384>
803a6840: a8 1f 00 88  	lha 0, 136(31)
803a6844: 3c 60 80 61  	lis 3, -32671
803a6848: c0 7e 01 44  	lfs 3, 324(30)
803a684c: 38 63 fc 80  	addi 3, 3, -896
803a6850: c0 5f 00 60  	lfs 2, 96(31)
803a6854: 54 00 0b f8  	rlwinm 0, 0, 1, 15, 28
803a6858: c0 3e 01 48  	lfs 1, 328(30)
803a685c: 7c 83 02 14  	add 4, 3, 0
803a6860: ec 83 00 b2  	fmuls 4, 3, 2
803a6864: c0 64 00 04  	lfs 3, 4(4)
803a6868: c0 1f 00 64  	lfs 0, 100(31)
803a686c: 38 80 00 00  	li 4, 0
803a6870: 7c 43 04 2e  	lfsx 2, 3, 0
803a6874: 38 61 00 20  	addi 3, 1, 32
803a6878: ec 21 00 32  	fmuls 1, 1, 0
803a687c: c1 be 01 78  	lfs 13, 376(30)
803a6880: fc 00 10 50  	fneg 0, 2
803a6884: c1 9e 01 68  	lfs 12, 360(30)
803a6888: ec 42 01 32  	fmuls 2, 2, 4
803a688c: c1 02 18 2c  	lfs 8, 6188(2)
803a6890: ed 43 01 32  	fmuls 10, 3, 4
803a6894: c0 e1 00 08  	lfs 7, 8(1)
803a6898: ed 63 00 72  	fmuls 11, 3, 1
803a689c: c0 61 00 0c  	lfs 3, 12(1)
803a68a0: ed 20 00 72  	fmuls 9, 0, 1
803a68a4: c0 01 00 10  	lfs 0, 16(1)
803a68a8: ec c2 03 32  	fmuls 6, 2, 12
803a68ac: d1 41 00 20  	stfs 10, 32(1)
803a68b0: ec ab 03 32  	fmuls 5, 11, 12
803a68b4: fc 80 68 50  	fneg 4, 13
803a68b8: d1 21 00 24  	stfs 9, 36(1)
803a68bc: ec 42 03 72  	fmuls 2, 2, 13
803a68c0: ec 2b 03 72  	fmuls 1, 11, 13
803a68c4: d1 01 00 28  	stfs 8, 40(1)
803a68c8: d0 e1 00 2c  	stfs 7, 44(1)
803a68cc: d0 c1 00 30  	stfs 6, 48(1)
803a68d0: d0 a1 00 34  	stfs 5, 52(1)
803a68d4: d0 81 00 38  	stfs 4, 56(1)
803a68d8: d0 61 00 3c  	stfs 3, 60(1)
803a68dc: d0 41 00 40  	stfs 2, 64(1)
803a68e0: d0 21 00 44  	stfs 1, 68(1)
803a68e4: d1 81 00 48  	stfs 12, 72(1)
803a68e8: d0 01 00 4c  	stfs 0, 76(1)
803a68ec: 48 11 9d 75  	bl 0x804c0660 <_binary_build_original_jpa_overwrite_20260903_DrawRotYBillboard_bin_end+0x119d24>
803a68f0: 80 1e 02 10  	lwz 0, 528(30)
803a68f4: 3c a0 80 5e  	lis 5, -32674
803a68f8: 38 a5 bd 68  	addi 5, 5, -17048
803a68fc: 7f c3 f3 78  	mr	3, 30
803a6900: 54 00 10 3a  	slwi 0, 0, 2
803a6904: 38 81 00 20  	addi 4, 1, 32
803a6908: 7d 85 00 2e  	lwzx 12, 5, 0
803a690c: 7d 89 03 a6  	mtctr 12
803a6910: 4e 80 04 21  	bctrl
803a6914: 3c 60 80 5e  	lis 3, -32674
803a6918: 38 80 00 20  	li 4, 32
803a691c: 38 63 bd 20  	addi 3, 3, -17120
803a6920: 48 11 9a 11  	bl 0x804c0330 <_binary_build_original_jpa_overwrite_20260903_DrawRotYBillboard_bin_end+0x1199f4>
803a6924: 80 01 00 64  	lwz 0, 100(1)
803a6928: 83 e1 00 5c  	lwz 31, 92(1)
803a692c: 83 c1 00 58  	lwz 30, 88(1)
803a6930: 7c 08 03 a6  	mtlr 0
803a6934: 38 21 00 60  	addi 1, 1, 96
803a6938: 4e 80 00 20  	blr
