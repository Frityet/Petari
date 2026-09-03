
build/original-jpa-overwrite-20260903/DrawStripeX.o:	file format elf32-powerpc

Disassembly of section .data:

803a5bc8 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start>:
803a5bc8: 94 21 fe b0  	stwu 1, -336(1)
803a5bcc: 7c 08 02 a6  	mflr 0
803a5bd0: 90 01 01 54  	stw 0, 340(1)
803a5bd4: db e1 01 40  	stfd 31, 320(1)
803a5bd8: f3 e1 01 48  	xsmaddmdp 31, 1, 0
803a5bdc: db c1 01 30  	stfd 30, 304(1)
803a5be0: f3 c1 01 38  	xxsel 30, 1, 0, 36
803a5be4: db a1 01 20  	stfd 29, 288(1)
803a5be8: f3 a1 01 28  	<unknown>
803a5bec: db 81 01 10  	stfd 28, 272(1)
803a5bf0: f3 81 01 18  	xscmpudp 7, 1, 0
803a5bf4: db 61 01 00  	stfd 27, 256(1)
803a5bf8: f3 61 01 08  	xsmaddadp 27, 1, 0
803a5bfc: db 41 00 f0  	stfd 26, 240(1)
803a5c00: f3 41 00 f8  	xxsel 26, 1, 0, 35
803a5c04: db 21 00 e0  	stfd 25, 224(1)
803a5c08: f3 21 00 e8  	<unknown>
803a5c0c: db 01 00 d0  	stfd 24, 208(1)
803a5c10: f3 01 00 d8  	<unknown>
803a5c14: da e1 00 c0  	stfd 23, 192(1)
803a5c18: f2 e1 00 c8  	xsmsubmsp 23, 1, 0
803a5c1c: da c1 00 b0  	stfd 22, 176(1)
803a5c20: f2 c1 00 b8  	xxsel 22, 1, 0, 34
803a5c24: 39 61 00 b0  	addi 11, 1, 176
803a5c28: 48 17 2d c5  	bl 0x805189ec <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x17272c>
803a5c2c: 80 c3 01 e4  	lwz 6, 484(3)
803a5c30: 7c 7c 1b 78  	mr	28, 3
803a5c34: 80 83 00 04  	lwz 4, 4(3)
803a5c38: 83 e6 00 08  	lwz 31, 8(6)
803a5c3c: 80 84 00 1c  	lwz 4, 28(4)
803a5c40: 28 1f 00 02  	cmplwi	31, 2
803a5c44: 41 80 06 14  	bt	0, 0x803a6258 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x690>
803a5c48: 3c 00 43 30  	lis 0, 17200
803a5c4c: 3c a0 80 54  	lis 5, -32684
803a5c50: 93 e1 00 7c  	stw 31, 124(1)
803a5c54: c3 c2 18 2c  	lfs 30, 6188(2)
803a5c58: 90 01 00 78  	stw 0, 120(1)
803a5c5c: c8 25 b8 c8  	lfd 1, -18232(5)
803a5c60: ff a0 f0 90  	fmr 29, 30
803a5c64: c8 01 00 78  	lfd 0, 120(1)
803a5c68: c1 02 18 28  	lfs 8, 6184(2)
803a5c6c: ec 20 08 28  	fsubs 1, 0, 1
803a5c70: c0 03 01 4c  	lfs 0, 332(3)
803a5c74: c0 63 01 50  	lfs 3, 336(3)
803a5c78: 80 84 00 00  	lwz 4, 0(4)
803a5c7c: ec e8 00 2a  	fadds 7, 8, 0
803a5c80: ec 21 40 28  	fsubs 1, 1, 8
803a5c84: ec 88 00 28  	fsubs 4, 8, 0
803a5c88: 80 04 00 08  	lwz 0, 8(4)
803a5c8c: c0 c2 18 4c  	lfs 6, 6220(2)
803a5c90: ec 48 18 2a  	fadds 2, 8, 3
803a5c94: ef 88 08 24  	fdivs 28, 8, 1
803a5c98: c0 23 01 44  	lfs 1, 324(3)
803a5c9c: c0 03 01 48  	lfs 0, 328(3)
803a5ca0: 54 00 02 95  	rlwinm. 0, 0, 0, 10, 10
803a5ca4: ec a6 00 72  	fmuls 5, 6, 1
803a5ca8: ec 26 00 32  	fmuls 1, 6, 0
803a5cac: ec 08 18 28  	fsubs 0, 8, 3
803a5cb0: ef 67 01 72  	fmuls 27, 7, 5
803a5cb4: ef 44 01 72  	fmuls 26, 4, 5
803a5cb8: ef 22 00 72  	fmuls 25, 2, 1
803a5cbc: ef 00 00 72  	fmuls 24, 0, 1
803a5cc0: 41 82 00 20  	bt	2, 0x803a5ce0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x118>
803a5cc4: ff a0 40 90  	fmr 29, 8
803a5cc8: 3f c0 80 3a  	lis 30, -32710
803a5ccc: ff c0 40 90  	fmr 30, 8
803a5cd0: 83 a6 00 04  	lwz 29, 4(6)
803a5cd4: ff 80 e0 50  	fneg 28, 28
803a5cd8: 3b de 4b b4  	addi 30, 30, 19380
803a5cdc: 48 00 00 10  	b 0x803a5cec <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x124>
803a5ce0: 3f c0 80 3a  	lis 30, -32710
803a5ce4: 83 a6 00 00  	lwz 29, 0(6)
803a5ce8: 3b de 4b ac  	addi 30, 30, 19372
803a5cec: 38 80 00 00  	li 4, 0
803a5cf0: 38 63 01 84  	addi 3, 3, 388
803a5cf4: 48 11 a9 6d  	bl 0x804c0660 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x11a3a0>
803a5cf8: 80 1c 02 10  	lwz 0, 528(28)
803a5cfc: 3c a0 80 5e  	lis 5, -32674
803a5d00: 38 a5 bd 68  	addi 5, 5, -17048
803a5d04: 7f 83 e3 78  	mr	3, 28
803a5d08: 54 00 10 3a  	slwi 0, 0, 2
803a5d0c: 38 9c 01 84  	addi 4, 28, 388
803a5d10: 7d 85 00 2e  	lwzx 12, 5, 0
803a5d14: 7d 89 03 a6  	mtctr 12
803a5d18: 4e 80 04 21  	bctrl
803a5d1c: 38 60 00 09  	li 3, 9
803a5d20: 38 80 00 01  	li 4, 1
803a5d24: 48 11 56 fd  	bl 0x804bb420 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x115160>
803a5d28: 38 60 00 0d  	li 3, 13
803a5d2c: 38 80 00 01  	li 4, 1
803a5d30: 48 11 56 f1  	bl 0x804bb420 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x115160>
803a5d34: 57 e5 0c 3c  	rlwinm 5, 31, 1, 16, 30
803a5d38: 38 60 00 98  	li 3, 152
803a5d3c: 38 80 00 01  	li 4, 1
803a5d40: 48 11 6d f1  	bl 0x804bcb30 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x116870>
803a5d44: 3f 20 80 61  	lis 25, -32671
803a5d48: 3f 60 80 5e  	lis 27, -32674
803a5d4c: c3 e2 18 2c  	lfs 31, 6188(2)
803a5d50: 7f b7 eb 78  	mr	23, 29
803a5d54: 3b 39 fc 80  	addi 25, 25, -896
803a5d58: 3b 7b bd 74  	addi 27, 27, -17036
803a5d5c: 3f 40 cc 01  	lis 26, -13311
803a5d60: 3b 00 00 00  	li 24, 0
803a5d64: 48 00 02 4c  	b 0x803a5fb0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x3e8>
803a5d68: 3a d7 00 08  	addi 22, 23, 8
803a5d6c: 92 fc 01 e8  	stw 23, 488(28)
803a5d70: 7e c4 b3 78  	mr	4, 22
803a5d74: 38 61 00 20  	addi 3, 1, 32
803a5d78: 4b c7 75 51  	bl 0x8001d2c8 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc77008>
803a5d7c: c0 16 00 60  	lfs 0, 96(22)
803a5d80: fc 40 f8 90  	fmr 2, 31
803a5d84: a8 16 00 88  	lha 0, 136(22)
803a5d88: 38 61 00 2c  	addi 3, 1, 44
803a5d8c: fc 00 00 50  	fneg 0, 0
803a5d90: 54 00 0b f8  	rlwinm 0, 0, 1, 15, 28
803a5d94: d3 e1 00 30  	stfs 31, 48(1)
803a5d98: 7c 99 02 14  	add 4, 25, 0
803a5d9c: 7e f9 04 2e  	lfsx 23, 25, 0
803a5da0: ec 00 06 f2  	fmuls 0, 0, 27
803a5da4: c2 c4 00 04  	lfs 22, 4(4)
803a5da8: d3 e1 00 34  	stfs 31, 52(1)
803a5dac: ec 20 05 b2  	fmuls 1, 0, 22
803a5db0: ec 60 05 f2  	fmuls 3, 0, 23
803a5db4: d0 01 00 2c  	stfs 0, 44(1)
803a5db8: 4b c7 15 2d  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a5dbc: c0 16 00 60  	lfs 0, 96(22)
803a5dc0: 38 61 00 38  	addi 3, 1, 56
803a5dc4: c0 42 18 2c  	lfs 2, 6188(2)
803a5dc8: ec 20 06 b2  	fmuls 1, 0, 26
803a5dcc: fc 60 10 90  	fmr 3, 2
803a5dd0: 4b c7 15 15  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a5dd4: c0 01 00 38  	lfs 0, 56(1)
803a5dd8: 38 61 00 38  	addi 3, 1, 56
803a5ddc: c0 42 18 2c  	lfs 2, 6188(2)
803a5de0: ec 20 05 b2  	fmuls 1, 0, 22
803a5de4: ec 60 05 f2  	fmuls 3, 0, 23
803a5de8: 4b c7 14 fd  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a5dec: 80 1c 02 00  	lwz 0, 512(28)
803a5df0: 7f 83 e3 78  	mr	3, 28
803a5df4: 7e c4 b3 78  	mr	4, 22
803a5df8: 38 a1 00 14  	addi 5, 1, 20
803a5dfc: 54 00 10 3a  	slwi 0, 0, 2
803a5e00: 7d 9b 00 2e  	lwzx 12, 27, 0
803a5e04: 7d 89 03 a6  	mtctr 12
803a5e08: 4e 80 04 21  	bctrl
803a5e0c: c0 22 18 48  	lfs 1, 6216(2)
803a5e10: 38 61 00 14  	addi 3, 1, 20
803a5e14: 48 04 11 d5  	bl 0x803e6fe8 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x40d28>
803a5e18: 2c 03 00 00  	cmpwi	3, 0
803a5e1c: 41 82 00 1c  	bt	2, 0x803a5e38 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x270>
803a5e20: c0 22 18 2c  	lfs 1, 6188(2)
803a5e24: 38 61 00 14  	addi 3, 1, 20
803a5e28: c0 42 18 28  	lfs 2, 6184(2)
803a5e2c: fc 60 08 90  	fmr 3, 1
803a5e30: 4b c7 14 b5  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a5e34: 48 00 00 0c  	b 0x803a5e40 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x278>
803a5e38: 38 61 00 14  	addi 3, 1, 20
803a5e3c: 48 04 05 75  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x400f0>
803a5e40: 38 76 00 54  	addi 3, 22, 84
803a5e44: 38 81 00 14  	addi 4, 1, 20
803a5e48: 38 a1 00 08  	addi 5, 1, 8
803a5e4c: 48 11 32 f1  	bl 0x804b913c <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x112e7c>
803a5e50: c0 22 18 48  	lfs 1, 6216(2)
803a5e54: 38 61 00 08  	addi 3, 1, 8
803a5e58: 48 04 11 91  	bl 0x803e6fe8 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x40d28>
803a5e5c: 2c 03 00 00  	cmpwi	3, 0
803a5e60: 41 82 00 1c  	bt	2, 0x803a5e7c <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x2b4>
803a5e64: c0 42 18 2c  	lfs 2, 6188(2)
803a5e68: 38 61 00 08  	addi 3, 1, 8
803a5e6c: c0 22 18 28  	lfs 1, 6184(2)
803a5e70: fc 60 10 90  	fmr 3, 2
803a5e74: 4b c7 14 71  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a5e78: 48 00 00 0c  	b 0x803a5e84 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x2bc>
803a5e7c: 38 61 00 08  	addi 3, 1, 8
803a5e80: 48 04 05 31  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x400f0>
803a5e84: c0 22 18 50  	lfs 1, 6224(2)
803a5e88: 38 61 00 14  	addi 3, 1, 20
803a5e8c: 38 81 00 08  	addi 4, 1, 8
803a5e90: 48 04 10 3d  	bl 0x803e6ecc <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x40c0c>
803a5e94: 2c 03 00 00  	cmpwi	3, 0
803a5e98: 41 82 00 18  	bt	2, 0x803a5eb0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x2e8>
803a5e9c: c0 22 18 2c  	lfs 1, 6188(2)
803a5ea0: 38 61 00 08  	addi 3, 1, 8
803a5ea4: c0 42 18 28  	lfs 2, 6184(2)
803a5ea8: fc 60 08 90  	fmr 3, 1
803a5eac: 4b c7 14 39  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a5eb0: 38 61 00 14  	addi 3, 1, 20
803a5eb4: 38 81 00 08  	addi 4, 1, 8
803a5eb8: 38 b6 00 54  	addi 5, 22, 84
803a5ebc: 48 11 32 81  	bl 0x804b913c <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x112e7c>
803a5ec0: 38 76 00 54  	addi 3, 22, 84
803a5ec4: 48 04 04 ed  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x400f0>
803a5ec8: c0 21 00 08  	lfs 1, 8(1)
803a5ecc: 38 81 00 2c  	addi 4, 1, 44
803a5ed0: c0 01 00 14  	lfs 0, 20(1)
803a5ed4: 7c 85 23 78  	mr	5, 4
803a5ed8: d0 21 00 44  	stfs 1, 68(1)
803a5edc: 38 61 00 44  	addi 3, 1, 68
803a5ee0: c0 61 00 0c  	lfs 3, 12(1)
803a5ee4: 38 c0 00 02  	li 6, 2
803a5ee8: d0 01 00 48  	stfs 0, 72(1)
803a5eec: c0 41 00 18  	lfs 2, 24(1)
803a5ef0: c0 16 00 54  	lfs 0, 84(22)
803a5ef4: c0 21 00 10  	lfs 1, 16(1)
803a5ef8: d0 01 00 4c  	stfs 0, 76(1)
803a5efc: c0 01 00 1c  	lfs 0, 28(1)
803a5f00: d3 e1 00 50  	stfs 31, 80(1)
803a5f04: d0 61 00 54  	stfs 3, 84(1)
803a5f08: d0 41 00 58  	stfs 2, 88(1)
803a5f0c: c0 56 00 58  	lfs 2, 88(22)
803a5f10: d0 41 00 5c  	stfs 2, 92(1)
803a5f14: d3 e1 00 60  	stfs 31, 96(1)
803a5f18: d0 21 00 64  	stfs 1, 100(1)
803a5f1c: d0 01 00 68  	stfs 0, 104(1)
803a5f20: c0 16 00 5c  	lfs 0, 92(22)
803a5f24: d0 01 00 6c  	stfs 0, 108(1)
803a5f28: d3 e1 00 70  	stfs 31, 112(1)
803a5f2c: 48 11 2e 3d  	bl 0x804b8d68 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x112aa8>
803a5f30: c0 21 00 2c  	lfs 1, 44(1)
803a5f34: c0 01 00 20  	lfs 0, 32(1)
803a5f38: c0 81 00 30  	lfs 4, 48(1)
803a5f3c: c0 41 00 24  	lfs 2, 36(1)
803a5f40: ec 21 00 2a  	fadds 1, 1, 0
803a5f44: c0 61 00 34  	lfs 3, 52(1)
803a5f48: c0 01 00 28  	lfs 0, 40(1)
803a5f4c: ec 44 10 2a  	fadds 2, 4, 2
803a5f50: ec 63 00 2a  	fadds 3, 3, 0
803a5f54: 48 00 0d 9d  	bl 0x803a6cf0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xa30>
803a5f58: c0 02 18 2c  	lfs 0, 6188(2)
803a5f5c: d0 1a 80 00  	stfs 0, -32768(26)
803a5f60: d3 ba 80 00  	stfs 29, -32768(26)
803a5f64: c0 21 00 38  	lfs 1, 56(1)
803a5f68: c0 01 00 20  	lfs 0, 32(1)
803a5f6c: c0 81 00 3c  	lfs 4, 60(1)
803a5f70: c0 41 00 24  	lfs 2, 36(1)
803a5f74: ec 21 00 2a  	fadds 1, 1, 0
803a5f78: c0 61 00 40  	lfs 3, 64(1)
803a5f7c: c0 01 00 28  	lfs 0, 40(1)
803a5f80: ec 44 10 2a  	fadds 2, 4, 2
803a5f84: ec 63 00 2a  	fadds 3, 3, 0
803a5f88: 48 00 0d 69  	bl 0x803a6cf0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xa30>
803a5f8c: c0 02 18 28  	lfs 0, 6184(2)
803a5f90: 7f cc f3 78  	mr	12, 30
803a5f94: 7e e3 bb 78  	mr	3, 23
803a5f98: d0 1a 80 00  	stfs 0, -32768(26)
803a5f9c: d3 ba 80 00  	stfs 29, -32768(26)
803a5fa0: 7d 89 03 a6  	mtctr 12
803a5fa4: 4e 80 04 21  	bctrl
803a5fa8: 7c 77 1b 78  	mr	23, 3
803a5fac: ef bd e0 2a  	fadds 29, 29, 28
803a5fb0: 7c 17 c0 40  	cmplw	23, 24
803a5fb4: 40 82 fd b4  	bf	2, 0x803a5d68 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x1a0>
803a5fb8: ff a0 f0 90  	fmr 29, 30
803a5fbc: 57 e5 0c 3c  	rlwinm 5, 31, 1, 16, 30
803a5fc0: 38 60 00 98  	li 3, 152
803a5fc4: 38 80 00 01  	li 4, 1
803a5fc8: 48 11 6b 69  	bl 0x804bcb30 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x116870>
803a5fcc: 3f 40 80 61  	lis 26, -32671
803a5fd0: 3f 60 80 5e  	lis 27, -32674
803a5fd4: c3 e2 18 2c  	lfs 31, 6188(2)
803a5fd8: 3b 5a fc 80  	addi 26, 26, -896
803a5fdc: 3b 7b bd 74  	addi 27, 27, -17036
803a5fe0: 3f e0 cc 01  	lis 31, -13311
803a5fe4: 3b 00 00 00  	li 24, 0
803a5fe8: 48 00 02 50  	b 0x803a6238 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x670>
803a5fec: 3a dd 00 08  	addi 22, 29, 8
803a5ff0: 93 bc 01 e8  	stw 29, 488(28)
803a5ff4: 7e c4 b3 78  	mr	4, 22
803a5ff8: 38 61 00 20  	addi 3, 1, 32
803a5ffc: 4b c7 72 cd  	bl 0x8001d2c8 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc77008>
803a6000: a8 16 00 88  	lha 0, 136(22)
803a6004: fc 40 f8 90  	fmr 2, 31
803a6008: c0 16 00 64  	lfs 0, 100(22)
803a600c: 38 61 00 2c  	addi 3, 1, 44
803a6010: 54 00 0b f8  	rlwinm 0, 0, 1, 15, 28
803a6014: fc 00 00 50  	fneg 0, 0
803a6018: 7c 3a 04 2e  	lfsx 1, 26, 0
803a601c: 7c 9a 02 14  	add 4, 26, 0
803a6020: d3 e1 00 30  	stfs 31, 48(1)
803a6024: fe e0 08 50  	fneg 23, 1
803a6028: c2 c4 00 04  	lfs 22, 4(4)
803a602c: ec 00 06 72  	fmuls 0, 0, 25
803a6030: d3 e1 00 34  	stfs 31, 52(1)
803a6034: ec 20 05 f2  	fmuls 1, 0, 23
803a6038: d0 01 00 2c  	stfs 0, 44(1)
803a603c: ec 60 05 b2  	fmuls 3, 0, 22
803a6040: 4b c7 12 a5  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a6044: c0 16 00 64  	lfs 0, 100(22)
803a6048: 38 61 00 38  	addi 3, 1, 56
803a604c: c0 42 18 2c  	lfs 2, 6188(2)
803a6050: ec 20 06 32  	fmuls 1, 0, 24
803a6054: fc 60 10 90  	fmr 3, 2
803a6058: 4b c7 12 8d  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a605c: c0 01 00 38  	lfs 0, 56(1)
803a6060: 38 61 00 38  	addi 3, 1, 56
803a6064: c0 42 18 2c  	lfs 2, 6188(2)
803a6068: ec 20 05 f2  	fmuls 1, 0, 23
803a606c: ec 60 05 b2  	fmuls 3, 0, 22
803a6070: 4b c7 12 75  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a6074: 80 1c 02 00  	lwz 0, 512(28)
803a6078: 7f 83 e3 78  	mr	3, 28
803a607c: 7e c4 b3 78  	mr	4, 22
803a6080: 38 a1 00 14  	addi 5, 1, 20
803a6084: 54 00 10 3a  	slwi 0, 0, 2
803a6088: 7d 9b 00 2e  	lwzx 12, 27, 0
803a608c: 7d 89 03 a6  	mtctr 12
803a6090: 4e 80 04 21  	bctrl
803a6094: c0 22 18 48  	lfs 1, 6216(2)
803a6098: 38 61 00 14  	addi 3, 1, 20
803a609c: 48 04 0f 4d  	bl 0x803e6fe8 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x40d28>
803a60a0: 2c 03 00 00  	cmpwi	3, 0
803a60a4: 41 82 00 1c  	bt	2, 0x803a60c0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x4f8>
803a60a8: c0 22 18 2c  	lfs 1, 6188(2)
803a60ac: 38 61 00 14  	addi 3, 1, 20
803a60b0: c0 42 18 28  	lfs 2, 6184(2)
803a60b4: fc 60 08 90  	fmr 3, 1
803a60b8: 4b c7 12 2d  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a60bc: 48 00 00 0c  	b 0x803a60c8 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x500>
803a60c0: 38 61 00 14  	addi 3, 1, 20
803a60c4: 48 04 02 ed  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x400f0>
803a60c8: 38 76 00 54  	addi 3, 22, 84
803a60cc: 38 81 00 14  	addi 4, 1, 20
803a60d0: 38 a1 00 08  	addi 5, 1, 8
803a60d4: 48 11 30 69  	bl 0x804b913c <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x112e7c>
803a60d8: c0 22 18 48  	lfs 1, 6216(2)
803a60dc: 38 61 00 08  	addi 3, 1, 8
803a60e0: 48 04 0f 09  	bl 0x803e6fe8 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x40d28>
803a60e4: 2c 03 00 00  	cmpwi	3, 0
803a60e8: 41 82 00 1c  	bt	2, 0x803a6104 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x53c>
803a60ec: c0 42 18 2c  	lfs 2, 6188(2)
803a60f0: 38 61 00 08  	addi 3, 1, 8
803a60f4: c0 22 18 28  	lfs 1, 6184(2)
803a60f8: fc 60 10 90  	fmr 3, 2
803a60fc: 4b c7 11 e9  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a6100: 48 00 00 0c  	b 0x803a610c <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x544>
803a6104: 38 61 00 08  	addi 3, 1, 8
803a6108: 48 04 02 a9  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x400f0>
803a610c: c0 22 18 50  	lfs 1, 6224(2)
803a6110: 38 61 00 14  	addi 3, 1, 20
803a6114: 38 81 00 08  	addi 4, 1, 8
803a6118: 48 04 0d b5  	bl 0x803e6ecc <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x40c0c>
803a611c: 2c 03 00 00  	cmpwi	3, 0
803a6120: 41 82 00 18  	bt	2, 0x803a6138 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x570>
803a6124: c0 22 18 2c  	lfs 1, 6188(2)
803a6128: 38 61 00 08  	addi 3, 1, 8
803a612c: c0 42 18 28  	lfs 2, 6184(2)
803a6130: fc 60 08 90  	fmr 3, 1
803a6134: 4b c7 11 b1  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xffffffffffc71024>
803a6138: 38 61 00 14  	addi 3, 1, 20
803a613c: 38 81 00 08  	addi 4, 1, 8
803a6140: 38 b6 00 54  	addi 5, 22, 84
803a6144: 48 11 2f f9  	bl 0x804b913c <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x112e7c>
803a6148: 38 76 00 54  	addi 3, 22, 84
803a614c: 48 04 02 65  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x400f0>
803a6150: c0 21 00 08  	lfs 1, 8(1)
803a6154: 38 81 00 2c  	addi 4, 1, 44
803a6158: c0 01 00 14  	lfs 0, 20(1)
803a615c: 7c 85 23 78  	mr	5, 4
803a6160: d0 21 00 44  	stfs 1, 68(1)
803a6164: 38 61 00 44  	addi 3, 1, 68
803a6168: c0 61 00 0c  	lfs 3, 12(1)
803a616c: 38 c0 00 02  	li 6, 2
803a6170: d0 01 00 48  	stfs 0, 72(1)
803a6174: c0 41 00 18  	lfs 2, 24(1)
803a6178: c0 16 00 54  	lfs 0, 84(22)
803a617c: c0 21 00 10  	lfs 1, 16(1)
803a6180: d0 01 00 4c  	stfs 0, 76(1)
803a6184: c0 01 00 1c  	lfs 0, 28(1)
803a6188: d3 e1 00 50  	stfs 31, 80(1)
803a618c: d0 61 00 54  	stfs 3, 84(1)
803a6190: d0 41 00 58  	stfs 2, 88(1)
803a6194: c0 56 00 58  	lfs 2, 88(22)
803a6198: d0 41 00 5c  	stfs 2, 92(1)
803a619c: d3 e1 00 60  	stfs 31, 96(1)
803a61a0: d0 21 00 64  	stfs 1, 100(1)
803a61a4: d0 01 00 68  	stfs 0, 104(1)
803a61a8: c0 16 00 5c  	lfs 0, 92(22)
803a61ac: d0 01 00 6c  	stfs 0, 108(1)
803a61b0: d3 e1 00 70  	stfs 31, 112(1)
803a61b4: 48 11 2b b5  	bl 0x804b8d68 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x112aa8>
803a61b8: c0 21 00 2c  	lfs 1, 44(1)
803a61bc: c0 01 00 20  	lfs 0, 32(1)
803a61c0: c0 81 00 30  	lfs 4, 48(1)
803a61c4: c0 41 00 24  	lfs 2, 36(1)
803a61c8: ec 21 00 2a  	fadds 1, 1, 0
803a61cc: c0 61 00 34  	lfs 3, 52(1)
803a61d0: c0 01 00 28  	lfs 0, 40(1)
803a61d4: ec 44 10 2a  	fadds 2, 4, 2
803a61d8: ec 63 00 2a  	fadds 3, 3, 0
803a61dc: 48 00 0b 15  	bl 0x803a6cf0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xa30>
803a61e0: c0 02 18 2c  	lfs 0, 6188(2)
803a61e4: d0 1f 80 00  	stfs 0, -32768(31)
803a61e8: d3 bf 80 00  	stfs 29, -32768(31)
803a61ec: c0 21 00 38  	lfs 1, 56(1)
803a61f0: c0 01 00 20  	lfs 0, 32(1)
803a61f4: c0 81 00 3c  	lfs 4, 60(1)
803a61f8: c0 41 00 24  	lfs 2, 36(1)
803a61fc: ec 21 00 2a  	fadds 1, 1, 0
803a6200: c0 61 00 40  	lfs 3, 64(1)
803a6204: c0 01 00 28  	lfs 0, 40(1)
803a6208: ec 44 10 2a  	fadds 2, 4, 2
803a620c: ec 63 00 2a  	fadds 3, 3, 0
803a6210: 48 00 0a e1  	bl 0x803a6cf0 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0xa30>
803a6214: c0 02 18 28  	lfs 0, 6184(2)
803a6218: 7f cc f3 78  	mr	12, 30
803a621c: 7f a3 eb 78  	mr	3, 29
803a6220: d0 1f 80 00  	stfs 0, -32768(31)
803a6224: d3 bf 80 00  	stfs 29, -32768(31)
803a6228: 7d 89 03 a6  	mtctr 12
803a622c: 4e 80 04 21  	bctrl
803a6230: 7c 7d 1b 78  	mr	29, 3
803a6234: ef bd e0 2a  	fadds 29, 29, 28
803a6238: 7c 1d c0 40  	cmplw	29, 24
803a623c: 40 82 fd b0  	bf	2, 0x803a5fec <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_start+0x424>
803a6240: 38 60 00 09  	li 3, 9
803a6244: 38 80 00 02  	li 4, 2
803a6248: 48 11 51 d9  	bl 0x804bb420 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x115160>
803a624c: 38 60 00 0d  	li 3, 13
803a6250: 38 80 00 02  	li 4, 2
803a6254: 48 11 51 cd  	bl 0x804bb420 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x115160>
803a6258: e3 e1 01 48  	<unknown>
803a625c: cb e1 01 40  	lfd 31, 320(1)
803a6260: e3 c1 01 38  	<unknown>
803a6264: cb c1 01 30  	lfd 30, 304(1)
803a6268: e3 a1 01 28  	<unknown>
803a626c: cb a1 01 20  	lfd 29, 288(1)
803a6270: e3 81 01 18  	<unknown>
803a6274: cb 81 01 10  	lfd 28, 272(1)
803a6278: e3 61 01 08  	<unknown>
803a627c: cb 61 01 00  	lfd 27, 256(1)
803a6280: e3 41 00 f8  	<unknown>
803a6284: cb 41 00 f0  	lfd 26, 240(1)
803a6288: e3 21 00 e8  	<unknown>
803a628c: cb 21 00 e0  	lfd 25, 224(1)
803a6290: e3 01 00 d8  	<unknown>
803a6294: cb 01 00 d0  	lfd 24, 208(1)
803a6298: e2 e1 00 c8  	<unknown>
803a629c: ca e1 00 c0  	lfd 23, 192(1)
803a62a0: e2 c1 00 b8  	<unknown>
803a62a4: 39 61 00 b0  	addi 11, 1, 176
803a62a8: ca c1 00 b0  	lfd 22, 176(1)
803a62ac: 48 17 27 8d  	bl 0x80518a38 <_binary_build_original_jpa_overwrite_20260903_DrawStripeX_bin_end+0x172778>
803a62b0: 80 01 01 54  	lwz 0, 340(1)
803a62b4: 7c 08 03 a6  	mtlr 0
803a62b8: 38 21 01 50  	addi 1, 1, 336
803a62bc: 4e 80 00 20  	blr
