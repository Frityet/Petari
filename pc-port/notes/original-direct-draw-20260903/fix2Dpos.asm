
build/original-direct-draw-20260903/fix2Dpos.o:	file format elf32-powerpc

Disassembly of section .data:

804043f8 <_binary_build_original_direct_draw_20260903_fix2Dpos_bin_start>:
804043f8: 94 21 ff c0  	stwu 1, -64(1)
804043fc: 7c 08 02 a6  	mflr 0
80404400: 90 01 00 44  	stw 0, 68(1)
80404404: db e1 00 30  	stfd 31, 48(1)
80404408: f3 e1 00 38  	xxsel 31, 1, 0, 32
8040440c: 39 61 00 30  	addi 11, 1, 48
80404410: 48 11 45 f9  	bl 0x80518a08 <_binary_build_original_direct_draw_20260903_fix2Dpos_bin_end+0x114574>
80404414: 7c 7d 1b 78  	mr	29, 3
80404418: 4b ff b1 8d  	bl 0x803ff5a4 <_binary_build_original_direct_draw_20260903_fix2Dpos_bin_end+0xffffffffffffb110>
8040441c: 2c 03 00 00  	cmpwi	3, 0
80404420: 41 82 00 54  	bt	2, 0x80404474 <_binary_build_original_direct_draw_20260903_fix2Dpos_bin_start+0x7c>
80404424: 4b eb 13 01  	bl 0x802b5724 <_binary_build_original_direct_draw_20260903_fix2Dpos_bin_end+0xffffffffffeb1290>
80404428: 6c 60 80 00  	xoris 0, 3, 32768
8040442c: 3f e0 43 30  	lis 31, 17200
80404430: 90 01 00 0c  	stw 0, 12(1)
80404434: 3f c0 80 54  	lis 30, -32684
80404438: c8 3e f5 b8  	lfd 1, -2632(30)
8040443c: 93 e1 00 08  	stw 31, 8(1)
80404440: c8 01 00 08  	lfd 0, 8(1)
80404444: ef e0 08 28  	fsubs 31, 0, 1
80404448: 4b ff 3d 99  	bl 0x803f81e0 <_binary_build_original_direct_draw_20260903_fix2Dpos_bin_end+0xffffffffffff3d4c>
8040444c: 6c 60 80 00  	xoris 0, 3, 32768
80404450: 93 e1 00 10  	stw 31, 16(1)
80404454: c8 5e f5 b8  	lfd 2, -2632(30)
80404458: 90 01 00 14  	stw 0, 20(1)
8040445c: c0 1d 00 00  	lfs 0, 0(29)
80404460: c8 21 00 10  	lfd 1, 16(1)
80404464: ec 21 10 28  	fsubs 1, 1, 2
80404468: ec 21 f8 24  	fdivs 1, 1, 31
8040446c: ec 00 00 72  	fmuls 0, 0, 1
80404470: d0 1d 00 00  	stfs 0, 0(29)
80404474: e3 e1 00 38  	<unknown>
80404478: 39 61 00 30  	addi 11, 1, 48
8040447c: cb e1 00 30  	lfd 31, 48(1)
80404480: 48 11 45 d5  	bl 0x80518a54 <_binary_build_original_direct_draw_20260903_fix2Dpos_bin_end+0x1145c0>
80404484: 80 01 00 44  	lwz 0, 68(1)
80404488: 7c 08 03 a6  	mtlr 0
8040448c: 38 21 00 40  	addi 1, 1, 64
80404490: 4e 80 00 20  	blr
