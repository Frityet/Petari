
build/original-jpa-overwrite-20260903/DrawRotDirection.o:	file format elf32-powerpc

Disassembly of section .data:

803a52f8 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_start>:
803a52f8: 94 21 ff 50  	stwu 1, -176(1)
803a52fc: 7c 08 02 a6  	mflr 0
803a5300: 90 01 00 b4  	stw 0, 180(1)
803a5304: db e1 00 a0  	stfd 31, 160(1)
803a5308: f3 e1 00 a8  	<unknown>
803a530c: db c1 00 90  	stfd 30, 144(1)
803a5310: f3 c1 00 98  	xscmpgedp 30, 1, 0
803a5314: 39 61 00 90  	addi 11, 1, 144
803a5318: 48 17 36 f1  	bl 0x80518a08 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x1734ec>
803a531c: 80 04 00 7c  	lwz 0, 124(4)
803a5320: 3f e0 80 5e  	lis 31, -32674
803a5324: 7c 7d 1b 78  	mr	29, 3
803a5328: 7c 9e 23 78  	mr	30, 4
803a532c: 54 00 07 39  	rlwinm. 0, 0, 0, 28, 28
803a5330: 3b ff bd 20  	addi 31, 31, -17120
803a5334: 40 82 01 c0  	bf	2, 0x803a54f4 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_start+0x1fc>
803a5338: a8 c4 00 88  	lha 6, 136(4)
803a533c: 3c a0 80 61  	lis 5, -32671
803a5340: 80 03 02 00  	lwz 0, 512(3)
803a5344: 38 a5 fc 80  	addi 5, 5, -896
803a5348: 54 c8 0b f8  	rlwinm 8, 6, 1, 15, 28
803a534c: 38 df 00 54  	addi 6, 31, 84
803a5350: 54 00 10 3a  	slwi 0, 0, 2
803a5354: 7f e5 44 2e  	lfsx 31, 5, 8
803a5358: 7c e5 42 14  	add 7, 5, 8
803a535c: 7d 86 00 2e  	lwzx 12, 6, 0
803a5360: 38 a1 00 14  	addi 5, 1, 20
803a5364: c3 c7 00 04  	lfs 30, 4(7)
803a5368: 7d 89 03 a6  	mtctr 12
803a536c: 4e 80 04 21  	bctrl
803a5370: c0 22 18 48  	lfs 1, 6216(2)
803a5374: 38 61 00 14  	addi 3, 1, 20
803a5378: 48 04 1c 71  	bl 0x803e6fe8 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x41acc>
803a537c: 2c 03 00 00  	cmpwi	3, 0
803a5380: 40 82 01 74  	bf	2, 0x803a54f4 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_start+0x1fc>
803a5384: 38 61 00 14  	addi 3, 1, 20
803a5388: 48 04 10 29  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x40e94>
803a538c: 38 7e 00 54  	addi 3, 30, 84
803a5390: 38 81 00 14  	addi 4, 1, 20
803a5394: 38 a1 00 08  	addi 5, 1, 8
803a5398: 48 11 3d a5  	bl 0x804b913c <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x113c20>
803a539c: c0 22 18 48  	lfs 1, 6216(2)
803a53a0: 38 61 00 08  	addi 3, 1, 8
803a53a4: 48 04 1c 45  	bl 0x803e6fe8 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x41acc>
803a53a8: 2c 03 00 00  	cmpwi	3, 0
803a53ac: 40 82 01 48  	bf	2, 0x803a54f4 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_start+0x1fc>
803a53b0: 38 61 00 08  	addi 3, 1, 8
803a53b4: 48 04 0f fd  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x40e94>
803a53b8: 38 61 00 14  	addi 3, 1, 20
803a53bc: 38 81 00 08  	addi 4, 1, 8
803a53c0: 38 be 00 54  	addi 5, 30, 84
803a53c4: 48 11 3d 79  	bl 0x804b913c <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x113c20>
803a53c8: 38 7e 00 54  	addi 3, 30, 84
803a53cc: 48 04 0f e5  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x40e94>
803a53d0: 80 1d 02 04  	lwz 0, 516(29)
803a53d4: 38 7f 00 68  	addi 3, 31, 104
803a53d8: c0 bd 01 44  	lfs 5, 324(29)
803a53dc: fc 20 f8 90  	fmr 1, 31
803a53e0: 54 00 10 3a  	slwi 0, 0, 2
803a53e4: c0 9e 00 60  	lfs 4, 96(30)
803a53e8: 7d 83 00 2e  	lwzx 12, 3, 0
803a53ec: fc 40 f0 90  	fmr 2, 30
803a53f0: c0 7d 01 48  	lfs 3, 328(29)
803a53f4: c0 1e 00 64  	lfs 0, 100(30)
803a53f8: ef c5 01 32  	fmuls 30, 5, 4
803a53fc: 38 61 00 50  	addi 3, 1, 80
803a5400: ef e3 00 32  	fmuls 31, 3, 0
803a5404: 7d 89 03 a6  	mtctr 12
803a5408: 4e 80 04 21  	bctrl
803a540c: 80 1d 02 08  	lwz 0, 520(29)
803a5410: 38 9f 00 7c  	addi 4, 31, 124
803a5414: fc 20 f0 90  	fmr 1, 30
803a5418: 38 61 00 50  	addi 3, 1, 80
803a541c: 54 00 10 3a  	slwi 0, 0, 2
803a5420: fc 40 f8 90  	fmr 2, 31
803a5424: 7d 84 00 2e  	lwzx 12, 4, 0
803a5428: 7d 89 03 a6  	mtctr 12
803a542c: 4e 80 04 21  	bctrl
803a5430: c0 5e 00 54  	lfs 2, 84(30)
803a5434: 38 81 00 50  	addi 4, 1, 80
803a5438: c0 21 00 14  	lfs 1, 20(1)
803a543c: 7c 85 23 78  	mr	5, 4
803a5440: c0 01 00 08  	lfs 0, 8(1)
803a5444: 38 61 00 20  	addi 3, 1, 32
803a5448: d0 41 00 20  	stfs 2, 32(1)
803a544c: c0 61 00 18  	lfs 3, 24(1)
803a5450: d0 21 00 24  	stfs 1, 36(1)
803a5454: c0 41 00 0c  	lfs 2, 12(1)
803a5458: d0 01 00 28  	stfs 0, 40(1)
803a545c: c0 21 00 1c  	lfs 1, 28(1)
803a5460: c0 9e 00 00  	lfs 4, 0(30)
803a5464: c0 01 00 10  	lfs 0, 16(1)
803a5468: d0 81 00 2c  	stfs 4, 44(1)
803a546c: c0 9e 00 58  	lfs 4, 88(30)
803a5470: d0 81 00 30  	stfs 4, 48(1)
803a5474: d0 61 00 34  	stfs 3, 52(1)
803a5478: d0 41 00 38  	stfs 2, 56(1)
803a547c: c0 5e 00 04  	lfs 2, 4(30)
803a5480: d0 41 00 3c  	stfs 2, 60(1)
803a5484: c0 5e 00 5c  	lfs 2, 92(30)
803a5488: d0 41 00 40  	stfs 2, 64(1)
803a548c: d0 21 00 44  	stfs 1, 68(1)
803a5490: d0 01 00 48  	stfs 0, 72(1)
803a5494: c0 1e 00 08  	lfs 0, 8(30)
803a5498: d0 01 00 4c  	stfs 0, 76(1)
803a549c: 48 11 2f 25  	bl 0x804b83c0 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x112ea4>
803a54a0: 38 7d 01 84  	addi 3, 29, 388
803a54a4: 38 81 00 50  	addi 4, 1, 80
803a54a8: 38 a1 00 20  	addi 5, 1, 32
803a54ac: 48 11 2f 15  	bl 0x804b83c0 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x112ea4>
803a54b0: 38 61 00 20  	addi 3, 1, 32
803a54b4: 38 80 00 00  	li 4, 0
803a54b8: 48 11 b1 a9  	bl 0x804c0660 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x11b144>
803a54bc: 80 1d 02 10  	lwz 0, 528(29)
803a54c0: 38 bf 00 48  	addi 5, 31, 72
803a54c4: 7f a3 eb 78  	mr	3, 29
803a54c8: 38 81 00 20  	addi 4, 1, 32
803a54cc: 54 00 10 3a  	slwi 0, 0, 2
803a54d0: 7d 85 00 2e  	lwzx 12, 5, 0
803a54d4: 7d 89 03 a6  	mtctr 12
803a54d8: 4e 80 04 21  	bctrl
803a54dc: 80 1d 02 0c  	lwz 0, 524(29)
803a54e0: 38 7f 00 40  	addi 3, 31, 64
803a54e4: 38 80 00 20  	li 4, 32
803a54e8: 54 00 10 3a  	slwi 0, 0, 2
803a54ec: 7c 63 00 2e  	lwzx 3, 3, 0
803a54f0: 48 11 ae 41  	bl 0x804c0330 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x11ae14>
803a54f4: e3 e1 00 a8  	<unknown>
803a54f8: cb e1 00 a0  	lfd 31, 160(1)
803a54fc: e3 c1 00 98  	<unknown>
803a5500: 39 61 00 90  	addi 11, 1, 144
803a5504: cb c1 00 90  	lfd 30, 144(1)
803a5508: 48 17 35 4d  	bl 0x80518a54 <_binary_build_original_jpa_overwrite_20260903_DrawRotDirection_bin_end+0x173538>
803a550c: 80 01 00 b4  	lwz 0, 180(1)
803a5510: 7c 08 03 a6  	mtlr 0
803a5514: 38 21 00 b0  	addi 1, 1, 176
803a5518: 4e 80 00 20  	blr
