
build/original-jpa-arithmetic-20260903/VolumeSphere.o:	file format elf32-powerpc

Disassembly of section .data:

8044844c <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_start>:
8044844c: 94 21 ff c0  	stwu 1, -64(1)
80448450: 7c 08 02 a6  	mflr 0
80448454: 90 01 00 44  	stw 0, 68(1)
80448458: db e1 00 30  	stfd 31, 48(1)
8044845c: f3 e1 00 38  	xxsel 31, 1, 0, 32
80448460: 39 61 00 30  	addi 11, 1, 48
80448464: 48 0d 05 a5  	bl 0x80518a08 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_end+0xd038c>
80448468: 7c 7f 1b 78  	mr	31, 3
8044846c: 80 63 00 00  	lwz 3, 0(3)
80448470: 38 80 00 02  	li 4, 2
80448474: 48 00 07 51  	bl 0x80448bc4 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_end+0x548>
80448478: 2c 03 00 00  	cmpwi	3, 0
8044847c: 41 82 00 d8  	bt	2, 0x80448554 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_start+0x108>
80448480: 80 bf 01 ec  	lwz 5, 492(31)
80448484: 3c 60 43 30  	lis 3, 17200
80448488: 80 df 01 f0  	lwz 6, 496(31)
8044848c: 3c 80 80 56  	lis 4, -32682
80448490: 38 05 00 01  	addi 0, 5, 1
80448494: 54 a7 80 1e  	slwi 7, 5, 16
80448498: 38 a6 ff ff  	addi 5, 6, -1
8044849c: 81 3f 01 f4  	lwz 9, 500(31)
804484a0: 7c a7 2b d6  	divw 5, 7, 5
804484a4: c8 44 c3 d8  	lfd 2, -15400(4)
804484a8: 80 ff 01 f8  	lwz 7, 504(31)
804484ac: 55 28 78 20  	slwi 8, 9, 15
804484b0: 90 61 00 08  	stw 3, 8(1)
804484b4: 7c 00 30 00  	cmpw	0, 6
804484b8: 38 67 ff ff  	addi 3, 7, -1
804484bc: 54 a4 04 3e  	clrlwi	4, 5, 16
804484c0: 7c 68 1b d6  	divw 3, 8, 3
804484c4: 90 81 00 0c  	stw 4, 12(1)
804484c8: c0 3f 00 3c  	lfs 1, 60(31)
804484cc: c8 01 00 08  	lfd 0, 8(1)
804484d0: 90 1f 01 ec  	stw 0, 492(31)
804484d4: ec 40 10 28  	fsubs 2, 0, 2
804484d8: 38 63 40 00  	addi 3, 3, 16384
804484dc: ec 22 00 72  	fmuls 1, 2, 1
804484e0: c0 02 1f fc  	lfs 0, 8188(2)
804484e4: 54 63 04 3e  	clrlwi	3, 3, 16
804484e8: 7c 7e 07 34  	extsh 30, 3
804484ec: ec 00 08 2a  	fadds 0, 0, 1
804484f0: fc 00 00 1e  	fctiwz 0, 0
804484f4: d8 01 00 10  	stfd 0, 16(1)
804484f8: 83 a1 00 14  	lwz 29, 20(1)
804484fc: 40 82 00 a8  	bf	2, 0x804485a4 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_start+0x158>
80448500: 38 69 00 01  	addi 3, 9, 1
80448504: 38 80 00 00  	li 4, 0
80448508: 54 60 08 3c  	slwi 0, 3, 1
8044850c: 90 9f 01 ec  	stw 4, 492(31)
80448510: 7c 00 38 00  	cmpw	0, 7
80448514: 90 7f 01 f4  	stw 3, 500(31)
80448518: 40 80 00 20  	bf	0, 0x80448538 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_start+0xec>
8044851c: 2c 06 00 01  	cmpwi	6, 1
80448520: 41 82 00 0c  	bt	2, 0x8044852c <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_start+0xe0>
80448524: 38 06 00 04  	addi 0, 6, 4
80448528: 48 00 00 08  	b 0x80448530 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_start+0xe4>
8044852c: 38 06 00 03  	addi 0, 6, 3
80448530: 90 1f 01 f0  	stw 0, 496(31)
80448534: 48 00 00 70  	b 0x804485a4 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_start+0x158>
80448538: 2c 06 00 04  	cmpwi	6, 4
8044853c: 41 82 00 0c  	bt	2, 0x80448548 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_start+0xfc>
80448540: 38 06 ff fc  	addi 0, 6, -4
80448544: 48 00 00 08  	b 0x8044854c <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_start+0x100>
80448548: 38 00 00 01  	li 0, 1
8044854c: 90 1f 01 f0  	stw 0, 496(31)
80448550: 48 00 00 54  	b 0x804485a4 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_start+0x158>
80448554: 80 7f 00 00  	lwz 3, 0(31)
80448558: 48 00 06 85  	bl 0x80448bdc <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_end+0x560>
8044855c: 7c 60 07 34  	extsh 0, 3
80448560: 80 7f 00 00  	lwz 3, 0(31)
80448564: 7c 1e 0e 70  	srawi 30, 0, 1
80448568: 48 00 06 75  	bl 0x80448bdc <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_end+0x560>
8044856c: 7c 63 07 34  	extsh 3, 3
80448570: 3c 00 43 30  	lis 0, 17200
80448574: 6c 63 80 00  	xoris 3, 3, 32768
80448578: 3c 80 80 56  	lis 4, -32682
8044857c: 90 61 00 14  	stw 3, 20(1)
80448580: c8 44 c3 d0  	lfd 2, -15408(4)
80448584: 90 01 00 10  	stw 0, 16(1)
80448588: c0 1f 00 3c  	lfs 0, 60(31)
8044858c: c8 21 00 10  	lfd 1, 16(1)
80448590: ec 21 10 28  	fsubs 1, 1, 2
80448594: ec 00 00 72  	fmuls 0, 0, 1
80448598: fc 00 00 1e  	fctiwz 0, 0
8044859c: d8 01 00 08  	stfd 0, 8(1)
804485a0: 83 a1 00 0c  	lwz 29, 12(1)
804485a4: 80 7f 00 00  	lwz 3, 0(31)
804485a8: 38 63 00 c4  	addi 3, 3, 196
804485ac: 48 00 05 d9  	bl 0x80448b84 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_end+0x508>
804485b0: ff e0 08 90  	fmr 31, 1
804485b4: 80 7f 00 00  	lwz 3, 0(31)
804485b8: 38 80 00 01  	li 4, 1
804485bc: 48 00 06 09  	bl 0x80448bc4 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_end+0x548>
804485c0: 2c 03 00 00  	cmpwi	3, 0
804485c4: 41 82 00 14  	bt	2, 0x804485d8 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_start+0x18c>
804485c8: ec 3f 07 f2  	fmuls 1, 31, 31
804485cc: c0 02 1f f8  	lfs 0, 8184(2)
804485d0: ec 3f 00 72  	fmuls 1, 31, 1
804485d4: ef e0 08 28  	fsubs 31, 0, 1
804485d8: c0 02 1f f8  	lfs 0, 8184(2)
804485dc: 3c a0 80 61  	lis 5, -32671
804485e0: c0 5f 00 38  	lfs 2, 56(31)
804485e4: 57 c6 0b f8  	rlwinm 6, 30, 1, 15, 28
804485e8: 38 a5 fc 80  	addi 5, 5, -896
804485ec: 57 a0 0b f8  	rlwinm 0, 29, 1, 15, 28
804485f0: ec 20 10 28  	fsubs 1, 0, 2
804485f4: 7c 65 32 14  	add 3, 5, 6
804485f8: c0 63 00 04  	lfs 3, 4(3)
804485fc: 7c 85 02 14  	add 4, 5, 0
80448600: c0 1f 00 34  	lfs 0, 52(31)
80448604: 38 7f 00 10  	addi 3, 31, 16
80448608: ec 3f 00 72  	fmuls 1, 31, 1
8044860c: c0 84 00 04  	lfs 4, 4(4)
80448610: 7c a5 34 2e  	lfsx 5, 5, 6
80448614: 7c c5 04 2e  	lfsx 6, 5, 0
80448618: ec 22 08 2a  	fadds 1, 2, 1
8044861c: ef e0 00 72  	fmuls 31, 0, 1
80448620: ec 7f 00 f2  	fmuls 3, 31, 3
80448624: fc 00 f8 50  	fneg 0, 31
80448628: ec 23 01 b2  	fmuls 1, 3, 6
8044862c: ec 40 01 72  	fmuls 2, 0, 5
80448630: ec 63 01 32  	fmuls 3, 3, 4
80448634: 4b bc ec b1  	bl 0x800172e4 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_end+0xffffffffffbcec68>
80448638: 38 7f 00 1c  	addi 3, 31, 28
8044863c: 38 9f 00 10  	addi 4, 31, 16
80448640: 38 bf 01 14  	addi 5, 31, 276
80448644: 4b c9 ec e5  	bl 0x800e7328 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_end+0xffffffffffc9ecac>
80448648: c0 3f 00 10  	lfs 1, 16(31)
8044864c: 38 7f 00 28  	addi 3, 31, 40
80448650: c0 42 1f f0  	lfs 2, 8176(2)
80448654: c0 7f 00 18  	lfs 3, 24(31)
80448658: 4b bc ec 8d  	bl 0x800172e4 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_end+0xffffffffffbcec68>
8044865c: e3 e1 00 38  	<unknown>
80448660: 39 61 00 30  	addi 11, 1, 48
80448664: cb e1 00 30  	lfd 31, 48(1)
80448668: 48 0d 03 ed  	bl 0x80518a54 <_binary_build_original_jpa_arithmetic_20260903_VolumeSphere_bin_end+0xd03d8>
8044866c: 80 01 00 44  	lwz 0, 68(1)
80448670: 7c 08 03 a6  	mtlr 0
80448674: 38 21 00 40  	addi 1, 1, 64
80448678: 4e 80 00 20  	blr
