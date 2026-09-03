
build/original-jpa-resource-loader-20260903/makeColorTable.o:	file format elf32-powerpc

Disassembly of section .data:

804472b8 <_binary_build_original_jpa_resource_loader_20260903_makeColorTable_bin_start>:
804472b8: 94 21 ff 70  	stwu 1, -144(1)
804472bc: 7c 08 02 a6  	mflr 0
804472c0: 90 01 00 94  	stw 0, 148(1)
804472c4: db e1 00 80  	stfd 31, 128(1)
804472c8: f3 e1 00 88  	xsmsubasp 31, 1, 0
804472cc: db c1 00 70  	stfd 30, 112(1)
804472d0: f3 c1 00 78  	xxsel 30, 1, 0, 33
804472d4: db a1 00 60  	stfd 29, 96(1)
804472d8: f3 a1 00 68  	<unknown>
804472dc: 39 61 00 60  	addi 11, 1, 96
804472e0: 48 0d 17 15  	bl 0x805189f4 <_binary_build_original_jpa_resource_loader_20260903_makeColorTable_bin_end+0xd14bc>
804472e4: 3c 00 43 30  	lis 0, 17200
804472e8: 7c 9a 23 78  	mr	26, 4
804472ec: 90 01 00 08  	stw 0, 8(1)
804472f0: 3b c6 00 01  	addi 30, 6, 1
804472f4: 7c 79 1b 78  	mr	25, 3
804472f8: 7c bb 2b 78  	mr	27, 5
804472fc: 90 01 00 10  	stw 0, 16(1)
80447300: 7c e5 3b 78  	mr	5, 7
80447304: 57 c3 10 3a  	slwi 3, 30, 2
80447308: 38 80 00 04  	li 4, 4
8044730c: 4b fc 3c b1  	bl 0x8040afbc <_binary_build_original_jpa_resource_loader_20260903_makeColorTable_bin_end+0xfffffffffffc3a84>
80447310: 88 da 00 02  	lbz 6, 2(26)
80447314: 7c 7f 1b 78  	mr	31, 3
80447318: 88 1a 00 03  	lbz 0, 3(26)
8044731c: 3c a0 80 56  	lis 5, -32682
80447320: 90 c1 00 0c  	stw 6, 12(1)
80447324: 3c 80 80 56  	lis 4, -32682
80447328: c0 c2 1f c4  	lfs 6, 8132(2)
8044732c: 3b a0 00 00  	li 29, 0
80447330: 90 01 00 14  	stw 0, 20(1)
80447334: 3b 00 00 00  	li 24, 0
80447338: c8 01 00 08  	lfd 0, 8(1)
8044733c: fc 80 30 90  	fmr 4, 6
80447340: 88 7a 00 04  	lbz 3, 4(26)
80447344: fc a0 30 90  	fmr 5, 6
80447348: cb e5 c3 c0  	lfd 31, -15424(5)
8044734c: fc 60 30 90  	fmr 3, 6
80447350: c8 21 00 10  	lfd 1, 16(1)
80447354: 88 1a 00 05  	lbz 0, 5(26)
80447358: ec 00 f8 28  	fsubs 0, 0, 31
8044735c: 90 61 00 0c  	stw 3, 12(1)
80447360: ec e1 f8 28  	fsubs 7, 1, 31
80447364: cb a4 c3 b8  	lfd 29, -15432(4)
80447368: 3b 80 00 00  	li 28, 0
8044736c: 90 01 00 14  	stw 0, 20(1)
80447370: c8 41 00 08  	lfd 2, 8(1)
80447374: c8 21 00 10  	lfd 1, 16(1)
80447378: ed 02 f8 28  	fsubs 8, 2, 31
8044737c: c3 c2 1f c0  	lfs 30, 8128(2)
80447380: ed 21 f8 28  	fsubs 9, 1, 31
80447384: 48 00 01 74  	b 0x804474f8 <_binary_build_original_jpa_resource_loader_20260903_makeColorTable_bin_start+0x240>
80447388: 7c 1a c2 ae  	lhax 0, 26, 24
8044738c: 7f 83 07 34  	extsh 3, 28
80447390: 7c 03 00 00  	cmpw	3, 0
80447394: 40 82 01 08  	bf	2, 0x8044749c <_binary_build_original_jpa_resource_loader_20260903_makeColorTable_bin_start+0x1e4>
80447398: 54 60 10 3a  	slwi 0, 3, 2
8044739c: 7c 9a c2 14  	add 4, 26, 24
804473a0: 7c 7f 02 14  	add 3, 31, 0
804473a4: 38 84 00 02  	addi 4, 4, 2
804473a8: 4b c8 1b cd  	bl 0x800c8f74 <_binary_build_original_jpa_resource_loader_20260903_makeColorTable_bin_end+0xffffffffffc81a3c>
804473ac: 7c 9a c2 14  	add 4, 26, 24
804473b0: 3b bd 00 01  	addi 29, 29, 1
804473b4: 88 64 00 02  	lbz 3, 2(4)
804473b8: 7c 1d d8 00  	cmpw	29, 27
804473bc: 88 04 00 03  	lbz 0, 3(4)
804473c0: 3b 18 00 06  	addi 24, 24, 6
804473c4: 90 61 00 0c  	stw 3, 12(1)
804473c8: 88 64 00 04  	lbz 3, 4(4)
804473cc: 90 01 00 14  	stw 0, 20(1)
804473d0: c8 01 00 08  	lfd 0, 8(1)
804473d4: c8 21 00 10  	lfd 1, 16(1)
804473d8: 88 04 00 05  	lbz 0, 5(4)
804473dc: ec 00 f8 28  	fsubs 0, 0, 31
804473e0: 90 61 00 0c  	stw 3, 12(1)
804473e4: ec e1 f8 28  	fsubs 7, 1, 31
804473e8: 90 01 00 14  	stw 0, 20(1)
804473ec: c8 41 00 08  	lfd 2, 8(1)
804473f0: c8 21 00 10  	lfd 1, 16(1)
804473f4: ed 02 f8 28  	fsubs 8, 2, 31
804473f8: ed 21 f8 28  	fsubs 9, 1, 31
804473fc: 40 80 00 8c  	bf	0, 0x80447488 <_binary_build_original_jpa_resource_loader_20260903_makeColorTable_bin_start+0x1d0>
80447400: 7c 9a c2 14  	add 4, 26, 24
80447404: 88 04 00 02  	lbz 0, 2(4)
80447408: 88 64 00 03  	lbz 3, 3(4)
8044740c: 90 01 00 0c  	stw 0, 12(1)
80447410: 88 04 00 04  	lbz 0, 4(4)
80447414: c8 21 00 08  	lfd 1, 8(1)
80447418: 90 61 00 14  	stw 3, 20(1)
8044741c: a8 64 ff fa  	lha 3, -6(4)
80447420: ec 21 f8 28  	fsubs 1, 1, 31
80447424: 90 01 00 0c  	stw 0, 12(1)
80447428: 7c 1a c2 ae  	lhax 0, 26, 24
8044742c: c8 41 00 08  	lfd 2, 8(1)
80447430: ec 61 00 28  	fsubs 3, 1, 0
80447434: 7c 03 00 50  	sub	0, 0, 3
80447438: c8 21 00 10  	lfd 1, 16(1)
8044743c: 6c 00 80 00  	xoris 0, 0, 32768
80447440: 88 64 00 05  	lbz 3, 5(4)
80447444: 90 01 00 0c  	stw 0, 12(1)
80447448: ec c1 f8 28  	fsubs 6, 1, 31
8044744c: c8 21 00 08  	lfd 1, 8(1)
80447450: ec 42 f8 28  	fsubs 2, 2, 31
80447454: 90 61 00 14  	stw 3, 20(1)
80447458: ec 81 e8 28  	fsubs 4, 1, 29
8044745c: ec 26 38 28  	fsubs 1, 6, 7
80447460: c8 a1 00 10  	lfd 5, 16(1)
80447464: ec 42 40 28  	fsubs 2, 2, 8
80447468: ec de 20 24  	fdivs 6, 30, 4
8044746c: ec 85 f8 28  	fsubs 4, 5, 31
80447470: ec a6 00 72  	fmuls 5, 6, 1
80447474: ec 66 00 f2  	fmuls 3, 6, 3
80447478: ec 24 48 28  	fsubs 1, 4, 9
8044747c: ec 86 00 b2  	fmuls 4, 6, 2
80447480: ec c6 00 72  	fmuls 6, 6, 1
80447484: 48 00 00 70  	b 0x804474f4 <_binary_build_original_jpa_resource_loader_20260903_makeColorTable_bin_start+0x23c>
80447488: c0 c2 1f c4  	lfs 6, 8132(2)
8044748c: fc 80 30 90  	fmr 4, 6
80447490: fc a0 30 90  	fmr 5, 6
80447494: fc 60 30 90  	fmr 3, 6
80447498: 48 00 00 5c  	b 0x804474f4 <_binary_build_original_jpa_resource_loader_20260903_makeColorTable_bin_start+0x23c>
8044749c: ec 00 18 2a  	fadds 0, 0, 3
804474a0: 54 60 10 3a  	slwi 0, 3, 2
804474a4: ec e7 28 2a  	fadds 7, 7, 5
804474a8: 7c 9f 02 14  	add 4, 31, 0
804474ac: ed 08 20 2a  	fadds 8, 8, 4
804474b0: fc 40 00 1e  	fctiwz 2, 0
804474b4: fc 20 38 1e  	fctiwz 1, 7
804474b8: ed 29 30 2a  	fadds 9, 9, 6
804474bc: d8 41 00 18  	stfd 2, 24(1)
804474c0: fc 40 40 1e  	fctiwz 2, 8
804474c4: d8 21 00 20  	stfd 1, 32(1)
804474c8: fc 20 48 1e  	fctiwz 1, 9
804474cc: 80 01 00 1c  	lwz 0, 28(1)
804474d0: d8 41 00 28  	stfd 2, 40(1)
804474d4: 80 61 00 24  	lwz 3, 36(1)
804474d8: 98 04 00 00  	stb 0, 0(4)
804474dc: 80 01 00 2c  	lwz 0, 44(1)
804474e0: 98 64 00 01  	stb 3, 1(4)
804474e4: d8 21 00 30  	stfd 1, 48(1)
804474e8: 98 04 00 02  	stb 0, 2(4)
804474ec: 80 01 00 34  	lwz 0, 52(1)
804474f0: 98 04 00 03  	stb 0, 3(4)
804474f4: 3b 9c 00 01  	addi 28, 28, 1
804474f8: 7f 80 07 34  	extsh 0, 28
804474fc: 7c 00 f0 00  	cmpw	0, 30
80447500: 41 80 fe 88  	bt	0, 0x80447388 <_binary_build_original_jpa_resource_loader_20260903_makeColorTable_bin_start+0xd0>
80447504: 93 f9 00 00  	stw 31, 0(25)
80447508: e3 e1 00 88  	<unknown>
8044750c: cb e1 00 80  	lfd 31, 128(1)
80447510: e3 c1 00 78  	<unknown>
80447514: cb c1 00 70  	lfd 30, 112(1)
80447518: e3 a1 00 68  	<unknown>
8044751c: cb a1 00 60  	lfd 29, 96(1)
80447520: 39 61 00 60  	addi 11, 1, 96
80447524: 48 0d 15 1d  	bl 0x80518a40 <_binary_build_original_jpa_resource_loader_20260903_makeColorTable_bin_end+0xd1508>
80447528: 80 01 00 94  	lwz 0, 148(1)
8044752c: 7c 08 03 a6  	mtlr 0
80447530: 38 21 00 90  	addi 1, 1, 144
80447534: 4e 80 00 20  	blr
