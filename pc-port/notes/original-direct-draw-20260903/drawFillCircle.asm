
build/original-direct-draw-20260903/drawFillCircle.o:	file format elf32-powerpc

Disassembly of section .data:

804028ac <_binary_build_original_direct_draw_20260903_drawFillCircle_bin_start>:
804028ac: 94 21 ff 80  	stwu 1, -128(1)
804028b0: 7c 08 02 a6  	mflr 0
804028b4: 90 01 00 84  	stw 0, 132(1)
804028b8: db e1 00 70  	stfd 31, 112(1)
804028bc: f3 e1 00 78  	xxsel 31, 1, 0, 33
804028c0: db c1 00 60  	stfd 30, 96(1)
804028c4: f3 c1 00 68  	<unknown>
804028c8: db a1 00 50  	stfd 29, 80(1)
804028cc: f3 a1 00 58  	xscmpgtdp 29, 1, 0
804028d0: db 81 00 40  	stfd 28, 64(1)
804028d4: f3 81 00 48  	xsmaddmsp 28, 1, 0
804028d8: 39 61 00 40  	addi 11, 1, 64
804028dc: 48 11 61 25  	bl 0x80518a00 <_binary_build_original_direct_draw_20260903_drawFillCircle_bin_end+0x115ff0>
804028e0: c0 03 00 08  	lfs 0, 8(3)
804028e4: 3c e0 43 30  	lis 7, 17200
804028e8: ff c0 08 90  	fmr 30, 1
804028ec: 7c 7b 1b 78  	mr	27, 3
804028f0: 90 e1 00 18  	stw 7, 24(1)
804028f4: 38 06 00 02  	addi 0, 6, 2
804028f8: 7c 9e 23 78  	mr	30, 4
804028fc: 7c bc 2b 78  	mr	28, 5
80402900: 90 e1 00 20  	stw 7, 32(1)
80402904: 7c dd 33 78  	mr	29, 6
80402908: 54 05 04 3e  	clrlwi	5, 0, 16
8040290c: 38 60 00 a0  	li 3, 160
80402910: d0 01 00 10  	stfs 0, 16(1)
80402914: 38 80 00 00  	li 4, 0
80402918: 48 0b a2 19  	bl 0x804bcb30 <_binary_build_original_direct_draw_20260903_drawFillCircle_bin_end+0xba120>
8040291c: c0 3b 00 00  	lfs 1, 0(27)
80402920: c0 5b 00 04  	lfs 2, 4(27)
80402924: c0 7b 00 08  	lfs 3, 8(27)
80402928: 48 00 1b 6d  	bl 0x80404494 <_binary_build_original_direct_draw_20260903_drawFillCircle_bin_end+0x1a84>
8040292c: 3f e0 cc 01  	lis 31, -13311
80402930: 3c 60 80 54  	lis 3, -32684
80402934: 93 df 80 00  	stw 30, -32768(31)
80402938: 3b c0 00 00  	li 30, 0
8040293c: cb e3 f5 c0  	lfd 31, -2624(3)
80402940: c3 82 1d 10  	lfs 28, 7440(2)
80402944: c3 a2 1d 14  	lfs 29, 7444(2)
80402948: 48 00 00 88  	b 0x804029d0 <_binary_build_original_direct_draw_20260903_drawFillCircle_bin_start+0x124>
8040294c: 93 c1 00 1c  	stw 30, 28(1)
80402950: 93 a1 00 24  	stw 29, 36(1)
80402954: c8 21 00 18  	lfd 1, 24(1)
80402958: c8 01 00 20  	lfd 0, 32(1)
8040295c: ec 21 f8 28  	fsubs 1, 1, 31
80402960: ec 00 f8 28  	fsubs 0, 0, 31
80402964: ec 01 00 24  	fdivs 0, 1, 0
80402968: ec 00 07 32  	fmuls 0, 0, 28
8040296c: ec 3d 00 32  	fmuls 1, 29, 0
80402970: 4b c2 1e 09  	bl 0x80024778 <_binary_build_original_direct_draw_20260903_drawFillCircle_bin_end+0xffffffffffc21d68>
80402974: ec 5e 00 72  	fmuls 2, 30, 1
80402978: 93 c1 00 1c  	stw 30, 28(1)
8040297c: c0 1b 00 00  	lfs 0, 0(27)
80402980: 93 a1 00 24  	stw 29, 36(1)
80402984: ec 40 10 28  	fsubs 2, 0, 2
80402988: c8 21 00 18  	lfd 1, 24(1)
8040298c: c8 01 00 20  	lfd 0, 32(1)
80402990: ec 21 f8 28  	fsubs 1, 1, 31
80402994: ec 00 f8 28  	fsubs 0, 0, 31
80402998: d0 41 00 08  	stfs 2, 8(1)
8040299c: ec 01 00 24  	fdivs 0, 1, 0
804029a0: ec 00 07 32  	fmuls 0, 0, 28
804029a4: ec 3d 00 32  	fmuls 1, 29, 0
804029a8: 4b c2 1e 15  	bl 0x800247bc <_binary_build_original_direct_draw_20260903_drawFillCircle_bin_end+0xffffffffffc21dac>
804029ac: ec 5e 00 72  	fmuls 2, 30, 1
804029b0: c0 1b 00 04  	lfs 0, 4(27)
804029b4: c0 21 00 08  	lfs 1, 8(1)
804029b8: c0 61 00 10  	lfs 3, 16(1)
804029bc: ec 40 10 2a  	fadds 2, 0, 2
804029c0: d0 41 00 0c  	stfs 2, 12(1)
804029c4: 48 00 1a d1  	bl 0x80404494 <_binary_build_original_direct_draw_20260903_drawFillCircle_bin_end+0x1a84>
804029c8: 93 9f 80 00  	stw 28, -32768(31)
804029cc: 3b de 00 01  	addi 30, 30, 1
804029d0: 7c 1e e8 40  	cmplw	30, 29
804029d4: 40 81 ff 78  	bf	1, 0x8040294c <_binary_build_original_direct_draw_20260903_drawFillCircle_bin_start+0xa0>
804029d8: e3 e1 00 78  	<unknown>
804029dc: cb e1 00 70  	lfd 31, 112(1)
804029e0: e3 c1 00 68  	<unknown>
804029e4: cb c1 00 60  	lfd 30, 96(1)
804029e8: e3 a1 00 58  	<unknown>
804029ec: cb a1 00 50  	lfd 29, 80(1)
804029f0: e3 81 00 48  	<unknown>
804029f4: 39 61 00 40  	addi 11, 1, 64
804029f8: cb 81 00 40  	lfd 28, 64(1)
804029fc: 48 11 60 51  	bl 0x80518a4c <_binary_build_original_direct_draw_20260903_drawFillCircle_bin_end+0x11603c>
80402a00: 80 01 00 84  	lwz 0, 132(1)
80402a04: 7c 08 03 a6  	mtlr 0
80402a08: 38 21 00 80  	addi 1, 1, 128
80402a0c: 4e 80 00 20  	blr
