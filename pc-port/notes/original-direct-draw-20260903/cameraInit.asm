
build/original-direct-draw-20260903/cameraInit.o:	file format elf32-powerpc

Disassembly of section .data:

80403b44 <_binary_build_original_direct_draw_20260903_cameraInit_bin_start>:
80403b44: 4b fc 4e 60  	b 0x803c89a4 <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xfffffffffffc4c7c>
80403b48: 94 21 ff 70  	stwu 1, -144(1)
80403b4c: 7c 08 02 a6  	mflr 0
80403b50: 90 01 00 94  	stw 0, 148(1)
80403b54: db e1 00 80  	stfd 31, 128(1)
80403b58: f3 e1 00 88  	xsmsubasp 31, 1, 0
80403b5c: db c1 00 70  	stfd 30, 112(1)
80403b60: f3 c1 00 78  	xxsel 30, 1, 0, 33
80403b64: db a1 00 60  	stfd 29, 96(1)
80403b68: f3 a1 00 68  	<unknown>
80403b6c: 3c 60 43 30  	lis 3, 17200
80403b70: 93 e1 00 5c  	stw 31, 92(1)
80403b74: 93 c1 00 58  	stw 30, 88(1)
80403b78: 3f c0 80 61  	lis 30, -32671
80403b7c: 3b de c5 a0  	addi 30, 30, -14944
80403b80: 88 0d da 28  	lbz 0, -9688(13)
80403b84: 90 61 00 48  	stw 3, 72(1)
80403b88: 7c 00 07 75  	extsb. 0, 0
80403b8c: 90 61 00 50  	stw 3, 80(1)
80403b90: 40 82 00 60  	bf	2, 0x80403bf0 <_binary_build_original_direct_draw_20260903_cameraInit_bin_start+0xac>
80403b94: 4b c9 3e 49  	bl 0x800979dc <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xffffffffffc93cb4>
80403b98: 6c 60 80 00  	xoris 0, 3, 32768
80403b9c: 3f e0 80 54  	lis 31, -32684
80403ba0: 90 01 00 4c  	stw 0, 76(1)
80403ba4: c8 5f f5 b8  	lfd 2, -2632(31)
80403ba8: c8 21 00 48  	lfd 1, 72(1)
80403bac: c0 02 1d 08  	lfs 0, 7432(2)
80403bb0: ec 21 10 28  	fsubs 1, 1, 2
80403bb4: ef e1 00 32  	fmuls 31, 1, 0
80403bb8: 4b ff 46 29  	bl 0x803f81e0 <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xffffffffffff44b8>
80403bbc: 6c 60 80 00  	xoris 0, 3, 32768
80403bc0: c8 9f f5 b8  	lfd 4, -2632(31)
80403bc4: 90 01 00 54  	stw 0, 84(1)
80403bc8: fc 40 f8 90  	fmr 2, 31
80403bcc: c0 02 1d 08  	lfs 0, 7432(2)
80403bd0: 38 7e 00 30  	addi 3, 30, 48
80403bd4: c8 21 00 50  	lfd 1, 80(1)
80403bd8: c0 62 1d 20  	lfs 3, 7456(2)
80403bdc: ec 21 20 28  	fsubs 1, 1, 4
80403be0: ec 21 00 32  	fmuls 1, 1, 0
80403be4: 4b c1 53 45  	bl 0x80018f28 <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xffffffffffc15200>
80403be8: 38 00 00 01  	li 0, 1
80403bec: 98 0d da 28  	stb 0, -9688(13)
80403bf0: 88 0d da 29  	lbz 0, -9687(13)
80403bf4: 7c 00 07 75  	extsb. 0, 0
80403bf8: 40 82 00 60  	bf	2, 0x80403c58 <_binary_build_original_direct_draw_20260903_cameraInit_bin_start+0x114>
80403bfc: 4b c9 3d e1  	bl 0x800979dc <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xffffffffffc93cb4>
80403c00: 6c 60 80 00  	xoris 0, 3, 32768
80403c04: 3f e0 80 54  	lis 31, -32684
80403c08: 90 01 00 4c  	stw 0, 76(1)
80403c0c: c8 5f f5 b8  	lfd 2, -2632(31)
80403c10: c8 21 00 48  	lfd 1, 72(1)
80403c14: c0 02 1d 08  	lfs 0, 7432(2)
80403c18: ec 21 10 28  	fsubs 1, 1, 2
80403c1c: ef e1 00 32  	fmuls 31, 1, 0
80403c20: 4b ff 45 c1  	bl 0x803f81e0 <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xffffffffffff44b8>
80403c24: 6c 60 80 00  	xoris 0, 3, 32768
80403c28: c8 9f f5 b8  	lfd 4, -2632(31)
80403c2c: 90 01 00 54  	stw 0, 84(1)
80403c30: fc 40 f8 90  	fmr 2, 31
80403c34: c0 02 1d 08  	lfs 0, 7432(2)
80403c38: 38 7e 00 3c  	addi 3, 30, 60
80403c3c: c8 21 00 50  	lfd 1, 80(1)
80403c40: c0 62 1d 04  	lfs 3, 7428(2)
80403c44: ec 21 20 28  	fsubs 1, 1, 4
80403c48: ec 21 00 32  	fmuls 1, 1, 0
80403c4c: 4b c1 52 dd  	bl 0x80018f28 <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xffffffffffc15200>
80403c50: 38 00 00 01  	li 0, 1
80403c54: 98 0d da 29  	stb 0, -9687(13)
80403c58: 88 0d da 2a  	lbz 0, -9686(13)
80403c5c: 7c 00 07 75  	extsb. 0, 0
80403c60: 40 82 00 20  	bf	2, 0x80403c80 <_binary_build_original_direct_draw_20260903_cameraInit_bin_start+0x13c>
80403c64: 38 7e 00 48  	addi 3, 30, 72
80403c68: 38 80 00 00  	li 4, 0
80403c6c: 38 a0 ff f6  	li 5, -10
80403c70: 38 c0 00 00  	li 6, 0
80403c74: 4b c3 38 d9  	bl 0x8003754c <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xffffffffffc33824>
80403c78: 38 00 00 01  	li 0, 1
80403c7c: 98 0d da 2a  	stb 0, -9686(13)
80403c80: c3 c2 1d 04  	lfs 30, 7428(2)
80403c84: c3 a2 1d 00  	lfs 29, 7424(2)
80403c88: 4b ff 45 59  	bl 0x803f81e0 <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xffffffffffff44b8>
80403c8c: 7c 60 0e 70  	srawi 0, 3, 1
80403c90: 3f e0 80 54  	lis 31, -32684
80403c94: 7c 00 01 94  	addze 0, 0
80403c98: c8 3f f5 b8  	lfd 1, -2632(31)
80403c9c: 6c 00 80 00  	xoris 0, 0, 32768
80403ca0: 90 01 00 4c  	stw 0, 76(1)
80403ca4: c8 01 00 48  	lfd 0, 72(1)
80403ca8: ef e0 08 28  	fsubs 31, 0, 1
80403cac: 4b c9 3d 31  	bl 0x800979dc <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xffffffffffc93cb4>
80403cb0: 7c 60 0e 70  	srawi 0, 3, 1
80403cb4: c8 3f f5 b8  	lfd 1, -2632(31)
80403cb8: 7c 00 01 94  	addze 0, 0
80403cbc: fc 80 f8 90  	fmr 4, 31
80403cc0: 6c 00 80 00  	xoris 0, 0, 32768
80403cc4: fc a0 f0 90  	fmr 5, 30
80403cc8: 90 01 00 54  	stw 0, 84(1)
80403ccc: fc 60 f8 50  	fneg 3, 31
80403cd0: fc c0 e8 50  	fneg 6, 29
80403cd4: c8 01 00 50  	lfd 0, 80(1)
80403cd8: 38 61 00 08  	addi 3, 1, 8
80403cdc: ec 20 08 28  	fsubs 1, 0, 1
80403ce0: fc 40 08 50  	fneg 2, 1
80403ce4: 48 0b 51 f9  	bl 0x804b8edc <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xb51b4>
80403ce8: 38 61 00 08  	addi 3, 1, 8
80403cec: 38 80 00 01  	li 4, 1
80403cf0: 48 0b c8 81  	bl 0x804c0570 <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xbc848>
80403cf4: 4b fc 7f 45  	bl 0x803cbc38 <_binary_build_original_direct_draw_20260903_cameraInit_bin_end+0xfffffffffffc7f10>
80403cf8: e3 e1 00 88  	<unknown>
80403cfc: cb e1 00 80  	lfd 31, 128(1)
80403d00: e3 c1 00 78  	<unknown>
80403d04: cb c1 00 70  	lfd 30, 112(1)
80403d08: e3 a1 00 68  	<unknown>
80403d0c: cb a1 00 60  	lfd 29, 96(1)
80403d10: 83 e1 00 5c  	lwz 31, 92(1)
80403d14: 80 01 00 94  	lwz 0, 148(1)
80403d18: 83 c1 00 58  	lwz 30, 88(1)
80403d1c: 7c 08 03 a6  	mtlr 0
80403d20: 38 21 00 90  	addi 1, 1, 144
80403d24: 4e 80 00 20  	blr
