
build/original-jpa-draw-20260903/DrawLine.o:	file format elf32-powerpc

Disassembly of section .data:

803a5690 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_start>:
803a5690: 94 21 ff c0  	stwu 1, -64(1)
803a5694: 7c 08 02 a6  	mflr 0
803a5698: 90 01 00 44  	stw 0, 68(1)
803a569c: db e1 00 30  	stfd 31, 48(1)
803a56a0: f3 e1 00 38  	xxsel 31, 1, 0, 32
803a56a4: 93 e1 00 2c  	stw 31, 44(1)
803a56a8: 7c 9f 23 78  	mr	31, 4
803a56ac: 93 c1 00 28  	stw 30, 40(1)
803a56b0: 7c 7e 1b 78  	mr	30, 3
803a56b4: 80 04 00 7c  	lwz 0, 124(4)
803a56b8: 54 00 07 39  	rlwinm. 0, 0, 0, 28, 28
803a56bc: 40 82 01 00  	bf	2, 0x803a57bc <_binary_build_original_jpa_draw_20260903_DrawLine_bin_start+0x12c>
803a56c0: 38 61 00 14  	addi 3, 1, 20
803a56c4: 4b c7 38 2d  	bl 0x80018ef0 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0xffffffffffc73714>
803a56c8: 38 61 00 08  	addi 3, 1, 8
803a56cc: 38 9f 00 24  	addi 4, 31, 36
803a56d0: 4b c7 7b f9  	bl 0x8001d2c8 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0xffffffffffc77aec>
803a56d4: c0 22 18 48  	lfs 1, 6216(2)
803a56d8: 38 61 00 08  	addi 3, 1, 8
803a56dc: 48 04 19 0d  	bl 0x803e6fe8 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0x4180c>
803a56e0: 2c 03 00 00  	cmpwi	3, 0
803a56e4: 40 82 00 d8  	bf	2, 0x803a57bc <_binary_build_original_jpa_draw_20260903_DrawLine_bin_start+0x12c>
803a56e8: c0 42 18 4c  	lfs 2, 6220(2)
803a56ec: 38 61 00 08  	addi 3, 1, 8
803a56f0: c0 3f 00 64  	lfs 1, 100(31)
803a56f4: c0 1e 01 48  	lfs 0, 328(30)
803a56f8: ec 22 00 72  	fmuls 1, 2, 1
803a56fc: ef e0 00 72  	fmuls 31, 0, 1
803a5700: 4b cd 20 b1  	bl 0x800777b0 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0xffffffffffcd1fd4>
803a5704: c0 02 18 30  	lfs 0, 6192(2)
803a5708: fc 01 00 40  	fcmpo 0, 1, 0
803a570c: 4c 40 13 82  	cror 2, 0, 2
803a5710: 40 82 00 08  	bf	2, 0x803a5718 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_start+0x88>
803a5714: 48 00 00 14  	b 0x803a5728 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_start+0x98>
803a5718: 4b c7 b1 7d  	bl 0x80020894 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0xffffffffffc7b0b8>
803a571c: ec 21 07 f2  	fmuls 1, 1, 31
803a5720: 38 61 00 08  	addi 3, 1, 8
803a5724: 4b c7 a9 ad  	bl 0x800200d0 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0xffffffffffc7a8f4>
803a5728: 38 81 00 08  	addi 4, 1, 8
803a572c: 38 61 00 14  	addi 3, 1, 20
803a5730: 7c 85 23 78  	mr	5, 4
803a5734: 4b c7 88 a5  	bl 0x8001dfd8 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0xffffffffffc787fc>
803a5738: 38 60 00 09  	li 3, 9
803a573c: 38 80 00 01  	li 4, 1
803a5740: 48 11 5c e1  	bl 0x804bb420 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0x115c44>
803a5744: 38 60 00 0d  	li 3, 13
803a5748: 38 80 00 01  	li 4, 1
803a574c: 48 11 5c d5  	bl 0x804bb420 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0x115c44>
803a5750: 38 60 00 a8  	li 3, 168
803a5754: 38 80 00 01  	li 4, 1
803a5758: 38 a0 00 02  	li 5, 2
803a575c: 48 11 73 d5  	bl 0x804bcb30 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0x117354>
803a5760: c0 21 00 14  	lfs 1, 20(1)
803a5764: c0 41 00 18  	lfs 2, 24(1)
803a5768: c0 61 00 1c  	lfs 3, 28(1)
803a576c: 48 00 15 85  	bl 0x803a6cf0 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0x1514>
803a5770: c0 02 18 2c  	lfs 0, 6188(2)
803a5774: 3f e0 cc 01  	lis 31, -13311
803a5778: d0 1f 80 00  	stfs 0, -32768(31)
803a577c: c0 02 18 2c  	lfs 0, 6188(2)
803a5780: d0 1f 80 00  	stfs 0, -32768(31)
803a5784: c0 21 00 08  	lfs 1, 8(1)
803a5788: c0 41 00 0c  	lfs 2, 12(1)
803a578c: c0 61 00 10  	lfs 3, 16(1)
803a5790: 48 00 15 61  	bl 0x803a6cf0 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0x1514>
803a5794: c0 02 18 2c  	lfs 0, 6188(2)
803a5798: 38 60 00 09  	li 3, 9
803a579c: 38 80 00 02  	li 4, 2
803a57a0: d0 1f 80 00  	stfs 0, -32768(31)
803a57a4: c0 02 18 28  	lfs 0, 6184(2)
803a57a8: d0 1f 80 00  	stfs 0, -32768(31)
803a57ac: 48 11 5c 75  	bl 0x804bb420 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0x115c44>
803a57b0: 38 60 00 0d  	li 3, 13
803a57b4: 38 80 00 02  	li 4, 2
803a57b8: 48 11 5c 69  	bl 0x804bb420 <_binary_build_original_jpa_draw_20260903_DrawLine_bin_end+0x115c44>
803a57bc: e3 e1 00 38  	<unknown>
803a57c0: 80 01 00 44  	lwz 0, 68(1)
803a57c4: cb e1 00 30  	lfd 31, 48(1)
803a57c8: 83 e1 00 2c  	lwz 31, 44(1)
803a57cc: 83 c1 00 28  	lwz 30, 40(1)
803a57d0: 7c 08 03 a6  	mtlr 0
803a57d4: 38 21 00 40  	addi 1, 1, 64
803a57d8: 4e 80 00 20  	blr
