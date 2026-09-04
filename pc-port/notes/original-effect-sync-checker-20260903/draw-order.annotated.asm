
build/original-effect-sync-checker-20260903/draw-order.o:	file format elf32-powerpc

Disassembly of section .data:

800c71dc <_binary_build_original_effect_sync_checker_20260903_draw_order_bin_start>:
800c71dc: 94 21 ff d0  	stwu 1, -48(1)
800c71e0: 7c 08 02 a6  	mflr 0
800c71e4: 90 01 00 34  	stw 0, 52(1)
800c71e8: 93 e1 00 2c  	stw 31, 44(1)
800c71ec: 93 c1 00 28  	stw 30, 40(1)
800c71f0: 98 81 00 14  	stb 4, 20(1)
800c71f4: 3c 80 80 58  	lis 4, -32680
800c71f8: 84 c4 85 94  	lwzu 6, -31340(4)
800c71fc: 80 01 00 14  	lwz 0, 20(1)
800c7200: 80 a4 00 04  	lwz 5, 4(4)
800c7204: 80 84 00 08  	lwz 4, 8(4)
800c7208: 90 c1 00 18  	stw 6, 24(1)
800c720c: 90 a1 00 1c  	stw 5, 28(1)
800c7210: 90 81 00 20  	stw 4, 32(1)
800c7214: 90 01 00 24  	stw 0, 36(1)
800c7218: 80 03 00 04  	lwz 0, 4(3)
800c721c: 83 c3 00 00  	lwz 30, 0(3)
800c7220: 54 00 18 38  	slwi 0, 0, 3
800c7224: 90 c1 00 08  	stw 6, 8(1)
800c7228: 7f fe 02 14  	add 31, 30, 0
800c722c: 90 a1 00 0c  	stw 5, 12(1)
800c7230: 90 81 00 10  	stw 4, 16(1)
800c7234: 48 00 00 1c  	b 0x800c7250 <_binary_build_original_effect_sync_checker_20260903_draw_order_bin_start+0x74>
800c7238: 88 81 00 24  	lbz 4, 36(1)
800c723c: 7f c3 f3 78  	mr	3, 30
800c7240: 39 81 00 18  	addi 12, 1, 24
800c7244: 48 45 16 6d  	bl 0x805188b0 <_binary_build_original_effect_sync_checker_20260903_draw_order_bin_end+0x451640>  # __ptmf_scall
800c7248: 60 00 00 00  	nop
800c724c: 3b de 00 08  	addi 30, 30, 8
800c7250: 7c 1e f8 40  	cmplw	30, 31
800c7254: 40 82 ff e4  	bf	2, 0x800c7238 <_binary_build_original_effect_sync_checker_20260903_draw_order_bin_start+0x5c>
800c7258: 80 01 00 34  	lwz 0, 52(1)
800c725c: 83 e1 00 2c  	lwz 31, 44(1)
800c7260: 83 c1 00 28  	lwz 30, 40(1)
800c7264: 7c 08 03 a6  	mtlr 0
800c7268: 38 21 00 30  	addi 1, 1, 48
800c726c: 4e 80 00 20  	blr
