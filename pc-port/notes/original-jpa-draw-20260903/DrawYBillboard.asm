
build/original-jpa-overwrite-20260903/DrawYBillboard.o:	file format elf32-powerpc

Disassembly of section .data:

803a66b8 <_binary_build_original_jpa_overwrite_20260903_DrawYBillboard_bin_start>:
803a66b8: 94 21 ff a0  	stwu 1, -96(1)
803a66bc: 7c 08 02 a6  	mflr 0
803a66c0: 90 01 00 64  	stw 0, 100(1)
803a66c4: 93 e1 00 5c  	stw 31, 92(1)
803a66c8: 7c 9f 23 78  	mr	31, 4
803a66cc: 93 c1 00 58  	stw 30, 88(1)
803a66d0: 7c 7e 1b 78  	mr	30, 3
803a66d4: 80 04 00 7c  	lwz 0, 124(4)
803a66d8: 54 00 07 39  	rlwinm. 0, 0, 0, 28, 28
803a66dc: 40 82 00 ec  	bf	2, 0x803a67c8 <_binary_build_original_jpa_overwrite_20260903_DrawYBillboard_bin_start+0x110>
803a66e0: c0 22 18 2c  	lfs 1, 6188(2)
803a66e4: 38 61 00 14  	addi 3, 1, 20
803a66e8: c0 5e 01 98  	lfs 2, 408(30)
803a66ec: c0 7e 01 a8  	lfs 3, 424(30)
803a66f0: 4b c7 28 39  	bl 0x80018f28 <_binary_build_original_jpa_overwrite_20260903_DrawYBillboard_bin_end+0xffffffffffc72748>
803a66f4: c0 22 18 48  	lfs 1, 6216(2)
803a66f8: 38 61 00 14  	addi 3, 1, 20
803a66fc: 48 04 08 ed  	bl 0x803e6fe8 <_binary_build_original_jpa_overwrite_20260903_DrawYBillboard_bin_end+0x40808>
803a6700: 2c 03 00 00  	cmpwi	3, 0
803a6704: 40 82 00 c4  	bf	2, 0x803a67c8 <_binary_build_original_jpa_overwrite_20260903_DrawYBillboard_bin_start+0x110>
803a6708: 7f e4 fb 78  	mr	4, 31
803a670c: 38 7e 01 84  	addi 3, 30, 388
803a6710: 38 a1 00 08  	addi 5, 1, 8
803a6714: 48 11 25 ad  	bl 0x804b8cc0 <_binary_build_original_jpa_overwrite_20260903_DrawYBillboard_bin_end+0x1124e0>
803a6718: c0 7e 01 44  	lfs 3, 324(30)
803a671c: 38 61 00 20  	addi 3, 1, 32
803a6720: c0 1f 00 60  	lfs 0, 96(31)
803a6724: 38 80 00 00  	li 4, 0
803a6728: c0 5e 01 48  	lfs 2, 328(30)
803a672c: ec 63 00 32  	fmuls 3, 3, 0
803a6730: c0 3f 00 64  	lfs 1, 100(31)
803a6734: c0 01 00 08  	lfs 0, 8(1)
803a6738: ec 82 00 72  	fmuls 4, 2, 1
803a673c: c0 41 00 0c  	lfs 2, 12(1)
803a6740: d0 01 00 2c  	stfs 0, 44(1)
803a6744: c0 21 00 10  	lfs 1, 16(1)
803a6748: d0 61 00 20  	stfs 3, 32(1)
803a674c: c0 02 18 2c  	lfs 0, 6188(2)
803a6750: c0 7e 01 68  	lfs 3, 360(30)
803a6754: ec 63 01 32  	fmuls 3, 3, 4
803a6758: d0 61 00 34  	stfs 3, 52(1)
803a675c: c0 7e 01 6c  	lfs 3, 364(30)
803a6760: d0 61 00 38  	stfs 3, 56(1)
803a6764: d0 41 00 3c  	stfs 2, 60(1)
803a6768: c0 5e 01 78  	lfs 2, 376(30)
803a676c: ec 42 01 32  	fmuls 2, 2, 4
803a6770: d0 41 00 44  	stfs 2, 68(1)
803a6774: c0 5e 01 7c  	lfs 2, 380(30)
803a6778: d0 41 00 48  	stfs 2, 72(1)
803a677c: d0 21 00 4c  	stfs 1, 76(1)
803a6780: d0 01 00 40  	stfs 0, 64(1)
803a6784: d0 01 00 30  	stfs 0, 48(1)
803a6788: d0 01 00 28  	stfs 0, 40(1)
803a678c: d0 01 00 24  	stfs 0, 36(1)
803a6790: 48 11 9e d1  	bl 0x804c0660 <_binary_build_original_jpa_overwrite_20260903_DrawYBillboard_bin_end+0x119e80>
803a6794: 80 1e 02 10  	lwz 0, 528(30)
803a6798: 3c a0 80 5e  	lis 5, -32674
803a679c: 38 a5 bd 68  	addi 5, 5, -17048
803a67a0: 7f c3 f3 78  	mr	3, 30
803a67a4: 54 00 10 3a  	slwi 0, 0, 2
803a67a8: 38 81 00 20  	addi 4, 1, 32
803a67ac: 7d 85 00 2e  	lwzx 12, 5, 0
803a67b0: 7d 89 03 a6  	mtctr 12
803a67b4: 4e 80 04 21  	bctrl
803a67b8: 3c 60 80 5e  	lis 3, -32674
803a67bc: 38 80 00 20  	li 4, 32
803a67c0: 38 63 bd 20  	addi 3, 3, -17120
803a67c4: 48 11 9b 6d  	bl 0x804c0330 <_binary_build_original_jpa_overwrite_20260903_DrawYBillboard_bin_end+0x119b50>
803a67c8: 80 01 00 64  	lwz 0, 100(1)
803a67cc: 83 e1 00 5c  	lwz 31, 92(1)
803a67d0: 83 c1 00 58  	lwz 30, 88(1)
803a67d4: 7c 08 03 a6  	mtlr 0
803a67d8: 38 21 00 60  	addi 1, 1, 96
803a67dc: 4e 80 00 20  	blr
